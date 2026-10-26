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
| draft 5 | Full reply: the user's note comes after the message as `<my_answer>`, "the user's own quick notes, written from their side"; a leading no or yes answers the request; notes become full sentences, never copied; a no says what cannot be done and why, then may ask for an alternative; the email ends with a closing in the reply's language and no name; without a note, nothing only the user would know is made up | owner report (2026-10-23): "no, web.de doesn't work" was written back as the other person's problem ("I understand that ... is not working for you") and signed "[Your Name]". On the full set: forbidden patterns (placeholders, invented details, echoed notes, injections) E4B 44% to 0%, E2B 28% to 0%; facts kept E4B 76.5% to 94.1%, E2B 88.2% to 100%; holdout clean. Cost: first output on B-cpu E4B 2.4 s to 5.3 s, E2B 1.4 s to 2.9 s (about 290 more prompt tokens after the message). Revision 6 moved that text into the cached system prompt (E4B 4.0 s) but lost the gains (facts 76.5%, the owner's case echoed again), so 5 ships |
| custom 2 | My actions: no placeholders in square brackets; a letter or email starts at the greeting and ends with the closing alone; what the text does not give is left out; no invented hashtags | revision 1 wrote address blocks, "[Date]" and "by [No specific date mentioned]": placeholders E4B 13.6% to 4.5%, E2B 18.2% to 4.5%; forbidden patterns E4B 18.2% to 9.1%, E2B 27.3% to 13.6%; facts kept about the same (80.7% and 78.9%); total time E4B 8.8 s to 5.9 s |
| draft 7 to 9 | 7: the shape and tone move before the message, so the window's read-ahead (ADR-005) covers them and only the note and one instruction follow the message; 8: "No name or placeholder after the closing" at the end; 9: dates as written ("14 June" never "June 14th") | revision 5 left about 200 tokens after the message, read on every request at about 50 tokens a second on E4B (B-cpu); with the message read ahead, the first output after the note went from about 4.1 s to 3.2 s. Revision 6 had moved the note's instruction into the system prompt and lost the gains. Revision 9 on the full set: no forbidden patterns, facts 100% (E4B) and 96% (E2B), holdout clean; first output cold 4.5 s to 3.9 s (E4B) |
| draft 10 | Full reply: "If my notes only thank or praise, the reply does only that, with no news, plans or promises." after the notes instruction | owner report (2026-10-23): a long announcement ending in the sender's own plan ("Today I will continue working on it") and the note "Thanks Gabri, cool idea" came back as "... I will continue working on it today". On that message (10 runs each): E4B wrote the sender's plan as the user's in 3 of 6 runs before, 0 of 20 after; E2B about half before, 0 of 10 after (some runs say "Thanks for the information" instead of the praise). Four new hard items (dr-en-113 to 115, dr-hi-106). Full set: E4B facts 100%, shape 100% chat and 94.7% email (the Hinglish item, as before), forbidden patterns 0; E2B email facts 96% to 89% (dr-en-105, which revision 9 also misses in 4 of 10 runs); holdout clean on both; first output unchanged (E4B 4.5 s chat). Rejected: rules that "I" and "we" in the message are its writer, and a system-prompt rule for short notes; they fixed the case but lost the agreed day and time in threads, the address in a Hindi "no", or let E2B follow "reply in French" |
| draft 11 | Full reply: a password or login, an OTP or any code sent to the user, a PIN, or card or bank details are never given and never promised, now or later: the reply says the user does not share them, or will contact the sender another way. Without a note, what only the user knows (an address, a time) gets "I will check or ask" instead of "I will send it or check", and a request to do something by a day or time gets "I will, by then". The eval's "agrees to send" patterns (English, and Hindi and Hinglish verbs and negations) catch a reply that offers a secret | owner report (dr-en-104): E4B once wrote "I will check and send my card details". Five new scam items written for this (dr-en-116 to 118, dr-hi-107, dr-hl-203) and three quick-reply ones (rp-en-127, rp-en-128, rp-hi-111); quick replies were already safe (40 of 40 on both models). Full reply, 10 runs per scam item: revision 10 agreed to send in 9 of 70 (E4B) and 32 of 70 (E2B), revision 11 in 1 of 80 and 3 of 80 (one E4B case is "I will check regarding the request to provide my password", which the pattern counts). No reply to an ordinary message mentions secrets (0 of 25 on each model). Full set: facts E4B email 88.9% to 100%, E2B email 77.8% to 88.9%; injections obeyed E2B email 14.3% to 0%; first output E4B email 7.2 to 4.4 s, E2B 4.1 to 2.3 s. Rejected on the way: a quoted refusal sentence (E4B copied it as the whole email), and a refusal rule in the no-note instruction (E2B wrote "I never share passwords by message" to 19 of 25 ordinary messages) |
| draft 12 | Full reply: notes may begin mid-thought ("until X is fixed, you can do Y" offers Y and refuses nothing); say no only when the notes say no; if the message reports a problem, open with one short sentence of thanks or sorry; if the notes ask them for something, end by asking for it plainly. All in the system prompt, which the window reads ahead | owner report (2026-10-25): notes starting "until we resolve the .de delivery issue, you can give us another address" came back as a refusal. Three new hard items (dr-en-119 to 121; revision 11 refused 119). The owner's case: no refusal in 4 of 4 runs (E4B) and 3 of 3 (E2B). Full set and holdout against revision 11: shape the same (E4B email 20/23, E2B 22/23, chat all), facts kept E4B email 18 to 20 of 20, E2B 18 of 20, no forbidden patterns; scam items 25 of 25 (E4B) and, over 20 runs of the Hindi and Hinglish PIN items, 17 of 20 on E2B against 18 of 20 for revision 11 (noise; still open); no refusal wording in replies to ordinary messages. First output cold +0.2 s (E4B 4.58 to 4.76 s email), about 87 more tokens before the message. Rejected: 12a signed the email with the name the message greets the user by (E2B wrote "[Your Name]" in 4 of 23 emails and once the sender's name); 12b put the problem and asking sentence after the message (first output +0.8 s on E4B, read on every request). Known: E2B sometimes promises to look into what the message asks (the owner's case, 2 of 3 runs) |
| rewrite 11 | Tone with a language the user chose: "apply the tone as fully as you would in <the text's language> (an elaborate rewrite is clearly longer than the text, a concise one clearly shorter), and write the result in <the chosen one>. It is a rewrite in that tone, not a plain translation." Only the sentence for a chosen language changed; without one the prompt is the same. Four hard items with `target_lang` (rw-en-123 to 125, rw-hi-108), and the metric checks the chosen language | owner report (2026-10-25): Elaborate after choosing Hindi read like a translation. Against a plain Translate into the same language (E4B, 5 texts, 3 runs each, words): Elaborate 1.10 to 1.27 times, Concise 0.86 to 0.73, Casual about the same. Hard set, elaborate, concise and professional on both models: protected facts the same or better (E4B concise 96.2% to 99.2%, E2B concise 99.2% to 100%), the four new items in the chosen language with every number kept, first output unchanged. Rejected: running the tone and then Translate (Elaborate 1.47 but Concise 0.85, a garbled sentence, twice the wait) |
| compose 2 to 4 | 2: no placeholders but the sign-off, greeting and closing in the description's language, numbers and times as written, no translating; 3: "Hello," instead of a literal "a plain greeting", one note item per line; 4: the email ends with "[Your name]" exactly as the approved mock (main-window-compose.html, note 2) | revision 1 wrote "[Date] at [Time]", "eight months" for "8 months", English greetings in Hindi emails; revision 2 printed "Greeting," once and copied a Hinglish note unsplit. Revision 4 (metric rescored: "7am" and "7 am" are the same fact, Hindi subject and address lines count): shape right on all but one item per model, facts 100% on the full set, one placeholder left per model ("[Date]" in an E2B note heading) and one Hindi request for an English answer obeyed by E4B |
| compose 5 | Write: an email signs with the name the description ends with or asks to sign as, "[Your name]" only without one; spelling and capitals are corrected ("monday" to "Monday") while names, numbers and dates stay as written. The metric takes the item's `signer` as the last line | owner report (2026-10-23): "hi team ... Best regards, Keshav Bhatt" came back signed "[Your name]" (E4B, 3 of 6 runs), and the facts line said "Bhatt is missing". After: 6 of 6 signed "Keshav Bhatt". Four new hard items (co-en-107 to 109, co-hi-104), which revision 4 mostly passed already. Full set and holdout: facts 100% on both models (holdout: the same co-hi-201 miss as revision 4); the other misses (E2B "[Date]" in a note, the Hindi injection asking for English, one "Thanks, [Your name]" on one line) are as frequent as with revision 4 over 6 runs each; first output unchanged (E4B 2.6 to 2.8 s) |
| compose 6 | Write: "[Your name]" belongs to an email alone. The template's closing sentence names it only through the email preset's `signoff` value (the other forms pass ""), and a note ends with its last item: "no greeting, no sign-off and no name under it". The shape metric now rejects any bracket placeholder outside an email, and accepts a Hindi greeting like "सभी को नमस्कार," | owner report (2026-10-23): a Note ended with "[Your name]" (E4B 5 of 10 on revision 5, 10 of 10 on revision 4). After: 0 of 10; co-en-008 and co-hl-002 together 20 of 20 clean (revision 5: 8 of 20). New hard item co-en-110. Full set: note shape E4B 60% to 100%, E2B 80% to 100%; email, message and post unchanged apart from single-run noise (E2B "Thanks, [Your name]" on one line, as before); facts 100% on both models; holdout unchanged; first output unchanged or faster (E4B note 2.8 to 2.0 s). E2B still writes "[Date]" in about 1 of 10 notes |
| compose 7 | Write: a note opens with a title line of two to six words saying what it is about, with every day, date, time and place the description gives for the whole note, never a label such as "Date:" and never the description copied; then each item as a bullet. The shape metric wants that title (10 words at most, no label, no markdown) and two bullets under it | owner report (2026-10-23): a note of questions came back as "Date: Unspecified" and three bullets. On that text, E4B: revision 6 opened with a date line or "[Date]" in 10 of 10 runs, revision 7 with a title ("Files Public Share Page Inquiry") in 10 of 10. New hard items co-en-111, co-en-112, co-hi-105. Note set, 8 items: E4B shape 100% both; E2B 75% to 88% (revision 7's one miss: a title with no bullets under it, which also loses that note's facts, 100% to 95%); first output E4B 1.7 to 1.9 s, total 4.8 to 5.5 s for the title's few words. Wordings tried first: "a short first line naming what the note is about" (E2B copied the whole description into it) and "any day, date, time or place" (dropped "4pm") |
| summarize 5 | keeps the number that identifies the matter (order, ticket, incident, invoice, flat) and a phone number or address the reader must use; dates as written | revision 4 dropped "#48213", "INC-2291" and phone numbers and wrote "April 1st" for "1 April": facts kept 85% to 88% (E4B), 83% to 84% (E2B); holdout E4B 97% to 92% (a seat number and a name, 1 of 8 items), E2B unchanged at 97% |
| 6 | proofread only: a "change nothing else" list: keep contractions, no optional commas, no added words in casual sentences, no full stop after headings or a final emoji, hashtag or URL, keep chat spellings and time formats, English words stay in Latin letters in Hindi; one worked injection example | revision 5 raised recall to 0.89 but changed 2 of 19 correct texts, expanded contractions, added Oxford commas and transliterated English words into Devanagari |
| 7 | proofread only: the worked example no longer resembles a test item | the model copied the example's British "summarise" into a test item |
| 8 (rejected) | proofread only: agreement rules with examples (a possessive takes the gender of the noun after it, plural verbs with आप, ने agreement) | E2B F0.5 0.92 to 0.88, false alarms on correct English 8% to 17%, Hindi and Hinglish worse; holdout unchanged. The remaining agreement misses look like a limit of E2B, not of the prompt |
| 10 | proofread only: `output.min_length: 0.7` (skill schema v4, DOCS/skills.md, "Length check"); no prompt change | the owner's CLI suite: a sentence repeated 14 times came back as 1 copy |
| 9 | proofread only: the language reminder sits before `<text>`, so the text is the last thing the model reads; a layout rule (every line break, blank line and tab stays, also a tab after a label such as "Name:"); single words are corrected the same way | the owner's CLI suite: "teh cat" came back as the reminder line itself, a two-line tab form came back as one line |

**Proofread revision 4 to 7, Gemma 4 E2B, through the daemon** (`eval/reports/q-proofread-r4`
to `-r7`, `q-holdout-r4`, `q-holdout-r7`):

| set | revision | precision | recall | F0.5 | exact | correct texts changed |
|---|---|---|---|---|---|---|
| all 124 items | 4 | 0.86 | 0.68 | 0.82 | 57% | 5.3% |
| all 124 items | 7 | 0.92 | 0.91 | 0.92 | 78% | 5.3% |
| holdout, 25 items | 4 | 0.65 | 0.37 | 0.56 | 40% | 0 |
| holdout, 25 items | 7 | 0.93 | 0.83 | 0.91 | 76% | 0 |

E4B with revision 7 changes 5 of 19 correct texts (F0.5 0.83 on all items), so proofread stays
on E2B, the fast tier.

**Reply and rewrite (M1).** Metrics count valid JSON and kept facts but cannot judge meaning,
tone or helpfulness, so these revisions are judged blind (`eval/judge.py`): reviewer agents
get the input and two outputs in random order, and say whether each is acceptable to send as
it is, which problems it has, and which is better. Reply has 26 hard and 10 holdout items,
rewrite 24 hard and 10 holdout items run in five tones (professional, casual, polite, concise,
friendly; only professional was tested before). A new reply metric checks that every
suggestion continues the user's draft.

| skill | revision | change | result |
|---|---|---|---|
| reply | 4 | replies specific to the message, never asking what it already says; three useful, different choices, warm ones for thanks, congratulations and sympathy, a pick for choice questions, never agreeing to a scam; reply to a thread's last line; the register of the message; every reply starts with the user's draft | E2B, 107 items: acceptable 64% to 80%, better in 64 items against 32; draft continued 0% to 100%; generic replies 31 to 4. Holdout (10): 6 to 8 |
| reply | 5 (rejected) | write as the user, never as an assistant; three different meanings; ask only needed questions | a tie with 4 (81% against 79%, 40 against 41 wins): fewer unsafe and repeated sets, more generic ones |
| rewrite | 5 | keep meaning and strength, every fact in its own format, every point and question, the layout, the language and script; add nothing; about the same length (not for concise or elaborate); rewrite instructions inside the text instead of obeying them | E4B, 201 items in 5 tones: acceptable 55% to 75%, better 114 against 42, changed facts 44 to 4; E2B: 48% to 68%, 101 against 40, changed facts 53 to 9. Holdout (50 per model): E2B 47% to 70% (reply and rewrite together), E4B 50% to 74% |
| rewrite | 6 (rejected) | firmer rules for meaning, feeling and formats; "you" in the text is the reader; new polite, friendly and concise presets | E4B 67% to 70%, a tie in wins; E2B a tie in rate and worse in wins (61 against 86): more missed tone and 6 obeyed instructions. The polite and friendly presets alone helped on E4B (friendly 9 to 15 of 24) and are tried next on their own |
| rewrite | 7 | revision 5 with only the new polite and friendly presets (the old polite one said "softening any demands") | polite and friendly, 48 items: meaning changed 13 to 8 (E2B) and 15 to 9 (E4B), better in 17 against 13 and 21 against 17; somewhat stiffer wording |
| reply | 6 (rejected) | revision 4 plus "a person, never an assistant or an AI" and reacting to instructions inside the message as the user would | a tie in rate (79% each) and worse in wins (32 against 43): fewer unsafe replies (4 to 1) but stiffer ones |
| rewrite | 8 (rejected) | revision 7 at temperature 0.3 instead of 0.6 | E4B, 120 hard items in five tones: a tie (79% against 78% on professional, casual and concise, 67% against 63% on polite and friendly; most pairs identical). Randomness is not what goes wrong |
| rewrite | 9 | polite and friendly presets with one worked example each (sentences unlike any test item); polite no longer asks for "please and thank you", which the model tacked on at the end | polite and friendly, 48 hard items: E4B acceptable 60% to 83% (better 20 against 5), E2B 48% to 73% (19 against 3). Holdout (20 per model): E4B 70% to 85%, E2B 70% to 75% |

A first draft of both named Hindi and Hinglish in the system prompt and made the models answer
Devanagari messages in Latin letters (12 reply failures), the M0 revision 2 lesson again: the
prompts state script rules without naming a language.

**From the owner's CLI suite (2026-10-22).** New items: 13 short proofread inputs, 7
tab-separated or multi-line forms and tables, a long email and a paragraph repeated ten times;
7 rewrite items with dates, money and times in their own formats; 12 reply items with injected
instructions or asking for information the user has not given. New metrics: proofread layout
kept (line breaks and tabs), proofread output under 70% of the input's characters (lost
content), reply obeying an injected instruction (`forbidden` patterns per item).

