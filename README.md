# Paraphrase

A fast, private writing assistant for the Linux desktop that runs a local language model:
proofread, change tone, smart replies, summarize and translate in any app, with nothing
leaving the machine.

## Status

Milestone M0 (feasibility spike): llama.cpp, candidate models, first skill prompts, eval sets
and a latency bench. No daemon or UI yet. See `DOCS/PLAN.md` for the plan and
`DOCS/PROGRESS.md` for where things stand.

## Layout

```
DOCS/          plan, decisions, progress log, models and skills notes
third_party/   llama.cpp (git submodule, pinned)
skills/        skill YAML files and GBNF grammars
eval/          Python eval harness, latency bench and datasets (offline tooling, never shipped)
scripts/       build and model download scripts
models.json    model catalog: pinned upstream revisions, sha256, licences
```

## Prerequisites (Arch Linux)

```sh
sudo pacman -S --needed base-devel git cmake ninja vulkan-headers shaderc vulkan-icd-loader uv shellcheck
```

plus the Vulkan driver for your GPU (`vulkan-intel`, `vulkan-radeon` or `nvidia-utils`).

## Quick start

```sh
git submodule update --init
make llama                     # builds llama.cpp twice: build/llama-cpu and build/llama-vulkan
make fetch MODEL=<id>          # downloads a model from models.json and checks its sha256
make test                      # harness unit tests and skill validation
make lint                      # ruff, mypy, reuse
make smoke MODEL=<id>          # 5 items per skill through the real model
```

Licence: see `LICENSING.md`.
