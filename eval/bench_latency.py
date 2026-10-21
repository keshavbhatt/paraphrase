#!/usr/bin/env python3
"""Latency bench for the PLAN 5.4 targets: time to first token, total time, tokens/s,
peak RAM and model load time, per model and run profile (eval/machines.json).

    uv run python eval/bench_latency.py --model qwen3.5-2b-q4km --profile A-emulated B-vulkan

Inputs are the five English dataset items closest to 100 words for each task (the "typical
input" of 5.4). Each task gets one warm-up request first, as the daemon keeps each skill's
system prompt cached (5.3), then --runs timed requests on different inputs. Loads are timed
twice: from disk with the model's pages evicted from the page cache ("cold"), then again
with the file cached ("warm").
"""

from __future__ import annotations

import argparse
import json
import statistics
import sys
import time
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from paraeval import common
from paraeval.llama import LlamaServer, evict_page_cache
from paraeval.skills import load_skills

TASKS = ("reply", "proofread", "rewrite.professional")
TARGET_WORDS = 100


def bench_inputs(base_skill: str, n: int) -> list[dict[str, Any]]:
    items = [i for i in common.dataset(base_skill, ("generated",)) if i["lang"] == "en"]
    if base_skill == "rewrite":
        items = [i for i in items if i.get("tone") == "professional"]
    field = "context" if base_skill == "reply" else "text"
    items.sort(key=lambda i: abs(len(i[field].split()) - TARGET_WORDS))
    return items[: n + 1]  # one extra for the warm-up


def _median(xs: list[float]) -> float:
    return round(statistics.median(xs), 3) if xs else 0.0


def bench(model_id: str, profile_name: str, runs: int) -> dict[str, Any]:
    info = common.model(model_id)
    profile = common.profiles()[profile_name]
    skills = load_skills(common.SKILLS_DIR)
    result: dict[str, Any] = {
        "model": model_id,
        "profile": profile_name,
        "label": profile.label,
        "when": time.strftime("%Y-%m-%d %H:%M"),
    }
    evict_page_cache(info.path)
    with LlamaServer(info.path, profile, ctx=info.ctx) as cold:
        result["load_cold_s"] = round(cold.load_s, 2)
    with LlamaServer(info.path, profile, ctx=info.ctx) as server:
        result["load_warm_s"] = round(server.load_s, 2)
        tasks: dict[str, Any] = {}
        for task in TASKS:
            skill = skills[task]
            inputs = bench_inputs(skill.base_id, runs)
            samples = []
            for k, item in enumerate(inputs):
                system, user = skill.render(common.skill_inputs(skill.base_id, item))
                c = server.chat(
                    system, user, skill.sampling, skill.max_output_tokens, skill.grammar
                )
                if k == 0:
                    continue  # warm-up: fills the system-prompt cache
                samples.append(c)
            tasks[task] = {
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
                "total_s": _median([c.total_s for c in samples]),
                "total_s_max": round(max(c.total_s for c in samples), 3),
                "gen_tps": _median([c.gen_tps for c in samples]),
                "prompt_tps": _median([c.prompt_tps for c in samples]),
                "prompt_tokens": _median([c.prompt_tokens for c in samples]),
                "cached_tokens": _median([c.cached_tokens for c in samples]),
                "gen_tokens": _median([c.gen_tokens for c in samples]),
            }
            t = tasks[task]
            print(
                f"  {task:22} ttft {t['ttft_s']:.2f}s  total {t['total_s']:.2f}s"
                f"  gen {t['gen_tps']:.1f} t/s  prompt {t['prompt_tps']:.0f} t/s"
            )
        result["tasks"] = tasks
        result["peak_rss_mb"] = round(server.peak_rss_mb)
    return result


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--model", nargs="+", required=True)
    ap.add_argument("--profile", nargs="+", default=["A-emulated", "B-vulkan"])
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--tag", default="m0")
    args = ap.parse_args(argv)
    out_dir = common.REPORTS_DIR / args.tag / "bench"
    out_dir.mkdir(parents=True, exist_ok=True)
    for model_id in args.model:
        for profile in args.profile:
            print(f"{model_id} on {profile}")
            res = bench(model_id, profile, args.runs)
            print(
                f"  load cold {res['load_cold_s']}s warm {res['load_warm_s']}s"
                f"  peak RSS {res['peak_rss_mb']} MB"
            )
            path = out_dir / f"{model_id}__{profile}.json"
            path.write_text(json.dumps(res, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
