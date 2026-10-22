# Progress

Newest first. One entry per working session.

## Milestone status

| Milestone | Status |
|---|---|
| M0 Feasibility spike | done: go (DOCS/models.md, 2026-10-22) |
| M1 Daemon | todo |
| M2 Popup on X11 + KDE Wayland | todo |
| M3 Polish + GNOME/wlroots | todo |
| M4 Mail | todo |
| M5 Release | todo |
| M6 IME + Windows | todo |

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
