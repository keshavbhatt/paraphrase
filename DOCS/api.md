# Daemon API

`paraphrase-daemon` owns the model and does all inference (PLAN D1). Clients (the CLI, the
main window, the popup) talk to it over D-Bus. This page is the contract; PLAN 5.1 has the
original sketch.

| | |
|---|---|
| Bus name | `com.ktechpit.paraphrase.Daemon` (session bus, ADR-007) |
| Object path | `/com/ktechpit/paraphrase/Daemon` |
| Interface | `com.ktechpit.paraphrase.Daemon1` (the number versions the API) |
| Code | `daemon/src/api/daemon_service.h`, tests in `daemon/tests/tst_api.cpp` |

## Starting the daemon (ADR-010)

A client that does not find the name on the bus starts `paraphrase-daemon` itself. The
daemon's exit code says what happened:

| Code | Meaning | What the client does |
|---|---|---|
| 0 | Exited after a quiet period, or another daemon already owns the name | Use the one on the bus |
| 1 | Bad setup: no skills folder, a broken skill file, a socket it cannot create | Show the message it printed |
| 3 | No session bus | Private mode |
| 4 | The name may not be owned (a snap waiting for its name declaration) | Private mode |

**Private mode.** The client starts `paraphrase-daemon --private-socket PATH` as its child,
with PATH in its runtime folder, and connects to `unix:path=PATH` as a D-Bus peer. Same
object, same interface, same signals. The socket is mode 0600, and the daemon ends when the
client that started it does. Never TCP.

Options: `--skills-dir`, `--models-dir` (also `PARAPHRASE_MODELS_DIR`), `--idle-unload
SECONDS` (default 600: the model is unloaded), `--idle-exit SECONDS` (default 1800: the
daemon exits; never while a download runs).

## Methods

| Method | Returns | |
|---|---|---|
| `RunSkill(s skill_id, a{sv} input, a{sv} options)` | `s request_id` | Queues a run; the answer comes as signals |
| `Cancel(s request_id)` | | Stops one of the caller's requests |
| `Warm(s skill_id, a{sv} input)` | | Reads the text ahead for a likely action (ADR-005) |
| `ListSkills()` | `s` JSON array | Every skill and preset: `id`, `base`, `title`, `tier`, `output`, `variants`, `max_input_tokens`, `main_input` (`text`, or `context` for a skill that answers a message) |
| `Status()` | `s` JSON object | Loaded model, threads, queue, memory, the computer |
| `ListModels()` | `s` JSON array | Offered models, installed or not, with their source |
| `DownloadModel(s model_id)` | | Downloads a model and its draft head |
| `PauseDownload(s model_id)` | | Keeps what arrived; `DownloadModel` resumes |
| `DeleteModel(s model_id)` | | Deletes a model and its draft head |

**`input`** (all strings): `text` (what to work on; required unless the skill's
`main_input` is `context`), `context` (the message to reply to; required when it is), `target_lang` (a language code the user
chose, for example `fr`; otherwise the output keeps the input's language; only skills whose
prompt takes it, today rewrite, accept it). Any other key is
an error. **`options`**: `model` (a model id from `ListModels` with role `model`) runs the
request on that model instead of the tier's (the eval harness, and a future Settings choice);
any other key is an error.

Skill ids are those `ListSkills` returns: `proofread`, `reply`, and a preset as
`<skill>.<preset>` (`rewrite.professional`).

### Behaviour

- One request runs at a time, on the inference thread; the rest wait in order.
- **Cancel on change** (PLAN 5.3): a new `RunSkill` from a client cancels that client's
  earlier requests, queued or running. Each gets `Error(id, "cancelled", ...)`.
- A client can cancel only its own requests.
- The skill's tier picks the model (ADR-016): quality skills run on the quality model when it
  is installed and fits the computer's memory, otherwise on the fast one.
- `Warm` is best effort: never an error, and dropped when a real request arrives. If the
  same request follows, the model reads only the last token of its prompt again.

### Error replies

Problems known when a request is submitted come back as D-Bus errors on the call itself:

