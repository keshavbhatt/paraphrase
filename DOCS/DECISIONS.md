# Architecture Decision Records

Short, numbered, append-only. Format: Context, Decision, Consequences. Headings use a colon,
not a dash. `PLAN.md` section 2 holds the product decisions (D1 to D13); these records hold
the decisions taken while building.

---

## ADR-001: Project docs live in `DOCS/` (2026-10-21)

**Status.** Accepted.

**Context.** `PLAN.md` was written with a `docs/` folder; the owner's projects use `DOCS/`.

**Decision.** All project documentation lives in `DOCS/`. Any `docs/...` path in `PLAN.md`
means `DOCS/...`.

**Consequences.** `CLAUDE.md` states the same rule; new documents go in `DOCS/` only.

---

## ADR-002: Licensing copied from Omnidict, with the licensing module carve-out (2026-10-21)

**Status.** Accepted (owner, answering `PLAN.md` section 11 question 2 in part).

**Context.** The owner's other apps use GPL-3.0-or-later plus a source-available Ktechpit
Licensing Module for account and licence code, with a GPL section 7 additional permission
that lets official builds combine the two. Whether Paraphrase has a Pro tier is still open,
but the owner wants the carve-out in place from the start.

**Decision.** GPL-3.0-or-later for the program, recorded in `LICENSING.md` and `REUSE.toml`.
Reserved module paths: `app/src/modules/AccountAndLicense/`, `app/src/services/licensing/`
and `daemon/src/licensing/`. The module's licence text is added together with its first
file, since REUSE rejects an unused licence text.

**Consequences.** `reuse lint` passes from the first commit. Model licences are separate and
live in `models.json`; models are never redistributed (PLAN D8).

---

## ADR-003: The fast tier ships on 8 GB CPU-only machines with measured targets (2026-10-22)

**Status.** Accepted (decided by Claude at the owner's request, 2026-10-22: "I want you to
decide the best options"). Reversible by the owner. Overrides PLAN 11 Q4's rule that a
no-go for the fast tier on machine A is fatal.

**Context.** PLAN 5.4 asked for a first token under 0.8 s and a proofread under 3.5 s on
machine A (8 GB, 4 cores, no GPU). The owner's rule (PLAN 11 Q4, 2026-10-22): after prompt
caching, batched variants, a thread sweep, quantisation and Vulkan, a fast tier still worse
than twice the target is a no-go. Measured on emulated A (DOCS/models.md, Latency), Gemma 4
E2B, the only fast candidate with acceptable quality, reaches its first token in about 3.1 s
and finishes a proofread in 7.9 s. The limit is arithmetic: four cores process about 48
prompt tokens a second, and a 100-word text is about 150 tokens. The one model fast enough
(Qwen3.5-0.8B, first token 1.0 s) had 15 of 20 spot-check answers rated bad.

**Decision.**
1. Report the strict result: by the owner's rule the fast tier on A is a no-go.
2. Ship anyway with Gemma 4 E2B as the fast tier on 8 GB machines: a correct answer in 5 to
   8 s is useful, a wrong answer in 2 s is not. The UI is designed for the wait (text
   streams in, the status line says what is happening, DOCS/DESIGN.md).
3. Replace the PLAN 5.4 targets with the measured p50 and p90 plus 20% headroom
   (PLAN 5.4 now lists them). `bench_latency.py` guards them as a regression gate.
4. Re-measure on a real older laptop before M5. If a real 8 GB machine is much slower than
   the emulation, this decision is reopened.

**Consequences.** 8 GB users see the first word after about 3 s (about 2 s once the daemon
reads ahead, ADR-005). The product promise for those machines is "private and correct", not
"instant". The quality tier (Gemma 4 E4B) is labelled "16 GB recommended"; on A it runs as a
slow fallback (about 6 s to the first token).

---

## ADR-004: Speculative decoding with the Gemma MTP draft heads, per skill (2026-10-22)

**Status.** Accepted.

**Context.** ggml-org publishes multi-token-prediction draft heads (about 60 MB) for Gemma 4
E2B and E4B. Measured p50 (DOCS/models.md): proofread 10.6 to 7.9 s on A and 7.1 to 4.1 s on
B for E2B, 13.4 to 7.2 s on B for E4B; rewrite gains less; reply gets a worse p90.
N-gram drafting from the input and E2B as a draft model for E4B were slower.

**Decision.** The engine turns MTP drafting on per skill: on for proofread and rewrite, off
for reply. The heads are catalog entries with `role: draft` and `draft_for`, downloaded and
verified like any model, and shown in the model manager as "Speed-up". Greedy outputs are
identical with and without it.

**Consequences.** About 1 to 2 s more model load, under 0.1 GB more memory. The skill schema
gains a speculative-decoding switch when the daemon implements it (M1), with a schema bump.

---

## ADR-005: Read ahead while the user picks an action (2026-10-22)

**Status.** Accepted for M1 (the daemon's `Warm()`, PLAN 5).

**Context.** Reading the input is most of the wait on A (about 3 s). The popup is open for
about a second before the user picks an action.

**Decision.** When the popup opens, the daemon starts processing the captured text with the
prompt of the action the user picks most often. If the user picks it, that work is reused
through the prompt cache; if not, it is dropped. No prompt change is needed: each skill's
system prompt is already cached separately.

**Consequences.** Saves up to the choosing time (about 1 s) on the likely action; costs
nothing visible otherwise. The popup's status line says "Reading the text for Proofread",
so the work is never hidden.

---

## ADR-006: Language check in the engine, and other languages never mislabelled (2026-10-22)

**Status.** Accepted (owner's rules, 2026-10-22).

**Context.** Small models switch language: v1 prompts made Qwen3.5-4B rewrite 49 of 50
English texts into Devanagari, and professional rewrites turned Hinglish into English. The
first language detector also called every language other than Hindi and Hinglish
"English", which tells a model to translate a French text into English.

**Decision.** Skill schema v2: `output.language: same` makes the engine check every output
against the expected language (the input's, or a `target_lang` the user explicitly chose)
and retry once; a second failure is shown with a warning, never hidden. A rewrite keeps the
input language unless the user chose another. The detector names about 40 languages by
script and common words and says "its own language" when unsure.

**Consequences.** Retries add a second generation to the few outputs that fail; the eval
and the bench run through the same engine code (`eval/paraeval/engine.py`), so their numbers
include it. The daemon ports the detector and the check to C++ with the same test vectors.
