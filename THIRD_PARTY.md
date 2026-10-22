# Third-party components

No new third-party dependency is added without an ADR in `DOCS/DECISIONS.md` and a row here.
Language models are not components: they are downloaded by the user from upstream and listed
with their licences in `models.json` and `DOCS/models.md`.

| Component | Version | Licence | How it is obtained | Used by |
|---|---|---|---|---|
| PyYAML | resolved in `uv.lock` | MIT | PyPI, via `uv` | `eval` (skill files) |
| ruff | dev-only | MIT | PyPI, via `uv` | tooling |
| mypy | dev-only | MIT | PyPI, via `uv` | tooling |
| pytest | dev-only | MIT | PyPI, via `uv` | tooling |
| reuse | dev-only | Apache-2.0 AND GPL-3.0-or-later (tool only, not linked) | PyPI, via `uv` | tooling (licence lint) |

## Design mocks only

`DOCS/mocks/mock.js` embeds Lucide glyph paths (ISC licence, the same set Omnidict's mocks
use) so the HTML mocks can draw icons. They do not ship in the app; licence text in
`LICENSES/ISC.txt`.
