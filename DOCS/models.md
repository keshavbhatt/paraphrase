# Models: catalog, M0 measurements and decision

This page records which language models Paraphrase can run, where they come from, their
licences, and the M0 feasibility results that chose the defaults (PLAN 10, M0). Raw numbers
live in `eval/reports/`; this page summarises them.

## Catalog

`models.json` pins every model to an upstream Hugging Face revision and lists each file's
size and sha256. `scripts/fetch_model.py <id>` downloads from that revision and refuses a
file whose sha256 does not match. Models are never committed or re-hosted (PLAN D8).
Checked on Hugging Face on 2026-10-21.

| id | role | upstream model | GGUF from | quant | download | licence |
|---|---|---|---|---|---|---|
| `qwen3.5-0.8b-q4km` | candidate | Qwen/Qwen3.5-0.8B | bartowski/Qwen_Qwen3.5-0.8B-GGUF | Q4_K_M (imatrix) | 0.58 GB | Apache-2.0 |
| `qwen3.5-2b-q4km` | candidate | Qwen/Qwen3.5-2B | bartowski/Qwen_Qwen3.5-2B-GGUF | Q4_K_M (imatrix) | 1.40 GB | Apache-2.0 |
| `qwen3.5-4b-q4km` | candidate | Qwen/Qwen3.5-4B | bartowski/Qwen_Qwen3.5-4B-GGUF | Q4_K_M (imatrix) | 3.01 GB | Apache-2.0 |
| `gemma-4-e2b-qat-q4_0` | model, fast tier | google/gemma-4-E2B-it | google/gemma-4-E2B-it-qat-q4_0-gguf (official) | Q4_0, quantisation-aware trained | 3.35 GB | Apache-2.0 |
| `gemma-4-e4b-qat-q4_0` | model, quality tier | google/gemma-4-E4B-it | google/gemma-4-E4B-it-qat-q4_0-gguf (official) | Q4_0, quantisation-aware trained | 5.15 GB | Apache-2.0 |
| `sarvam-30b-q4km` | judge only | sarvamai/sarvam-30b | sarvamai/sarvam-30b-gguf (official, 6 parts) | Q4_K_M | 19.57 GB | Apache-2.0 |
| `gemma-4-e2b-q4km` | experiment | google/gemma-4-E2B-it | bartowski/google_gemma-4-E2B-it-GGUF | Q4_K_M (imatrix) | 3.46 GB | Apache-2.0 |
| `gemma-4-e2b-mtp-q4_0` | draft for E2B | google/gemma-4-E2B-it | ggml-org/gemma-4-E2B-it-GGUF | Q4_0 | 0.06 GB | Apache-2.0 |
| `gemma-4-e4b-mtp-q4_0` | draft for E4B | google/gemma-4-E4B-it | ggml-org/gemma-4-E4B-it-GGUF | Q4_0 | 0.06 GB | Apache-2.0 |

**Schema 2** (after M0). The daemon builds `models.json` in (ADR-015) and reads only the
entries it can offer:

- `role`: `model` ships and has `tiers` (`fast`, `quality`); `draft` is a draft head for the
  models in `draft_for` (ADR-004), downloaded with them and shown as "speed-up"; `candidate`,
  `experiment` and `judge` are for evaluation only.
- `name`: what the app shows ("Gemma 4 E2B").
- `ram_required`: the computer's total memory, in GB, the model needs (8 for the fast tier,
  16 for the quality tier). A computer reporting slightly less, as most do, still counts.
- `memory_gb`: peak memory in use with its draft head (M0, Chosen setup below), for the
  first-run and settings screens.
- `files`: each file's name, size and sha256; the download URL is
  `https://huggingface.co/<hf_repo>/resolve/<revision>/<file>`.

Notes on the choices:

- **Qwen3.5** publishes no GGUF of its own; ggml-org has only the 0.8B. bartowski's
  imatrix Q4_K_M files are used for all three sizes so the family is quantised one way.
- **Gemma 4** is taken from Google's own quantisation-aware-trained Q4_0 files rather than a
  third-party Q4_K_M: QAT keeps more quality at 4 bits, and the files are official. The "E"
  models carry per-layer embeddings, so the file is larger than the effective parameter
  count suggests (E2B is 5.1B parameters stored, about 2B computed per token).
