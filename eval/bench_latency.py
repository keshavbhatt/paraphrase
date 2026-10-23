#!/usr/bin/env python3
"""Latency bench for the PLAN 5.4 targets: time to first token, total time, tokens/s,
peak RAM and model load time, per model and run profile (eval/machines.json).

    uv run python eval/bench_latency.py --model qwen3.5-2b-q4km --profile A-emulated B-vulkan

Inputs are the five English dataset items closest to 100 words for each task (the "typical
input" of 5.4). Each task gets one warm-up request first, as the daemon keeps each skill's
system prompt cached (5.3), then --runs timed requests on different inputs. Loads are timed
twice: from disk with the model's pages evicted from the page cache ("cold"), then again
with the file cached ("warm"). Requests go through the engine (paraeval.engine), so batched
variants and the language-check retry are timed as the daemon would run them; p50 and p90
are over the timed requests.

Experiments: --server-args adds llama-server flags (e.g. "-fa on --spec-type ngram-simple")
and --variant names the run, so results land in <model>__<profile>__<variant>.json.
"""

from __future__ import annotations

import argparse
import dataclasses
import json
import math
import shlex
import statistics
import sys
import time
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from paraeval import common, engine
from paraeval.daemon import Daemon
from paraeval.llama import LlamaServer, evict_page_cache
from paraeval.skills import load_skills

TASKS = (
    "reply",
    "proofread",
    "rewrite.professional",
    "summarize",
    "format.bullets",
    "translate",
)
DEFAULT_TASKS = ("reply", "proofread", "rewrite.professional")
TARGETS = Path(__file__).resolve().parent / "targets.json"
TARGET_WORDS = 100


def bench_inputs(base_skill: str, n: int, preset: str | None = None) -> list[dict[str, Any]]:
    items = [i for i in common.dataset(base_skill, ("generated",)) if i["lang"] == "en"]
    if base_skill == "rewrite":
        items = [i for i in items if i.get("tone") == "professional"]
    elif preset:
        items = [i for i in items if i.get("preset", preset) == preset]
    field = "context" if base_skill == "reply" else "text"
    items.sort(key=lambda i: abs(len(i[field].split()) - TARGET_WORDS))
    return items[: n + 1]  # one extra for the warm-up


def _median(xs: list[float]) -> float:
    return round(statistics.median(xs), 3) if xs else 0.0


def _p90(xs: list[float]) -> float:
    """Nearest-rank 90th percentile: the value 90% of requests stay at or below."""
    if not xs:
        return 0.0
    ordered = sorted(xs)
    return round(ordered[max(0, math.ceil(0.9 * len(ordered)) - 1)], 3)


