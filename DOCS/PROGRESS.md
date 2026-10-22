# Progress

Newest first. One entry per working session.

## Milestone status

| Milestone | Status |
|---|---|
| M0 Feasibility spike | done: go (DOCS/models.md, 2026-10-22) |
| M1 Daemon | in progress |
| M2 Popup on X11 + KDE Wayland | todo |
| M3 Polish + GNOME/wlroots | todo |
| M4 Mail | todo |
| M5 Release | todo |
| M6 IME + Windows | todo |

## 2026-10-22 - M1 step 5: models

- Catalog schema 2 (DOCS/models.md, Catalog): Gemma 4 E2B is the fast-tier model and E4B
  the quality-tier one, with the memory they need and use, and their draft heads. The daemon
  builds the catalog in and never fetches it (ADR-015); the eval harness reads the same file.
- `paraphrase_models`: the catalog reader (strict; file names, repositories and checksums are
  checked because they become paths and URLs), the models folder (ADR-009 paths, the verified
  stamps of scripts/fetch_model.py, so development models count as installed), the hardware
  probe and the tier choice (ADR-016), and the downloader (one model at a time, `.part` files
  resumed with Range across restarts, sha256 on the thread pool, HTTPS only).
- Probe on machine B: 10 threads of 14 CPUs, the M0 sweep's fastest; it leaves out the two
  low-power cores. tst_engine now uses it and runs in 49 s instead of 84 s.
- The quality model is offered from 16 GB; on smaller computers rewrite runs on the fast one.
- Checked once by hand: the downloader fetched the E2B draft head (59 MB) from Hugging Face,
  followed its redirect and verified it (`PARAPHRASE_NETWORK_TESTS=1`; the suite stays offline).
- Open for step 6: the engine's few error messages are plain strings; they need `tr()` when
  they reach clients through the API.
- Next: step 6, the D-Bus API (service, request queue, inference thread, idle unload, private
  fallback).

## 2026-10-22 - M1 step 4b: MTP drafting per skill

- The engine loads the model's MTP draft head as llama-server does (`--spec-type draft-mtp`,
  default draft length 3) and drafts requests whose skill says `speculative: draft`
  (proofread, rewrite; ADR-004). A reply in between does not break the cache: the engine
  tracks what the draft context holds and re-reads the prompt where the two differ.
- Greedy answers are identical with and without drafting (tst_engine, 10 prompts).
  Proposals accepted on those prompts: proofread 74%, rewrite 30%; llama-server with the same
  head on the same prompts: 65% and 29%.
- Fix found on the way: llama-server feeds the prompt to the sampler, so the repeat penalty
  counts words from the prompt. The engine did not, which changed rewrite and reply answers
  (proofread has no penalty). With the fix, greedy rewrites match llama-server's for the same
  cache state.
- Memory 4.71 GB with the head (4.49 GB without), load 4.8 s (3.5 s without), machine B.
- Model note: greedy E2B writes a malformed Tamil word in one rewrite vector (rw-ta-001), in
  llama-server too; rewrite samples at temperature 0.6 in use. Recorded for the M1 eval.
- Next: step 5, models (catalog, downloader, hardware probe with the thread count, tier).

## 2026-10-22 - M1 step 4: the engine

- `paraphrase_engine`: `LlamaEngine`, the only code that includes llama.cpp. It builds prompts
  as llama-server does (the model's chat template, thinking off) and matches its tokens for
  all 15 vector prompts (tests/vectors/prompt_tokens.json, from eval/export_prompt_tokens.py).
  One deliberate difference: user text is never parsed for control tokens, so text that spells
  out `<|turn>` stays plain text. Prompt prefix cache (a repeat reuses all but the last
  token), the skill's GBNF grammar, streaming of whole characters, an output limit, cancel,
  and variants decoded in one batch.
- `SkillRunner`: a port of eval/paraeval/engine.py. Checks the language of every output,
  retries once with a line naming the language (temperature 0.3 for greedy skills, seed
  +7919), and returns a second failure with a warning (ADR-006).
- Measured on machine B with E2B: peak memory 4.49 GB with the full-size sliding-window cache,
  below llama-server in M0 (4.5 to 5.1 GB).
- Found: llama.cpp's default thread count on this hybrid Intel CPU is 2 (performance cores
  only), where the M0 sweep found 10 fastest. The hardware probe (step 5) picks the threads.
- Next: step 4b, per-skill MTP drafting (ADR-004); then step 5, models.

## 2026-10-22 - M1 steps 1 to 3: tooling, language logic and skills in C++

- Tooling as Omnidict (owner): DOCS/CODING_STANDARDS.md, .clang-tidy, scripts/dev-build.sh and
  dev-run.sh against the KDE Qt snap SDK and runtime, Makefile (build, test, lint, format) and
  CI (.github/workflows/ci.yml). The SDK library path is no longer exported for cmake: it
  breaks the host's cmake on Arch (libnettle.so.8), and the SDK's moc runs without it.
- Step 1: CMake at the repository root, llama.cpp linked as a library (native for dev,
  portable for packages, ADR-008), headers of third-party code treated as system headers.
- Step 2: `paraphrase_util` with the language detector and check. The word lists and names
  moved to daemon/data/language.json, read by both the daemon and the Python harness; the
  Python code stays the reference and eval/export_vectors.py writes tests/vectors/. C++
  matches all 774 detection and 429 check vectors.