| Error name | When |
|---|---|
| `com.ktechpit.paraphrase.Error.InvalidInput` | Unknown skill, model or input key; a missing text or context; a `target_lang` the skill does not take, or an unknown one |
| `com.ktechpit.paraphrase.Error.NoModel` | No model for the skill's tier is installed |
| `com.ktechpit.paraphrase.Error.Busy` | `DeleteModel` on a model in use or downloading |
| `com.ktechpit.paraphrase.Error.Failed` | `DeleteModel` could not remove the files |

Every message is a sentence for the user.

## Signals

`Token`, `Retrying`, `Done` and `Error` are sent **only to the client that made the
request** (targeted signals), never broadcast: other programs on the session bus cannot
watch someone's text go by. Subscribe to them with a match rule on the interface; they
arrive on your own connection.

| Signal | |
|---|---|
| `Token(s request_id, i variant, s text)` | The next piece of the answer, whole characters only |
| `Retrying(s request_id, s reason)` | The answer failed the language check and is being written again: clear what was streamed (ADR-006) |
| `Done(s request_id, s result)` | The final answer, JSON below |
| `Error(s request_id, s code, s message)` | The request ended without an answer |
| `DownloadProgress(s status)` | Broadcast to all clients (no user text), JSON below |

**`Error` codes**: `cancelled`, `too-long` (the input is over the skill's
`max_input_tokens`), `model` (the model could not be loaded), `engine` (the model failed
while reading or writing), `incomplete` (the answer left out part of the text twice, so it is
not shown; DOCS/skills.md, "Length check").

### `Done` result

```json
{
  "request_id": "6f1c...",
  "skill": "reply",
  "model": "gemma-4-e2b-qat-q4_0",
  "output": "{\"replies\": [\"Sure, see you then!\", ...]}",
  "replies": ["Sure, see you then!", "Can we make it 7?", "Sorry, I can't tonight."],
  "retried": false,
  "warning": "",
  "finish": "stop",
  "load_s": 0,
  "calls": [
    {"ttft_s": 0.61, "total_s": 2.4, "prompt_tokens": 182, "cached_tokens": 150,
     "generated_tokens": 31, "draft_tokens": 0, "draft_accepted": 0}
  ]
}
```

- `output`: the answer as the skill produces it (text, or JSON for a JSON skill).
- `replies`: present for JSON skills and variants: the separate answers.
- `warning`: set when the retry also failed the language check (`language: <why>`); the
  answer is still shown, with the warning, never hidden (ADR-006).
- `finish`: `stop`, or `length` when the output limit cut it short.
- `load_s`: seconds this request spent loading the model first; 0 when it was loaded.
- `calls`: one per model call (two after a retry), for the latency bench.

### `DownloadProgress` status

```json
{"model": "gemma-4-e2b-qat-q4_0", "state": "downloading", "received": 1210000000,
 "total": 3349516256, "verified": 0, "error": ""}
```

`state`: `waiting`, `downloading`, `verifying` (`verified` counts the bytes checked so far),
`paused`, `failed` (`error` says why, for the user), `installed`. Byte updates come at most
four times a second; state changes always.

### `ListModels` entry

```json
{"id": "gemma-4-e2b-qat-q4_0", "name": "Gemma 4 E2B", "role": "model", "tiers": ["fast"],
 "draft_for": [], "installed": true, "fits": true, "recommended": true,
 "size": 3349516256, "on_disk": 3349516256, "ram_required_gb": 8, "memory_gb": 5,
 "licence": "Apache-2.0", "licence_url": "https://ai.google.dev/gemma/docs/gemma_4_license",
 "source": "https://huggingface.co/google/gemma-4-E2B-it-qat-q4_0-gguf/resolve/675cff4.../gemma-4-E2B_q4_0-it.gguf",
 "revision": "675cff4...", "sha256": "fa401b55...", "path": "/home/u/.local/share/paraphrase/models/...",
 "download": {"state": "downloading", "received": 0, "total": 0, "error": ""}}
```

`download` is present only while a download is queued, running, paused or failed. Draft
heads are listed with `role: draft` and come with their model; the app shows them as
"speed-up" (ADR-004). `source`, `revision` and `sha256` are shown in the UI so a careful user
can check the download (PLAN 8).

## Privacy

- The daemon logs request ids, skill ids, sizes and timings, never text (CLAUDE.md).
- Text reaches only the client that sent it (targeted signals, a private socket).
- The only network use is `DownloadModel`: HTTPS to Hugging Face at the pinned revision
  (ADR-015). Nothing else leaves the computer.
