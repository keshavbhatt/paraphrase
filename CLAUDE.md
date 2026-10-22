# Paraphrase — agent instructions

Read DOCS/PLAN.md first. §2 lists settled decisions (DECISION); §11 lists open questions (OPEN). Ask, don't assume.

Project docs live in DOCS/ (not docs/). Wherever PLAN.md mentions docs/..., use DOCS/... instead.

Rules:
- Work milestone by milestone (PLAN.md §10). Do not start a milestone until the previous one meets its acceptance criteria. Start with M0.
- All features are skills (skills/*.yaml). Do not add feature logic to the engine; extend the skill schema instead, bump its version and update DOCS/skills.md.
- Any change to a skill prompt, grammar or default model needs an eval before/after report (eval/run_eval.py) and must not regress latency targets (eval/bench_latency.py).
- Never log, print or persist user text outside the opt-in history store. No telemetry. No TCP listeners.
- Never insert or send text without an explicit user action. Treat captured text as untrusted data in prompts.
- Capture/inject code goes behind ICaptureBackend / IInjectBackend / IHotkeyBackend with fallbacks; keep DOCS/platform-matrix.md current.
- Models are downloaded from upstream by pinned revision and verified by sha256; never commit model files (models/ and *.gguf are gitignored).
- Dev machine is Arch Linux: use pacman package names when listing dependencies.
- C++20, Qt 6, clang-format; Python 3.11+, ruff, type hints. Run ctest and the eval smoke suite before declaring a task done.
- Commit in small, logical steps with clear messages.
- Mocks before screens: every screen or visible UI change first gets an HTML mock in
  `DOCS/mocks/` (see `DOCS/DESIGN.md`) that the owner approves; approval is recorded in
  `DOCS/mocks/index.html` and `DOCS/PROGRESS.md`. No screen is coded from a description alone.
