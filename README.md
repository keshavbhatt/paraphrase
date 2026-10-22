# Paraphrase

A fast, private writing assistant for the Linux desktop that runs a local language model:
proofread, change tone, smart replies, summarize and translate in any app, with nothing
leaving the machine.

## Status

M0 (feasibility) is done: Gemma 4 E2B and E4B, see `DOCS/models.md`. M1 (the daemon and the
CLI) is in progress: the CLI proofreads, rewrites and replies. See `DOCS/PLAN.md` for the plan and `DOCS/PROGRESS.md` for where things
stand; code follows `DOCS/CODING_STANDARDS.md`.

## Layout

```
DOCS/          plan, decisions, progress log, coding standards, models, skills, design and mocks
daemon/        paraphrase-daemon (C++20, Qt 6): owns the model, does all inference
cli/           paraphrase, the command-line client (D-Bus, DOCS/api.md)
third_party/   llama.cpp (git submodule, pinned)
skills/        skill YAML files and GBNF grammars
eval/          Python eval harness, latency bench and datasets (offline tooling, never shipped)
scripts/       build and model download scripts
models.json    model catalog: pinned upstream revisions, sha256, licences
```

## Prerequisites (Arch Linux)

```sh
sudo pacman -S --needed base-devel git cmake ninja clang vulkan-headers shaderc vulkan-icd-loader uv shellcheck snapd docker
sudo snap install kde-qt6-core24-sdk kf6-core24
sudo snap install cmake --classic
```

The daemon builds exactly as the snap pipeline does: in core24 (Ubuntu 24.04) with GCC 13,
the cmake snap and the KDE Qt snap SDK. On a host other than Ubuntu 24.04, `make build` and
`make test` run in a Docker container with that environment (`scripts/core24-run.sh`). The Vulkan driver for your GPU (`vulkan-intel`,
`vulkan-radeon` or `nvidia-utils`) is only needed for the M0 Vulkan bench.

## Quick start

```sh
git submodule update --init
make build                     # the daemon, the CLI and their tests, warnings as errors (build/dev)
make test                      # eval harness tests, skill validation, daemon build and ctest
make lint                      # ruff, mypy, clang-format, clang-tidy, shellcheck, reuse
make fetch MODEL=<id>          # downloads a model from models.json and checks its sha256
make smoke MODEL=<id>          # 5 items per skill through the real model
scripts/dev-run.sh -- --help   # run the daemon against the runtime snap
make llama                     # M0 tools: llama-server and llama-bench (build/llama-cpu, -vulkan)
```

## Using the CLI (development build)

```sh
export PARAPHRASE_MODELS_DIR=$PWD/models          # the models make fetch downloaded
alias paraphrase='PARAPHRASE_RUN=cli/paraphrase scripts/dev-run.sh --'
paraphrase models download fast                   # Gemma 4 E2B and its speed-up, checked
echo "Their going to the market tomorow" | paraphrase proofread
paraphrase rewrite --tone casual "Please find attached the figures."
paraphrase reply "Are you free for dinner on Friday?"   # three suggestions, one per line
paraphrase skills; paraphrase status; paraphrase models
```

The CLI starts `paraphrase-daemon` when it is not running, or runs it privately when there is
no session bus. Answers stream into a terminal; a pipe gets the finished answer only.

Licence: see `LICENSING.md`.