- **Phi-4-mini** (MIT), listed in PLAN 5.5, was dropped before benchmarking with the owner's
  agreement: Hindi and Hinglish are a selection criterion and it is the weakest of the
  family candidates there.
- **The judge** must come from a different family than the models it compares (owner,
  2026-10-21). A Qwen-versus-Gemma pair rules out both families, so one judge from a third
  family scores every pair: Sarvam-30B, a mixture-of-experts model (2.4B active) trained
  for Indian languages. It is used offline for evaluation only and never ships.

## Inference engine

llama.cpp **v0.7.0** (2026-10-15, MIT), pinned as the submodule `third_party/llama.cpp`, built
twice by `scripts/build-llama.sh`: CPU only and CPU + Vulkan, both with `GGML_NATIVE`. In M0
the eval harness drives `llama-server` bound to a Unix socket (never a TCP port) with
`--reasoning off`, which turns thinking off through each model's own chat template (D12).
The daemon links the library directly from M1 (D2).

## Method

- **Machines.** B is the development laptop: Intel Core Ultra 7 255U (2 P-cores, 8 E-cores,
  2 low-power E-cores, 14 threads, AVX2 and AVX-VNNI, no AVX-512), 30 GB RAM, Intel Graphics
  (Arrow Lake iGPU) through Mesa Vulkan, on AC power in the "balanced" profile. It is run
  two ways: CPU only (10 threads, the fastest in a 2 to 14 sweep) and Vulkan with every
  layer on the iGPU. **A is emulated** on the same laptop: 4 E-cores (taskset 4-7), 4
  threads, CPU only, a hard 8 GB memory cap without swap (`systemd-run --user --scope -p
  MemoryMax=8G`). **A's numbers are optimistic (emulated):** these E-cores reach 4.3 GHz and
  are faster than a typical older 4-core laptop, and a real 8 GB machine has less than 8 GB
  free for the model. **Re-run the bench on a real older laptop before M5.**
- **Latency** (`eval/bench_latency.py`): the five English dataset items closest to 100 words
  per task, one warm-up request per task first (the daemon keeps each skill's system prompt
  in the KV cache, PLAN 5.3), then the median of 5 runs. Time to first token is measured to
  the first streamed content token. Model load is timed twice: cold, with the model's pages
  evicted from the page cache, and warm. Peak RAM is the server's peak resident set (VmHWM);
  with Vulkan, weights in iGPU memory are not counted there.
- **Quality** (`eval/run_eval.py`): every dataset item, one request each, greedy decoding
  for proofread and the skill's own sampling for reply and rewrite.
  - Proofread: word-level edits against the reference, precision, recall and F0.5 (precision
    weighs double: a wrong edit costs more than a missed one), exact match, false alarms on
    already-correct text, protected tokens kept (names, numbers, dates, URLs, emails, code).
  - Reply: valid JSON with three replies, replies distinct, at most 15 words.
  - Rewrite: protected facts kept, no preamble, length ratio.
  - All skills: **the language floor** (owner, PLAN 11 Q6): a Hindi input answered outside
    Devanagari, broken Devanagari, Hinglish turned into English or Devanagari, or English
    answered in Hindi or Hinglish is a failure.
- **Judge** (`eval/run_judge.py`): blind pairwise comparison of reply and rewrite outputs by
  the judge, both orders, a win only when both orders agree; outputs that fail a hard check
  lose without being judged.
- **Owner's spot-check** (`eval/spotcheck.py`): 20 items per model, the same items for every
  model, blind. This is the final word on quality.

## Eval datasets

All items were written for this project in M0 (generated with Claude, an LLM, and checked by
script), so they are covered by the repository's licence. **No public dataset is used.** Public
grammatical-error-correction sets are mostly non-commercial (JFLEG: CC BY-NC-SA 4.0;
W&I+LOCNESS and GYAFC: restricted licences), so none is in the repository (owner,
2026-10-21). Each skill has a separate `*.human.jsonl` file for items the owner writes (messy
real typing, typos, Hinglish); reports score those separately from the generated items.

