# Progress

Newest first. One entry per working session.

## Milestone status

| Milestone | Status |
|---|---|
| M0 Feasibility spike | in progress |
| M1 Daemon | todo |
| M2 Popup on X11 + KDE Wayland | todo |
| M3 Polish + GNOME/wlroots | todo |
| M4 Mail | todo |
| M5 Release | todo |
| M6 IME + Windows | todo |

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
