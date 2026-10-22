# Coding Standards: Paraphrase (Qt 6 / C++20, and the Python eval harness)

These are the rules the project follows. They are binding for every commit. When a rule is
broken for a good reason, the reason goes in a code comment and, if architectural, in
`DOCS/DECISIONS.md`.

Adapted from Omnidict's `DOCS/CODING_STANDARDS.md` (same owner, same stack): the Omnidict
pipeline section becomes the eval harness, and the layout follows PLAN 4 (daemon, cli, app).
The goal is the same: **small, boring, testable code**.

---

## 1. Language & toolchain

| Item | Rule |
|---|---|
| Language | C++20 for our targets (`CMAKE_CXX_STANDARD 20`, required, no extensions). Third-party code (llama.cpp, yaml-cpp) keeps its own standard. |
| Qt | Qt 6.11 minimum, the version of the KDE snap SDK and runtimes. No Qt5 compatibility shims. |
| Build | CMake >= 3.25 (`add_subdirectory(... SYSTEM)` for third-party headers), `qt_standard_project_setup()`, `qt_add_executable`. No qmake. The CMake root is the repository root: the daemon, the CLI and later the app share one build. |
| Dev builds | **Identical to the snap pipeline** (owner, 2026-10-22): `scripts/dev-build.sh` builds and `dev-run.sh --ctest` tests in core24 (Ubuntu 24.04) with its GCC 13, the `cmake` snap and the `kde-qt6-core24-sdk` snap, tests running against the `kf6-core24` runtime snap. On any other host they run inside the container `packaging/core24/Dockerfile` (`scripts/core24-run.sh`, Docker); CI's Ubuntu 24.04 runner builds directly. Never the host's compiler, cmake or Qt. The Flatpak pipeline's toolchain differs and is checked in CI from M5. |
| Compiler | GCC >= 12 / Clang >= 15. Warnings on: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wnon-virtual-dtor -Wold-style-cast` and more (`cmake/Warnings.cmake`), on our targets only. Warnings are errors in CI (`-Werror` behind `PARAPHRASE_WERROR`). |
| Formatting | `clang-format` with the repo `.clang-format`; CI runs `clang-format --dry-run --Werror`. |
| Static analysis | `clang-tidy` with the repo `.clang-tidy`; `make lint` runs it with warnings as errors. |
| Tests | Qt Test (`QTest`) via CTest. Every non-UI module ships a test. |

---

## 2. Project layout

```
paraphrase/
  daemon/       paraphrase-daemon: owns the model, does all inference (PLAN D1)
    src/
      app/        entry point, version (version.h.in)
      util/       pure logic: language detection and checks, text helpers
      skills/     SkillRegistry, YAML loader, template rendering, validation
      engine/     LlamaEngine: model, prompt cache, sampling, grammars, variants, language retry
      models/     ModelCatalog, Downloader, HardwareProbe, TierSelector
      api/        D-Bus adaptor, request queue, native-messaging socket (M4)
    tests/        Qt Test, one tst_<module>.cpp per module
  cli/          paraphrase CLI, a D-Bus client
  app/          Qt Widgets desktop client (from M2): main window, popup, settings
  eval/         Python harness: datasets, metrics, eval and latency runs (the reference
                implementation of the engine's text logic, see section 12)
  skills/       built-in skill YAML files and grammars (PLAN D4)
  tests/        data shared between the Python harness and the C++ tests (vectors, golden files)
  scripts/      dev-build.sh, dev-run.sh, snap-runtime-env.sh, build-llama.sh, fetch_model.py
  third_party/  pinned submodules: llama.cpp, yaml-cpp
```

Dependency direction inside `daemon/src/` is strict and one-way:

```
app --> api --> engine --> skills --> util
                models --> util
```

* `util/` and `skills/` compile with only `Qt6::Core` (and yaml-cpp for `skills/`).
* `engine/` is the only code that includes llama.cpp headers.
* `models/` may use `Qt6::Network`; `api/` may use `Qt6::DBus`.
* Every directory is its own `add_library(paraphrase_<name> STATIC)` target with explicit
  `target_link_libraries`, so the arrows are enforced by the linker. Directories are created
  in the step that needs them, not before: no dead code.

---

## 3. Naming

| Thing | Convention | Example |
|---|---|---|
| Namespace | `paraphrase::<layer>` | `paraphrase::util`, `paraphrase::engine` |
| Class / struct / enum | `PascalCase` | `SkillRegistry`, `LlamaEngine` |
| Method / free function | `camelCase` | `renderTemplate()` |
| Member variable | `m_camelCase` | `m_model` |
| Static member | `s_camelCase` | (should be rare) |
| Constant / constexpr | `kPascalCase` | `constexpr int kSchemaVersion = 3;` |
| Enum values | `PascalCase` in `enum class` | `Tier::Fast` |
| Signals | past tense or noun-ish, no `on` prefix | `tokenGenerated(QString)` |
| Files | `snake_case.cpp/.h` matching the primary class: `skill_registry.h` -> `class SkillRegistry` | |
| CMake option/env prefix | `PARAPHRASE_` | `PARAPHRASE_WERROR` |
| CMake target prefix | `paraphrase_` | `paraphrase_util` |
| Logging category root | `paraphrase.` | `paraphrase.engine`, `paraphrase.models` |
| D-Bus names | under the app ID (ADR-007) | `com.ktechpit.paraphrase.Daemon` |

One class per file pair. Header guards: `#pragma once`.

---

## 4. Classes & ownership

1. **Qt parent-child ownership** for every `QObject`. A `QObject` without a parent is held in
   `std::unique_ptr`, never a bare pointer.
2. Non-`QObject` types: value semantics or `std::unique_ptr`. `std::shared_ptr` needs a
   justification comment.
3. Raw pointers are **non-owning observers only**. llama.cpp handles (`llama_model *`,
   `llama_context *`, samplers) are owned through `std::unique_ptr` with their free function
   as the deleter, in `engine/` only.
4. Rule of zero. A hand-written destructor explains why.
5. No singletons. Pass dependencies through constructors.
6. `Q_OBJECT` and `Q_DISABLE_COPY_MOVE` on every `QObject` subclass.
7. Prefer composition over inheritance.
8. Hard size limits: a class over 500 lines or a function over 60 lines is a review blocker.
9. No `friend`. No `protected` data members.
10. Interfaces (`class ICaptureBackend { public: virtual ~ICaptureBackend() = default; ... }`) for
    anything with more than one implementation (platform backends, test doubles).

---

## 5. Qt idioms

* **Signals/slots**: pointer-to-member-function syntax only; lambdas capture `this` only with
  a context object.
* **No `.ui` files.** Widgets are built in code (from M2).
* **Strings**: `u"..."_s` (`using namespace Qt::StringLiterals`) or `QStringLiteral`; `tr()` for
  anything the user sees.
* **Containers**: `QList`/`QHash` when the value crosses a Qt API boundary; `std::` otherwise.
* **Enums** in signals, settings or D-Bus are `enum class` with `Q_ENUM`/`Q_ENUM_NS`.
* **Threads**: none unless measured. Pre-approved: one inference thread in the daemon that owns
  the llama.cpp context (decoding blocks for seconds; the D-Bus thread must stay responsive),
  and a `QThreadPool` for downloads and sha256 checks. Nothing else spins up a thread without
  a measurement.
* **Logging**: `Q_LOGGING_CATEGORY` per module. `qDebug()` without a category is forbidden. No
  `std::cout`/`printf`, except a CLI writing its result to stdout. **Never log user text**
  (CLAUDE.md): log lengths, ids and timings, not content.
* **Deprecated API** is a build error (`QT_DISABLE_DEPRECATED_UP_TO=0x060b00`).
* `QT_NO_CAST_FROM_ASCII`, `QT_NO_CAST_TO_ASCII`, `QT_NO_KEYWORDS` are defined globally (use
  `Q_SIGNALS`/`Q_SLOTS`/`Q_EMIT`).
* `QStandardPaths` for every path; the data folder differs per package format (ADR-009).

---

## 6. Error handling

* No exceptions across Qt or D-Bus boundaries. yaml-cpp throws: catch at the loader and turn
  it into a `Result`.
* Fallible operations return `std::optional<T>` or `Result<T>` (`util/result.h`), never a bool
  plus out-parameter.
* Every failure a user could care about is logged with a category and reaches the client as
  an `Error(request_id, code, message)` signal (PLAN 5.1). Messages say what went wrong and
  what to do.
* `Q_ASSERT` for invariants, never for input validation.

---

## 7. Settings

* One `Settings` facade (from the step that first needs one), typed accessors, defaults in
  one table, keys `constexpr` and never spelled twice; a signal on change.

---

## 8. Testing

* `util/`, `skills/` and the engine's text logic are pure and get high coverage.
* Tests that need a model read it from `models/` and skip with `QSKIP` when it is not
  downloaded, so `ctest` passes on a fresh checkout; CI fetches the fast model.
* Tests use only public API.
* Test files: `daemon/tests/tst_<module>.cpp`, one `QObject` class per file,
  `QTEST_GUILESS_MAIN`.

---

## 9. Git & review

* Commit style: `area: plain sentence`, for example `skills: loader for schema v3 with tests`.
  No conventional-commit prefixes, no trailers.
* One logical change per commit; a commit builds and passes its tests on its own.
* Every feature change updates `DOCS/PROGRESS.md`.
* No commented-out code. No `TODO` without an issue number. No dead code "for later".

---

## 10. Things explicitly banned

| Banned | Why | Do instead |
|---|---|---|
| llama.cpp calls outside `engine/` | the engine owns the model, the context and the thread | go through `LlamaEngine` |
| Logging, printing or persisting user text | privacy (CLAUDE.md, PLAN D13) | log lengths and ids |
| A TCP listener of any kind | PLAN D7 | D-Bus, or the Unix socket for the native host |
| Feature logic in the engine | PLAN D4: features are skills | extend the skill schema (bump it, update DOCS/skills.md) |
| `#ifdef Q_OS_*` inside business logic | untestable | platform backends behind interfaces |
| `SIGNAL()`/`SLOT()` macros, `.ui` files | not type-checked | PMF connects, code-built widgets |
| Global mutable state, `extern` variables | untraceable | constructor injection |
| Third-party code copied into `src/` | unmaintained forks | pinned submodule + `THIRD_PARTY.md` + ADR |
| Downloading at build time (`FetchContent` from the network) | Flatpak and snap builds have no network | submodules |
| Blocking the D-Bus thread on inference or I/O | the API freezes | the inference thread, signals back |

---

## 11. Python (eval harness)

* Python 3.11+. Full type hints; `mypy --strict` clean.
* `ruff check` and `ruff format` clean, line length 100.
* Tests with `pytest` under `eval/tests/test_<module>.py`; names describe behaviour.
* `pathlib.Path` for every path. No network access in tests.
* No bare `except`. Records are frozen dataclasses where they are plain data.

---

## 12. Shared vectors and golden files

* The Python harness (`eval/paraeval/`) is the reference for the engine's text logic: language
  detection and checks, template rendering and data fencing, output parsing. The C++ port must
  agree with it exactly.
* The contract is `tests/vectors/*.json`, generated from the Python code by
  `eval/export_vectors.py` and read by both test suites. Any change to that logic changes the
  vectors in the same commit, and the diff is reviewed like code.
* Golden files are regenerated only deliberately, never to make a failing test pass without
  reading why it failed.

---

## 13. Shell

* `#!/usr/bin/env bash` and `set -euo pipefail` at the top of every script; `shellcheck` clean.

---

## 14. Definition of Done (per change)

- [ ] Follows every section above; `clang-format` and `clang-tidy` clean; `ruff`, `mypy` and
      `pytest` clean for Python changes.
- [ ] Unit tests for the logic touched; `ctest` and `make test` pass.
- [ ] Skill, prompt, grammar or model changes: eval before/after and the latency check
      (CLAUDE.md).
- [ ] Logging category used; no silent failures; no user text in logs.
- [ ] `DOCS/PROGRESS.md` entry.
- [ ] No new third-party dependency without an ADR in `DOCS/DECISIONS.md` and a row in
      `THIRD_PARTY.md`.
