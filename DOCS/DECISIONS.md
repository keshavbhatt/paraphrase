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
3. Replace the PLAN 5.4 targets with measured numbers: for each task the slowest p50 and p90
   of every session run with the chosen settings, plus 20% headroom (PLAN 5.4,
   `eval/targets.json`). `bench_latency.py --check` guards them as a regression gate. A
   single session was not enough: machine B is a desktop in use and its reply times moved by
   up to 30% between two sessions on the same day.
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

**Consequences.** About 1 to 2 s more model load, and about 0.2 GB more memory: the daemon
measured 4.49 GB without the head and 4.71 GB with it for E2B on B (M1), most of it the draft
context's cache. The skill schema has the switch since v3 (`speculative: none | draft`).

---

## ADR-005: Read ahead while the user picks an action (2026-10-22)

**Status.** Accepted for M1 (the daemon's `Warm()`, PLAN 5).

**Context.** Reading the input is most of the wait on A (about 3 s). The popup is open for
about a second before the user picks an action.

**Decision.** When the popup opens, the daemon starts processing the captured text with the
prompt of the action the user picked last (until 2026-10-23: the one picked most often,
which kept Reply on top for the user's own text; owner's choice). If the user picks it, that work is reused
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

---

## ADR-007: One app ID, and a D-Bus name under it (2026-10-22)

**Status.** Accepted (owner, 2026-10-22; DOCS/packaging.md, section 3).

**Context.** A Flatpak may own `<app-id>` and `<app-id>.*` on the session bus without extra
permission; any other name needs `--own-name`, which Flathub reviewers question. A snap
needs a store declaration for any well-known name. PLAN 5.1 used `com.ktechpit.Paraphrase1`.

**Decision.** App ID `com.ktechpit.paraphrase` everywhere (desktop file, metainfo, Flatpak,
snap `common-id`), like Omnidict's `com.ktechpit.omnidict`. Daemon bus name
`com.ktechpit.paraphrase.Daemon`, object path `/com/ktechpit/paraphrase/Daemon`, interface
`com.ktechpit.paraphrase.Daemon1` (the number versions the API). Request the snap's name
declaration with the first upload.

**Consequences.** No extra Flatpak permission for the bus name. PLAN 5.1 uses the new names.

---

## ADR-008: Packages build llama.cpp for every CPU, not for the build machine (2026-10-22)

**Status.** Accepted.

**Context.** The M0 build uses `GGML_NATIVE=ON`, tuned to the laptop that built it. A package
built that way stops with an illegal instruction on older CPUs.

**Decision.** Packages build llama.cpp with `GGML_NATIVE=OFF`, `BUILD_SHARED_LIBS=ON`,
`GGML_BACKEND_DL=ON` and `GGML_CPU_ALL_VARIANTS=ON`: one CPU library per instruction-set level,
the best one loaded at run time (as Debian's llama.cpp packages do). Developer builds may
stay native.

**Consequences.** M1 re-runs the M0 bench on the portable build; a gap against the native
numbers is a bug to fix before M5, not a reason to ship native builds.

---

## ADR-009: Models live in each format's own data folder (2026-10-22)

**Status.** Accepted.

**Context.** Models are 3 to 10 GB. In a snap the per-revision folder (`$SNAP_USER_DATA`) is
copied on every refresh, and snapd snapshots a snap's data when it is removed.

**Decision.** Models go in `$SNAP_USER_COMMON` (snap), `~/.var/app/com.ktechpit.paraphrase/data`
(Flatpak), `$XDG_DATA_HOME/paraphrase` (AppImage, native). The UI always shows the real path
and the space used; uninstall help mentions `snap remove --purge` and
`flatpak uninstall --delete-data`.

**Consequences.** The Settings mock's fixed path becomes the path of the running format.

---

## ADR-010: Clients start the daemon; it never depends on desktop services (2026-10-22)

**Status.** Accepted.

**Context.** D-Bus activation, user services and login autostart each work differently in a
snap, a Flatpak and an AppImage, and some need store approval (DOCS/packaging.md).

**Decision.** Any client (main window, popup, CLI) starts `paraphrase-daemon` when it is not
on the bus; the daemon unloads the model and exits after a quiet period. If the daemon
cannot own its bus name (for example a snap still waiting for its name declaration), the
client runs it as a private child process over a private socket, with the same API. Login
autostart is an option in Settings, through the Background portal (Flatpak), the snap's
`autostart` key, or `~/.config/autostart` (AppImage, native). D-Bus activation and user
services are optimisations, never required.

**Consequences.** PLAN M5's "systemd user unit" is replaced by this. The daemon API is the
same in both modes.

---

## ADR-011: D-Bus is the API; the Unix socket serves only the browser's native host (2026-10-22)

**Status.** Accepted (refines PLAN D7).

**Decision.** Desktop clients use D-Bus only. The Unix socket exists for the native messaging
host (M4) and lives where each format shares files with the host:
`$XDG_RUNTIME_DIR/app/com.ktechpit.paraphrase/` (Flatpak), `$XDG_RUNTIME_DIR/snap.paraphrase/`
(snap), `$XDG_RUNTIME_DIR/paraphrase/` (AppImage, native); mode 0600, never TCP.

---

## ADR-012: One package per format, holding the daemon, the app and the CLI (2026-10-22)

**Status.** Accepted.

**Decision.** Each of snap, Flatpak and AppImage ships all three programs together. The
exceptions are pieces that must live on the host: the input-method addon (M6) as distro
packages (deb, rpm, AUR), and browser manifests that the app writes where the format allows
(DOCS/packaging.md, section 4).

**Consequences.** No cross-sandbox D-Bus rules between our own components.

---

## ADR-013: Standalone first: every feature works without desktop integration (2026-10-22)

**Status.** Accepted (owner, 2026-10-22: "The application and all its features should be
available without these desktop integration issues caused by packaging").

**Context.** The global shortcut, reading text from other apps and typing back into them
depend on the session (X11, KDE, GNOME, wlroots) and on the package format (DOCS/packaging.md).
Any of them can be missing.

**Decision.** The main window is a complete surface on its own: paste or type text, run any
skill (proofread with the diff, reply, every tone, summarize, format, translate, custom
actions), copy the result. It needs nothing from the desktop but a window and the clipboard
while it has focus. The popup, the global shortcut, capture and inject are accelerators on
top of the same skills; when one is missing the app says what is unavailable and how to
get the same result in the main window, and nothing else stops working. The CLI is the
second standalone surface.

**Consequences.** M2 builds the main window with every skill before the popup's integration
backends; its acceptance test runs with every hotkey, capture and inject backend switched off.
A main-window mock is needed before it is coded (mocks-before-screens rule).

---

## ADR-014: yaml-cpp reads skill files in the daemon (2026-10-22)

**Status.** Accepted.

**Context.** Skills are YAML (PLAN D4) and users write their own (custom actions). Qt has no
YAML reader. The Python harness uses PyYAML (YAML 1.1).

**Decision.** yaml-cpp 0.9.0 (MIT), as a pinned git submodule `third_party/yaml-cpp`, built
static by `cmake/YamlCpp.cmake` with its tools, tests and install rules off. Snap and Flatpak
builds have no network, so no download at build time. The loader resolves plain scalars as
PyYAML does (YAML 1.1 null, booleans, integers and floats; quoted scalars stay strings) so
both read a skill file the same way; `tests/vectors/skills.json` checks it.

**Consequences.** yaml-cpp throws on malformed YAML; the loader catches it and returns an
error naming the file, as the coding standards require.

---

## ADR-015: The model catalog is built into the daemon (2026-10-22)

**Status.** Accepted.

**Context.** Omnidict fetches its dictionary catalog from a release on the network. A model
catalog pins each file to an upstream revision and a sha256; those pins are what make a
download trustworthy, so whoever can change the catalog can change what users run.

**Decision.** `models.json` is compiled into the daemon as a resource and never fetched. The
only network use stays the model files themselves, from Hugging Face at the pinned revision,
over HTTPS, checked against the sha256 before first use (PLAN D8, section 8). New or updated
models arrive with an app update.

**Consequences.** No signing or update channel for the catalog is needed. A model fix waits
for a release. The eval harness and the daemon read the same file (`models.json` at the
repository root), so what is measured is what ships.

---

## ADR-016: Hardware probe defaults: threads, CPU only, quality tier from 16 GB (2026-10-22)

**Status.** Accepted for M1; the GPU choice is revisited with the first-run timing run (M3).

**Context.** llama.cpp's default thread count on Intel hybrid CPUs is the number of
performance cores: 2 on machine B, where the M0 sweep (2 to 14 threads) found 10 fastest.
Machine B's 10 are its 2 P-cores and 8 E-cores; its 2 low-power E-cores sit outside the L3
cache and slow every step down to their pace. M0 also found the Intel iGPU (Vulkan) slower
than the CPU. The quality tier needs about 9 GB in use (DOCS/models.md).

**Decision.** `HardwareProbe` reads `/proc` and `/sys` only. Threads: one per physical core
that shares the L3 cache (all physical cores when the CPU reports no L3), capped by the CPUs
the process may use. Memory: RAM, or a lower cgroup limit. The engine runs on the CPU. The
quality model is offered when the computer has 16 GB (`ram_required`, with 10% tolerance
because computers report less than their nominal size); a skill that asks for the quality
tier runs on the fast model otherwise. The fast model runs on any computer, as ADR-003 says.

**Consequences.** The rule is measured on one CPU; the bench on a real older laptop (before
M5) checks it there. Settings can override the thread count. GPU offload waits for the
timing run the first-run mock describes, which names a GPU it does not use.

## ADR-017: The history lives in the app, encrypted, its key from the desktop's secret store (2026-10-25)

**Status.** Accepted (mocks settings-privacy.html and main-window-history.html, approved by the
owner on 2026-10-25).

**Context.** PLAN D13: history is off by default, stored locally and encrypted, wiped in one
click. CLAUDE.md allows user text to persist only in this opt-in store. SQLCipher is not in
the KDE build SDK; Qt's SQL module with its SQLite driver and OpenSSL's libcrypto are, and
QtKeychain is in the runtime without headers. The desktop offers secrets two ways: the
Secret portal (`org.freedesktop.portal.Secret`, one secret per app, no permission needed in a
Flatpak or snap) and the Secret Service (`org.freedesktop.secrets`: KWallet's ksecretd,
GNOME Keyring).

**Decision.** The app, not the daemon, keeps the history: the daemon stays free of stored
text. One SQLite file (`history.sqlite` in the app's data folder) through Qt SQL; each entry
keeps its time, action, preset or language and app name in clear (to list and filter), and
its text and result sealed with AES-256-GCM (libcrypto, a random 96-bit nonce per field). The
key is 32 bytes derived (HKDF-SHA256) from a secret the desktop keeps: the Secret portal's
app secret first, else one stored in the Secret Service under "Paraphrase history". Without
either, history cannot be turned on and Settings says why. Search decrypts in memory. Delete
all removes the file and forgets the stored secret. The key also mixes in a random salt kept in
the file, so a portal secret that cannot be forgotten still opens nothing once the file is
gone, and a sealed check value tells a wrong key from a damaged file (the file is then left as
it was and Settings offers Delete all). Old entries are deleted from their clear times, so
pruning at start needs no keyring.

**Consequences.** Text never reaches the disk unsealed; the metadata (when, which action,
which app) does, which the Privacy page says. A lost keyring makes the history unreadable,
which is the point. The snap needs the `password-manager-service` plug only for the Secret
Service fallback; the portal needs nothing. Unverified: the portal's app secret for an
unsandboxed app on GNOME (KDE answers it).
