# Top-level entry points for M0 (offline tooling; the daemon arrives in M1).
#
#   make llama              build llama.cpp: build/llama-cpu and build/llama-vulkan
#   make fetch MODEL=<id>   download a model from models.json and verify its sha256
#   make test               harness unit tests and skill validation
#   make lint               ruff, ruff format check, mypy, shellcheck, reuse
#   make smoke MODEL=<id>   5 items per skill through the real model (CPU build)
#   make format             apply ruff format

export PATH := $(HOME)/.local/bin:$(PATH)

PY := uv run python
MODEL ?=

.PHONY: all llama fetch test lint smoke format

all: test

llama:
	scripts/build-llama.sh

fetch:
	@test -n "$(MODEL)" || { echo "usage: make fetch MODEL=<id>"; exit 2; }
	$(PY) scripts/fetch_model.py $(MODEL)

test:
	uv run pytest -q
	$(PY) eval/validate_skills.py

lint:
	uv run ruff check .
	uv run ruff format --check .
	uv run mypy eval scripts
	shellcheck -x scripts/*.sh
	git ls-files --cached --others --exclude-standard -z | grep -zv '^third_party/' | xargs -0 uv run reuse lint-file

smoke:
	@test -n "$(MODEL)" || { echo "usage: make smoke MODEL=<id>"; exit 2; }
	for s in proofread reply rewrite.professional; do \
		$(PY) eval/run_eval.py --model $(MODEL) --skill $$s --limit 5 --profile B-cpu --tag smoke || exit 1; \
	done

format:
	uv run ruff format .