def bench(
    model_id: str,
    profile_name: str,
    runs: int,
    server_args: list[str] | None = None,
    overrides: list[Path] | None = None,
    tasks: tuple[str, ...] = TASKS,
    backend: str = "llama-server",
    build_dir: Path | None = None,
) -> dict[str, Any]:
    info = common.model(model_id)
    profile = common.profiles()[profile_name]
    if server_args:
        profile = dataclasses.replace(profile, extra_args=profile.extra_args + server_args)
    skills = load_skills(common.SKILLS_DIR, overrides)
    parallel = max(skills[t].variants for t in tasks)
    result: dict[str, Any] = {
        "model": model_id,
        "profile": profile_name,
        "label": profile.label,
        "when": time.strftime("%Y-%m-%d %H:%M"),
        "server_args": server_args or [],
        "skill_revisions": {t: skills[t].revision for t in tasks},
        "backend": backend,
    }
    if backend == "daemon" and (server_args or overrides):
        raise SystemExit("--server-args and --skill-override need the llama-server backend")

    def start() -> LlamaServer | Daemon:
        if backend == "daemon":
            return Daemon(model_id, profile, build_dir)
        return LlamaServer(info.path, profile, ctx=info.ctx, parallel=parallel)

    if build_dir:
        result["build"] = build_dir.name
    evict_page_cache(info.path)
    with start() as cold:
        result["load_cold_s"] = round(cold.load_s, 2)
    with start() as server:
        result["load_warm_s"] = round(server.load_s, 2)
        results_by_task: dict[str, Any] = {}
        for task in tasks:
            skill = skills[task]
            inputs = bench_inputs(
                skill.base_id, runs, task.split(".", 1)[1] if "." in task else None
            )
            samples = []
            results = []
            for k, item in enumerate(inputs):
                values = common.skill_inputs(skill.base_id, item)
                res = (
                    server.run(skill, values)
                    if isinstance(server, Daemon)
                    else engine.run(server, skill, values)
                )
                if k == 0:
                    continue  # warm-up: fills the system-prompt cache
                samples.append(res.calls[0])
                results.append(res)
            results_by_task[task] = {
                "runs": len(samples),
                "input_words": _median(
                    [
                        len(
                            common.skill_inputs(skill.base_id, i)
                            .get("context" if skill.base_id == "reply" else "text", "")
                            .split()
                        )
                        for i in inputs[1:]
                    ]
                ),
                "ttft_s": _median([c.ttft_s for c in samples]),
                "ttft_s_p90": _p90([c.ttft_s for c in samples]),
                "total_s": _median([r.total_s for r in results]),
                "total_s_p90": _p90([r.total_s for r in results]),
                "total_s_max": round(max(r.total_s for r in results), 3),
                "retried": sum(r.retried for r in results),
                "draft_accept_pct": round(
                    100
                    * sum(c.draft_accepted for c in samples)
                    / max(1, sum(c.draft_tokens for c in samples)),
                    1,
                ),
                "gen_tps": _median([c.gen_tps for c in samples]),
                "prompt_tps": _median([c.prompt_tps for c in samples]),
                "prompt_tokens": _median([c.prompt_tokens for c in samples]),
                "cached_tokens": _median([c.cached_tokens for c in samples]),
                "gen_tokens": _median([c.gen_tokens for c in samples]),
            }
            t = results_by_task[task]
            print(
                f"  {task:22} ttft {t['ttft_s']:.2f}/{t['ttft_s_p90']:.2f}s"
                f"  total {t['total_s']:.2f}/{t['total_s_p90']:.2f}s (p50/p90)"
                f"  gen {t['gen_tps']:.1f} t/s  prompt {t['prompt_tps']:.0f} t/s"
                f"  draft accepted {t['draft_accept_pct']}%"
            )
        result["tasks"] = results_by_task
        result["peak_rss_mb"] = round(server.peak_rss_mb)
    return result


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--model", nargs="+", required=True)
    ap.add_argument("--profile", nargs="+", default=["A-emulated", "B-vulkan"])
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--tag", default="m0")
    ap.add_argument("--server-args", default="", help="extra llama-server flags, one string")
    ap.add_argument("--variant", default="", help="names an experiment in the output file")
    ap.add_argument("--skill-override", type=Path, action="append", default=[])
    ap.add_argument(
        "--tasks",
        nargs="+",
        default=list(DEFAULT_TASKS),
        choices=TASKS,
        help="time only these tasks (e.g. reply without a draft head, the rest with one)",
    )
    ap.add_argument(
        "--check", action="store_true", help="fail when a result exceeds eval/targets.json"
    )
    ap.add_argument(
        "--backend",
        choices=("llama-server", "daemon"),
        default="llama-server",
        help="daemon: through paraphrase-daemon and the CLI (M1), as shipped",
    )
    ap.add_argument(
        "--build-dir", type=Path, help="daemon backend: the build to run (default build/dev)"
    )
    args = ap.parse_args(argv)
    misses: list[str] = []
    out_dir = common.REPORTS_DIR / args.tag / "bench"
    out_dir.mkdir(parents=True, exist_ok=True)
    for model_id in args.model:
        for profile in args.profile:
            print(f"{model_id} on {profile}")
            res = bench(
                model_id,
                profile,
                args.runs,
                shlex.split(args.server_args),
                args.skill_override,
                tuple(args.tasks),
                args.backend,
                args.build_dir,
            )
            res["variant"] = args.variant
            print(
                f"  load cold {res['load_cold_s']}s warm {res['load_warm_s']}s"
                f"  peak RSS {res['peak_rss_mb']} MB"
            )
            suffix = f"__{args.variant}" if args.variant else ""
            path = out_dir / f"{model_id}__{profile}{suffix}.json"
            path.write_text(json.dumps(res, indent=2) + "\n")
            if args.check:
                misses += check(model_id, profile, res)
    for m in misses:
        print("over target:", m)
    return 1 if misses else 0


def check(model_id: str, profile: str, res: dict[str, Any]) -> list[str]:
    """Results over the PLAN 5.4 targets (eval/targets.json); a pair with no target passes."""
    targets = json.loads(TARGETS.read_text())["targets"].get(f"{model_id}__{profile}")
    if not targets:
        print(f"  no targets for {model_id} on {profile}")
        return []
    out = []
    for key, limit in targets.items():
        if isinstance(limit, dict):
            if key not in res["tasks"]:
                continue
            for metric, cap in limit.items():
                got = res["tasks"][key][metric]
                if got > cap:
                    out.append(f"{model_id} {profile} {key} {metric} {got} > {cap}")
        elif res[key] > limit:
            out.append(f"{model_id} {profile} {key} {res[key]} > {limit}")
    return out


if __name__ == "__main__":
    sys.exit(main())