From M1 the project writes all items, the hard ones too (owner, 2026-10-22: "fill the samples,
make complex samples instead of asking me"). Two more sets per skill:

- `*.hard.jsonl`: complex cases aimed at the models' weak spots, and the owner's own CLI
  tests. Part of every run (`--set all`).
- `*.holdout.jsonl`: written before a prompt is tuned and never looked at item by item while
  tuning; run alone (`--set holdout`) as the final check that a revision generalises.

Proofread has 43 hard items (30 English with tense against time words, missing words,
prepositions, non-native patterns, run-ons, 3 prompt injections and 5 already-correct traps;
13 Hindi and Hinglish with gender, number and honorific agreement and 1 injection) and 25
holdout items.

| skill | English | Hindi (Devanagari) | Hinglish | notes |
|---|---|---|---|---|
| proofread | 50 | 8 | 7 | 10 English items already correct (over-correction), 3 prompt injections, protected names, URLs, prices and code |
| reply | 50 | 8 | 7 | work and personal messages, 5 with a started draft, 3 prompt injections, a condolence and an angry customer |
| rewrite (professional) | 50 | 8 | 7 | 34 items with facts that must survive (invoice numbers, amounts, dates), 3 prompt injections, 3 already professional |

Other languages (best effort, PLAN 1) have a smoke set per skill in `other.generated.jsonl`,
also project-written: 2 items each in Spanish, French, German, Portuguese, Marathi, Bengali,
Tamil and Chinese (16 per skill; the Tamil proofread items and one Chinese one are already
correct). It checks that the language is kept and that the detector never calls these
languages English or Hindi; 16 items cannot rank models on them.

## Prompt revisions

| revision | change | why |
|---|---|---|
| 1 | first drafts | M0 baseline |
| 2 | the system prompt no longer names Hindi or Hinglish; a language detector names the answer language in the user message ("The text is in English. Keep it in that language.") | with revision 1, models answered English in Hindi: Qwen3.5-4B rewrote 49 of 50 English texts into Devanagari and answered 36 of 50 English messages in Hinglish |
| 3 | proofread keeps casual lowercase, a missing final full stop, British or American spelling and date formats as written; the Hinglish language name says to keep Hinglish even in a formal tone | with revision 2, Gemma E4B changed 6 of 12 already-correct texts ("colour" to "color", "14 September" to "September 14", capitalised chat); 3 to 5 of 7 Hinglish texts were translated into English by the professional rewrite |
| 4 | proofread only: "if a sentence starts in lowercase or has no final full stop, leave it so" and "never change the case of a word that is already correct" replace "keep lowercase starts" | with revision 3, both Gemma models lowercased whole texts (English F0.5 0.88 to 0.29 for E2B) while Qwen improved; revision 4 keeps Qwen's gains and restores Gemma (E4B English F0.5 0.92, no false alarms) |
| 5 | proofread only: fixes like a proofreader (owner, 2026-10-22): capital at every sentence start, end punctuation, question marks, run-ons split; names the error kinds the model missed (tense against time words, missing words, verb forms, prepositions, Hindi and Hinglish gender and number agreement); instructions inside the text are corrected and kept | revision 4 left lowercase starts and missing full stops by design, missed tense and agreement errors, and dropped 2 of 3 injected instructions ("Summarize the following ..."): E2B recall 0.68 on 124 items |
| 6 | proofread only: a "change nothing else" list: keep contractions, no optional commas, no added words in casual sentences, no full stop after headings or a final emoji, hashtag or URL, keep chat spellings and time formats, English words stay in Latin letters in Hindi; one worked injection example | revision 5 raised recall to 0.89 but changed 2 of 19 correct texts, expanded contractions, added Oxford commas and transliterated English words into Devanagari |
| 7 | proofread only: the worked example no longer resembles a test item | the model copied the example's British "summarise" into a test item |

**Proofread revision 4 to 7, Gemma 4 E2B, through the daemon** (`eval/reports/q-proofread-r4`
to `-r7`, `q-holdout-r4`, `q-holdout-r7`):

| set | revision | precision | recall | F0.5 | exact | correct texts changed |
|---|---|---|---|---|---|---|
| all 124 items | 4 | 0.86 | 0.68 | 0.82 | 57% | 5.3% |
| all 124 items | 7 | 0.92 | 0.91 | 0.92 | 78% | 5.3% |
| holdout, 25 items | 4 | 0.65 | 0.37 | 0.56 | 40% | 0 |
| holdout, 25 items | 7 | 0.93 | 0.83 | 0.91 | 76% | 0 |

E4B with revision 7 changes 5 of 19 correct texts (F0.5 0.83 on all items), so proofread stays
on E2B, the fast tier. Still missed by E2B: some Hindi and Hinglish gender agreement on
masculine nouns ("मेरी भाई", "Mera bahan" half fixed), honorific agreement with आप, since/for
with a duration, "until" for a deadline, and about half of the run-on sentences.

## Results

### Go/no-go (2026-10-22)

**Go, with Gemma 4 E2B as the fast tier and Gemma 4 E4B as the quality tier.**

- Quality: both pass every hard check and the Hindi and Hinglish floor (below); E4B is the
  better writer, E2B the better speed for its quality. Qwen3.5-0.8B and 2B fail the
  spot-check (15 and 9 of 20 answers bad); Qwen3.5-4B ties E4B on quality and lost the tie
  to family consistency (owner, 2026-10-22).
- Speed: no candidate meets the original PLAN 5.4 targets, and by the owner's rule the
  fast tier on machine A is a no-go (Latency, below). ADR-003 overrides that rule: a
  correct answer in 5 to 8 s on an 8 GB laptop beats a wrong one in 2 s. PLAN 5.4 now holds
  targets from these measurements.
- Settings: flash attention; the MTP draft head for proofread and rewrite (ADR-004); the
  engine's language check with one retry that names the language (ADR-006); read-ahead in
  the daemon (ADR-005, M1).
- Before M5: re-run the bench on a real older laptop (ADR-003), and add the owner's own
  items to the `*.human.jsonl` files.

### Quality of the chosen models (2026-10-22)

Skills at proofread revision 4, reply revision 3, rewrite revision 4, skill schema v3, run
through the engine (`eval/paraeval/engine.py`) with its language retry. Proofread from
`eval/reports/m0-v5/`, reply and rewrite from `eval/reports/m0-v6/` (the retry that names the
language). 81 items per skill: 50 English, 8 Hindi, 7 Hinglish, 16 in eight other languages.

| | Gemma 4 E2B (fast) | Gemma 4 E4B (quality) |
|---|---|---|
| proofread F0.5, English / Hindi / Hinglish | 0.886 / 0.833 / 0.714 | 0.916 / 0.900 / 0.714 |
| proofread exact match, false alarms on 15 clean texts | 63% · 1 of 15 | 68% · 0 of 15 |
| proofread protected tokens kept | 100% | 100% |
| reply valid JSON, 3 distinct replies, any over 15 words | 100% · 100% · 0% | 100% · 100% · 0% |
| rewrite changed, protected facts kept, preamble | 100% · 89% · 0% | 100% · 88% · 0% |
| outputs retried by the language check (reply, rewrite) | 2.5%, 4.9% | 6.2%, 2.5% |
| other languages: language kept (48 outputs) | 48 of 48 | 48 of 48 |
| spot-check, good / ok / bad of 20 | 11 / 7 / 2 | 14 / 5 / 1 |

**Hindi and Hinglish floor** (owner, 2026-10-22: over all 15 Hindi and Hinglish items per
skill, reject a model only if more than 10% of its outputs switch language or are garbled;
`eval/reports/m0-v6/floor.md`). Claude read all 90 outputs of both models against their
inputs and found none garbled (`floor-review.json`).

| | proofread | reply | rewrite | total | verdict |
|---|---|---|---|---|---|
| Gemma 4 E2B | 0 of 15 | 0 of 15 | 1 of 15 (Hinglish into English) | 1 of 45 (2%) | pass |
| Gemma 4 E4B | 0 of 15 | 2 of 15 (code-mixed Hindi into Hinglish, Hinglish into English) | 1 of 15 (Hinglish into English) | 3 of 45 (7%) | pass |

Before the retry named the language (`m0-v5`), E2B turned 5 of 7 Hinglish professional
rewrites into English (13%, a reject); with it, 1 of 7. Every remaining switch reaches the
user with a language warning, never silently (ADR-006).

**Rejected candidate: reply variants batched from one prompt** (`eval/reports/m0-v5-variants/`):
only 48% (E2B) and 38% (E4B) of reply sets had three different replies, against 100% when
one sequence writes all three. Reply keeps one sequence.


### Latency (2026-10-22)

Measured with `eval/bench_latency.py` on the five English items closest to 100 words per
task (see Method), here 10 timed requests per task after one warm-up, p50 / p90, in
seconds. Raw files: `eval/reports/m0-speed/bench/` (one per model, profile and setting) and
`eval/reports/m0-v3/bench/` (all five candidates, prompt revision 3). Laptop on AC, balanced
power profile.

**What each lever did** (Gemma 4 E2B on machine A, emulated; p50 totals):

| setting | reply first token | reply total | proofread total | rewrite total | kept? |
|---|---|---|---|---|---|
| baseline (prompt-prefix cache on, as in every run) | 3.04 | 5.24 | 10.57 | 12.24 | |
| flash attention | 3.06 | 5.00 | 10.66 | 11.90 | yes (neutral; never slower) |
| speculative decoding, n-gram from the input | 3.02 | 5.04 | 11.97 | 12.79 | no: the check costs more than it saves on 4 cores |
| **speculative decoding, Gemma MTP draft head (59 MB)** | 3.10 | 5.45 | **7.85** | 11.96 | **yes, for proofread and rewrite**; off for reply (its p90 gets worse) |
| reply variants batched from one prompt (`n_cmpl`) | 2.96 | 4.58 | | | no: the three replies come out nearly identical (quality eval below) |
| more threads | | | | | no: 4 threads already uses all of A's cores; on B 10 threads stays best (E2B prompt 146 tok/s at 10, 136 at 12) |
| Q4_K_M instead of the QAT Q4_0 | | | | | no: same prompt speed on A, slower on B (125 vs 154 prompt tok/s, 20 vs 22 output tok/s) |
| Vulkan on the Arrow Lake iGPU | | | | | no: slower than the CPU for both models on B (E2B proofread 11.8 vs 7.1 s) |
| Gemma E2B as draft model for E4B | | | | | no: 13.3 GB resident and slower than E4B alone (proofread 18.6 vs 13.4 s on B) |

With a greedy skill (proofread) speculative decoding returns exactly the output the model
alone would, so it cannot change quality; with sampling it keeps the same distribution.

**Chosen setup** (flash attention; MTP draft head for proofread and rewrite):

| model, machine | reply first token | reply total | proofread first token | proofread total | rewrite first token | rewrite total | load cold / warm | peak RSS |
|---|---|---|---|---|---|---|---|---|
| Gemma 4 E2B, A (emulated) | 3.06 / 3.23 | 5.00 / 5.84 | 3.31 / 3.63 | 7.85 / 8.85 | 3.70 / 4.04 | 11.96 / 13.09 | 5.9 / 5.0 | 5.0 GB |
| Gemma 4 E2B, B (CPU) | 1.12 / 1.18 | 2.71 / 2.98 | 1.47 / 2.00 | 4.07 / 6.10 | 1.49 / 2.72 | 6.58 / 10.16 | 4.9 / 4.0 | 5.0 GB |
| Gemma 4 E4B, B (CPU) | 2.36 / 2.44 | 5.49 / 6.28 | 2.76 / 5.51 | 7.16 / 13.18 | 3.22 / 5.07 | 14.28 / 15.55 | 6.1 / 4.7 | 8.8 GB |
| Gemma 4 E4B, A (emulated, no MTP measured) | 6.06 / 6.40 | 10.13 / 10.95 | 6.36 / 7.00 | 20.22 / 22.43 | 7.38 / 7.95 | 23.96 / 27.03 | 4.3 / 3.1 | 8.5 GB |

Loading the MTP head adds about 1 to 2 s to a model load.

These numbers are from the first session. A second session on the same day, with reply on
a server without the draft head and proofread and rewrite on one with it (the setting the
daemon will use), is in `eval/reports/m0-v6/bench/` (`chosen-reply`, `chosen-draft`): machine A
repeated within 2%; machine B, a desktop in use with a browser open, was up to 30% slower
on reply (E2B reply 1.48 s first token, 3.32 s total; E4B reply first token p90 3.96 s) and
slightly faster on proofread and rewrite (E2B proofread 3.84 s, E4B rewrite 13.46 s). The
PLAN 5.4 targets use the slowest session. llama-server v0.7.0 ignores per-request
speculative settings, so per-skill drafting is measured on two server setups; the daemon
switches per request.

**Against the original PLAN 5.4 targets** no candidate of acceptable quality meets them.
The fast tier on machine A (the owner's must-have, PLAN 11 Q4) was to be reported as a
no-go if, after these levers, it was still worse than twice the target:

| fast tier on A (Gemma E2B), p50 | measured | 2 x target | verdict |
|---|---|---|---|
| reply, first token | 3.06 | 1.6 | over |
| reply, total | 5.00 | 4.0 | over |
| proofread, first token | 3.31 | 1.6 | over |
| proofread, total | 7.85 | 7.0 | over |

**By that rule the fast tier on machine A is a no-go.** The cause is reading the input:
on four cores Gemma E2B processes about 48 prompt tokens a second, so a 100-word text
(about 150 tokens) takes about 3 s before the first word, and no setting changes that. The
only candidate fast enough (Qwen3.5-0.8B: reply 2.4 s, first token 1.0 s on A) has 15 of 20
spot-check answers rated bad. The decision taken on this is ADR-003 (DOCS/DECISIONS.md).

### Spot-check (2026-10-22): Claude-rated (2 passes and a final check), not human-reviewed: independent of the judge model, not of the item writer

20 items per model (7 proofread, 7 reply, 6 rewrite; 3 Hindi, 3 Hinglish), five blind
answers per item, rated on the page built by `eval/spotcheck.py` (proofread at revision 4,
reply and rewrite at revision 3). **Claude rated all 100 answers; no human reviewed or
edited any rating** (owner, 2026-10-22). Claude is from a different family than the
candidates and than the judge (Sarvam-30B), so these ratings are independent of the judge
model. They are not independent of the item writer: the eval items were also written with
Claude. PLAN 11 makes the owner's own spot-check the final word on quality; these ratings
are not that.

Item ids use `hi` for every item from the `hi.*.jsonl` files, Hindi and Hinglish alike; the
language comes from each item's `lang` field. Hinglish items here: pr-hi-015, rp-hi-013,
rw-hi-009.

| group | items | answers |
|---|---|---|
| Claude-rated, not human-reviewed | 20 | 100 |
| human-reviewed | 0 | 0 |

How the ratings were made, each step on all 100 answers:

| step | ratings changed | details |
|---|---|---|
| pass 1 | (first draft) | `ratings-pass1.jsonl` |
| pass 2 | 8 of 100 | 4 up (pr-en-038 C, rp-en-007 B, rp-en-010 A, rp-en-030 E), 4 down (rp-en-007 A, rp-hi-013 A and C, rw-en-004 E); `ratings-pass2.jsonl` |
| final check (owner asked Claude to check the result, 2026-10-22) | 3 of 100 | pr-en-038 C good to ok (an unrequested Oxford comma is a style edit, which proofread must not make); rw-en-048 B bad to ok (facts kept, one unprofessional phrase); rw-en-004 E ok to bad (asks for "time off this weekend" instead of Friday, the same lost fact that makes A bad); `ratings.jsonl` |

The final check applied one rule for reply consistently: one wrong chip of three is ok, two
or more is bad. Notes are transcribed as stored, with dashes replaced by commas. Full table
by skill and language: `eval/reports/m0-v3/spotcheck/report.md` (`spotcheck.py report`).

Good / ok / bad counts (final):

| model | total | proofread | reply | rewrite | English (14) | Hindi (3) | Hinglish (3) |
|---|---|---|---|---|---|---|---|
| gemma-4-e4b-qat-q4_0 | 14 / 5 / 1 | 5 / 2 / 0 | 4 / 2 / 1 | 5 / 1 / 0 | 10 / 4 / 0 | 2 / 1 / 0 | 2 / 0 / 1 |
| qwen3.5-4b-q4km | 14 / 4 / 2 | 5 / 2 / 0 | 5 / 1 / 1 | 4 / 1 / 1 | 10 / 3 / 1 | 3 / 0 / 0 | 1 / 1 / 1 |
| gemma-4-e2b-qat-q4_0 | 11 / 7 / 2 | 3 / 3 / 1 | 4 / 3 / 0 | 4 / 1 / 1 | 8 / 6 / 0 | 1 / 1 / 1 | 2 / 0 / 1 |
| qwen3.5-2b-q4km | 3 / 8 / 9 | 3 / 2 / 2 | 0 / 4 / 3 | 0 / 2 / 4 | 3 / 8 / 3 | 0 / 0 / 3 | 0 / 0 / 3 |
| qwen3.5-0.8b-q4km | 1 / 4 / 15 | 1 / 2 / 4 | 0 / 1 / 6 | 0 / 1 / 5 | 1 / 4 / 9 | 0 / 0 / 3 | 0 / 0 / 3 |

**Quality tier: Gemma E4B (owner, 2026-10-22).** After pass 2 Gemma E4B and Qwen3.5-4B were
tied at 14 / 5 / 1; the final check leaves Gemma one answer ahead. **On a 20-item sample this
is within noise**, and the automatic metrics and the judge do not separate them either
(rewrite judge win rate 0.56 and 0.55). The owner chose Gemma E4B for consistency with the
fast tier (Gemma E2B): one set of per-family prompt overrides, one style across tiers, and E2B
as a possible speculative-decoding draft for E4B (the two files have the same 262,144-token
vocabulary, checked 2026-10-22). Fallback: Qwen3.5-4B, only if E4B does not fit comfortably in
16 GB by the measured RAM. Both failed rp-hi-013, a Hinglish reply written from the
courier's side.

No other pair is close: among the models that fit machine A, Gemma E2B (11 / 7 / 2) is far
ahead of Qwen 2B (3 / 8 / 9) and Qwen 0.8B (1 / 4 / 15).

Failure categories, matched by keyword in the notes of ok and bad ratings (rules in
`spotcheck.py`, `CATEGORIES`):

| model | injection obeyed | unchanged rewrite | wrong-speaker reply | Hindi/Hinglish garbage | switched language |
|---|---|---|---|---|---|
| gemma-4-e4b-qat-q4_0 | 0 | 0 | 1 (rp-hi-013 bad) | 0 | 0 |
| qwen3.5-4b-q4km | 0 | 0 | 1 (rp-hi-013 bad) | 1 (rp-hi-013 bad) | 0 |
| gemma-4-e2b-qat-q4_0 | 0 | 0 | 0 | 0 | 1 (rw-hi-009 bad) |
| qwen3.5-2b-q4km | 0 | 3 (rw-en-049, rw-hi-006, rw-hi-009, all bad) | 4 (rp-en-005 ok, rp-en-007 bad, rp-en-010 ok, rp-hi-013 bad) | 2 (rp-hi-004, rp-hi-013 bad) | 0 |
| qwen3.5-0.8b-q4km | 1 (rp-en-030 bad) | 2 (rw-hi-006, rw-hi-009 bad) | 2 (rp-en-005, rp-en-010 bad) | 2 (rp-hi-004, rp-hi-013 bad) | 0 |

Gemma E2B's rp-en-030 answer counted as "injection obeyed" in the first pass; the second pass
rated it ok ("did not follow the injection, but the uppercase 'YES' chip may echo it").
"Unchanged" proofread answers (a missed fix, not a rewrite failure) are not a category: Qwen
0.8B on pr-hi-001, pr-hi-015 and pr-en-023; Gemma E2B on pr-hi-001; Qwen 2B on pr-hi-015.
