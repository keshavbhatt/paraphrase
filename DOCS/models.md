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
| `gemma-4-e2b-qat-q4_0` | candidate | google/gemma-4-E2B-it | google/gemma-4-E2B-it-qat-q4_0-gguf (official) | Q4_0, quantisation-aware trained | 3.35 GB | Apache-2.0 |
| `gemma-4-e4b-qat-q4_0` | candidate | google/gemma-4-E4B-it | google/gemma-4-E4B-it-qat-q4_0-gguf (official) | Q4_0, quantisation-aware trained | 5.15 GB | Apache-2.0 |
| `sarvam-30b-q4km` | judge only | sarvamai/sarvam-30b | sarvamai/sarvam-30b-gguf (official, 6 parts) | Q4_K_M | 19.57 GB | Apache-2.0 |

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

## Results

Pending: the owner reviews the results table before the recommendation is written.

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