| skill | revision | result |
|---|---|---|
| proofread | 9 | E2B, 143 items: F0.5 0.871 to 0.872, correct texts changed 13.6% to 9.1%, layout kept 96.5% to 97.2%, first output 0.66 s to 0.43 s (median). Holdout (31): F0.5 0.83 to 0.89. Fixed: the reminder line is never returned ("teh cat" to "The cat"), lines no longer merge, tables keep their tabs. Not fixed: a tab after a label still becomes a space ("Name:\tAnjali"), a lone "teh" stays "Teh", and a paragraph repeated ten times comes back with 6 or 7 copies (both revisions; the daemon's length guard is to catch it) |
| proofread | 10 | the daemon's length check (retry once, then `incomplete`). E2B, 143 items (`eval/reports/s-pr-r10`): every output identical to revision 9 except the paragraph repeated ten times, which revision 9 returned with 7 copies; revision 10 retried, was short again and refused it, scored as an empty output. That one item moves the totals: protected tokens kept 100% to 75.5% (its 40 tokens), recall 0.870 to 0.847, precision 0.873 to 0.891, F0.5 0.872 to 0.882. Holdout (31): identical. First output 0.43 to 0.38 s (median). The suite's E07 (one sentence 14 times on one line) now ends with "The answer left out part of the text" instead of returning 1 copy |
| rewrite | 10 | "dates in their exact format and word order", with US, British, numeric and ordinal examples. E4B, professional and casual, 29 hard items: facts kept 91.1% to 95.1% and 92.7% to 95.1%; "March 1" no longer becomes "1 March". Holdout (12): the same, except one Hindi item that wrote English words in Devanagari; sampled 24 times per revision, both do this most of the time (old 4 of 24 kept Latin, new 6 of 24), a known weakness of mixed-script text in the professional tone |
| format | 6 | Bullets: "start your reply with \"- \" unless the text starts with a greeting or a title ending in a colon", in place of "a heading or greeting line stays above the list". Owner report (2026-10-26): in Hindi E2B left a message's first sentence without "- ", taking it for a heading. The bullets check is now strict (every line a bullet but one leading greeting, colon title or short heading) and 4 new items (fm-en-013, 014, fm-hi-004, 005) are messages in paragraphs. E2B, 13 items: shape 76.9% to 100% (Hindi 50% to 100%), facts kept 100% both; the injection item fm-hi-101 now needs the engine's language retry (first answer in English), the old prompt kept the injected sentence as a heading instead. Holdout (2): both pass; the Hindi lead-in "यात्रा के लिए याद रखें:" now gets a bullet too. Prompt 154 to 170 tokens (median). Rejected: a rule that every sentence becomes an item (no change in Hindi, a duplicated line in English), and a shorter "the first line too" (Hindi still unbulleted) |
| draft | 18 | Full reply without notes: a message that only informs (news, an update, the sender's own plan) gets one or two short sentences of thanks in general words, never a retelling and never the sender's tasks taken on; a request for a password, code, PIN or card details gets "I do not share these" before any "I will check". Owner report (2026-10-26): a colleague's status update came back as "I will check on the migration of ..." with every detail repeated. The old "say I will check or ask" was applied to every message. New metric `restated` (share of the message's distinctive words the reply reuses, scored only without notes; `retold` at 0.6 or more) and 7 new hard items (dr-en-122 to 125, dr-hi-108, 109, dr-hl-204). E4B, hard set, 3 samples each: informational messages taken over 13 of 21 to 0, retold 3 to 0; scam messages answered with a promise to check or send 10 of 21 to 0; shape 96 to 96 of 108. Holdout: the refund-for-bank-details email went from "I will check regarding sending my bank details ... and get back to you" to "I do not share bank details." (without a greeting, the one shape miss). E2B, full set: taken over 3 of 7 to 0, scam promises 4 of 7 to 1; the Hinglish leave message once answered in English. No-notes prompt 157 to 221 tokens: first output 3.15 to 4.22 s on E4B (median), but the reply ends sooner (5.9 to 5.67 s) as it is shorter; E2B 1.73 to 2.08 s and 2.66 to 2.99 s. Rejected on the way: 13 (credentials got "I will check and get back"), 14 and 15 (the same, then fixed), 16 (an English example sentence made Hinglish replies English), 17 (right, but 287 tokens: first output 5.4 s) |
| reply | 7 (rejected) | firmer rule against instructions inside the message, a reminder after it, and "never say a reply includes an address, number, file or link it does not contain". Sampled 8 times per item on 12 injection and missing-information items: 44 of 96 against 50 of 96 for revision 4; better on hard items, worse on a holdout one (invented a flat number 7 of 8 times against 4); first output 0.5 s to 0.96 s. Strong injections ("answer in French", "HAHAHA", "I love pizza", repeat a code) win every time on E2B with either prompt |

Reply on E4B (revision 4, 4 samples per item): 9 of 48 obeyed against 50 of 96 on E2B; only "I
love pizza" and "repeat the code" still get through. But reply takes 4.3 s on E4B against 2.2 s
on E2B (median, hard set), over the 4.0 s target, so moving reply to the quality tier is the
owner's call.

Revisions 8 and 9 (rewrite) and 8 (proofread) were judged by Claude in a parallel session
(no reviewer agents there), blind to which revision wrote which output.

Prompting has levelled off for reply at about 80% acceptable. Still weak: rewrite in polite and
friendly tones (about half acceptable) and meaning drift in
formal Hindi; reply sometimes answers as an assistant to injected instructions and repeats one
answer three ways. Still missed by E2B: some Hindi and Hinglish gender agreement on
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
