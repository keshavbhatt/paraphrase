# Top-level entry points (as Omnidict). The daemon builds through scripts/dev-build.sh
# against the KDE snap SDK; the Python eval harness runs through uv.
#
#   make build              configure and build the daemon with tests, warnings as errors
#   make test               eval harness tests, skill validation, daemon build, ctest
#   make lint               ruff, mypy, clang-format, clang-tidy, shellcheck, reuse
#   make format             apply ruff format and clang-format
#   make llama              M0 tools: build llama-server/llama-bench (build/llama-cpu, -vulkan)
#   make fetch MODEL=<id>   download a model from models.json and verify its sha256
#   make smoke MODEL=<id>   5 items per skill through the real model (CPU build)

export PATH := $(HOME)/.local/bin:$(PATH)

PY := uv run python
MODEL ?=
BUILD_DIR ?= build/dev
CXX_DIRS := daemon cli
CXX_SOURCES := $(shell find $(CXX_DIRS) -name '*.cpp' -o -name '*.h')
TIDY_SOURCES := $(shell find $(CXX_DIRS) -name '*.cpp')

.PHONY: all build test test-eval test-daemon lint lint-eval lint-daemon format llama fetch smoke clean

all: test

build:
	PARAPHRASE_BUILD_DIR=$(abspath $(BUILD_DIR)) \
	PARAPHRASE_CMAKE_ARGS="-DPARAPHRASE_BUILD_TESTS=ON -DPARAPHRASE_WERROR=ON" \
	scripts/dev-build.sh

test-eval:
	uv run pytest -q
	$(PY) eval/validate_skills.py
	$(PY) eval/export_vectors.py --check

test-daemon: build
	PARAPHRASE_BUILD_DIR=$(abspath $(BUILD_DIR)) scripts/dev-run.sh --ctest

test: test-eval test-daemon

lint-eval:
	uv run ruff check .
	uv run ruff format --check .
	uv run mypy eval scripts

lint-daemon: build
	clang-format --dry-run --Werror $(CXX_SOURCES)
	clang-tidy -p $(BUILD_DIR) --quiet --warnings-as-errors='*' $(TIDY_SOURCES)

# reuse scans git-ignored trees such as .venv, so lint what git tracks plus new files.
lint: lint-eval lint-daemon
	shellcheck -x scripts/*.sh
	git ls-files --cached --others --exclude-standard -z | grep -zv '^third_party/' | xargs -0 uv run reuse lint-file

format:
	uv run ruff format .
	clang-format -i $(CXX_SOURCES)

llama:
	scripts/build-llama.sh

fetch:
	@test -n "$(MODEL)" || { echo "usage: make fetch MODEL=<id>"; exit 2; }
	$(PY) scripts/fetch_model.py $(MODEL)

smoke:
	@test -n "$(MODEL)" || { echo "usage: make smoke MODEL=<id>"; exit 2; }
	for s in proofread reply rewrite.professional; do \
		$(PY) eval/run_eval.py --model $(MODEL) --skill $$s --limit 5 --profile B-cpu --tag smoke || exit 1; \
	done

clean:
	rm -rf $(BUILD_DIR) compile_commands.json