- Step 3: yaml-cpp 0.9.0 (ADR-014) and `paraphrase_skills`: registry, schema v3 loader with
  PyYAML's scalar rules, template rendering with fences, request inputs. C++ matches all 11
  skills, 254 input builds, 293 rendered prompts and 12 template cases; broken files (bad
  YAML, unknown values, a grammar outside skills/) are rejected by name.
- Next: step 4, the engine (model, chat template, prompt cache, grammar, streaming,
  variants, cancel, language retry, per-skill MTP drafting).

## 2026-10-22 - Mocks approved, packaging decided, M1 started

- Owner approved all eight mocks (DOCS/mocks/index.html): main window proofread and reply,
  popup actions, proofread, reply and change tone, first-run setup, settings.
- Packaging analysis (DOCS/packaging.md) and ADR-007 to ADR-013: app ID
  `com.ktechpit.paraphrase`, portable llama.cpp build, data folders per format, clients start
  the daemon, D-Bus as the API, one package per format, standalone first (owner: every
  feature works without desktop integration). PLAN D14, D15, M2 and M5 updated.

## 2026-10-22 - M0 decided: go with Gemma 4 E2B (fast) and E4B (quality)

- Owner's answers: quality-tier tie to Gemma E4B (family consistency with E2B); apply
  every speed lever before judging machine A; Hindi/Hinglish floor over all 15 items per
  language at 10%; an engine-level language check; rewrites keep the input language. The
  owner then asked Claude to decide the open product questions (ADR-003 to ADR-006).
- Spot-check: Claude rated all 100 answers in two passes plus a final check (3 changes);
  no human rated. E4B 14/5/1, Qwen 4B 14/4/2, E2B 11/7/2 (good/ok/bad).
- Speed levers measured (DOCS/models.md, Latency): only the Gemma MTP draft heads help
  (proofread -26% on A, -42% to -46% on B); n-gram drafting, Q4_K_M, Vulkan on the iGPU,
  more threads, E2B as a draft model and batched reply variants do not. llama-server
  v0.7.0 ignores per-request speculative settings, so the bench times reply and the other
  skills on two server setups; the daemon switches per request (M1).
- Strict result: the fast tier on machine A misses twice the old targets (first token about
  3 s against 1.6 s). ADR-003 ships it anyway with targets from measurements (PLAN 5.4,
  `eval/targets.json`, `bench_latency.py --check`).
- Engine (`eval/paraeval/engine.py`): language check with one retry that names the language
  (E2B Hinglish rewrites into English: 5 of 7 before, 1 of 7 after), per-skill speculative
  decoding (skill schema v3), batched variants. Both models pass the floor (2% and 7%);
  Claude read all 90 Hindi and Hinglish outputs, none garbled.
- Other languages: the detector called every language English or Hindi; it now names about
  40 and says "its own language" when unsure, without changing any existing item's
  detection. Eight-language smoke set: 48 of 48 outputs kept their language for both models.
- Mocks (DOCS/DESIGN.md, DOCS/mocks/, proposed, waiting for the owner): popup actions,
  proofread, reply, change tone, first-run setup, settings. Also on a design canvas.
- Waiting on the owner: mock approval; human-written eval items; a real older laptop for
  the bench before M5.

## 2026-10-22 - M0 measurements

- llama.cpp v0.7.0 built CPU only and CPU + Vulkan. Five candidates and the judge fetched by
  pinned revision and sha256 (`models.json`, `DOCS/models.md`).
- Eval harness, three skills (proofread, reply, rewrite with 9 tones) and 195 project-written
  eval items. No public dataset is used.
- Prompt revision 1: models answered English text in Hindi (Qwen3.5-4B rewrote 49 of 50
  English texts into Devanagari). Revision 2 removed Hindi from the system prompts and names
  the detected language in the user message: English drift went to 0 for every model.
- Revision 2 left two problems: proofread changed correct casual text (up to 6 of 12 clean
  items), and the professional rewrite translated Hinglish into English (3 to 5 of 7).
  Revision 3 addresses both; its eval, the judge (Sarvam-30B, tier-relevant pairs only) and
  the latency bench run overnight.
- Latency (revision 2, `eval/reports/m0-v2/summary.md`): no candidate meets the PLAN 5.4
  totals on A or B. The iGPU through Vulkan is slower than the CPU on this laptop.
- Waiting on the owner: the results table, the spot-check ratings, then the go/no-go and
  the revised latency targets.

## 2026-10-21 - M0 started

- Owner answers for M0 (recorded in `PLAN.md` section 11):
  - Q4: 8 GB CPU-only (machine A) is a must-have for the fast tier only (proofread, reply,
    format). The quality tier may be labelled "16 GB or GPU recommended" with a slower
    fallback on A. A no-go on A for the fast tier is fatal.
  - Machines: B is the development laptop (Intel Core Ultra 7 255U, 30 GB RAM, Intel
    Graphics (Arrow Lake) iGPU through Mesa Vulkan). A is emulated on the same laptop and its
    numbers are labelled "optimistic (emulated)"; re-run on a real older laptop before M5.
  - Q6: Hindi and Hinglish are a tie-breaker with a hard floor: reject a model that answers
    Hindi in English, breaks Devanagari, or rewrites Hinglish into English.
  - Q2: licensing as Omnidict, with the licensing module carve-out (ADR-002).
  - Eval data written for the project (no JFLEG in the repository), with room for about
    20 human-written items per skill in separate files, scored separately.
  - Judge: a model from a different family than the candidate it scores, blind pairwise,
    plus hard checks. The owner's spot-check of 20 outputs per model is the final word.
- Scaffold: git repository, licence files, editor and format config, Python tooling.
