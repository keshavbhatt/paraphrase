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
