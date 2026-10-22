#!/usr/bin/env python3
"""Run skills over their eval datasets with one model and write a report.

    uv run python eval/run_eval.py --model qwen3.5-2b-q4km --skill proofread reply
    uv run python eval/run_eval.py --model gemma-4-e2b-qat-q4_0 --skill rewrite.professional \
        --profile B-vulkan --limit 5 --tag smoke

Raw outputs go to eval/reports/raw/<tag>/ (not committed); the summary to
eval/reports/<tag>/<model>__<skill>.{md,json}. Scores are split by language (en, hi,
hinglish) and by set (generated, human) so the owner's human-written items are compared
separately.
"""

from __future__ import annotations

import argparse
import json
import statistics
import sys
import time
from collections import defaultdict
from collections.abc import Callable
from dataclasses import asdict
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from paraeval import common, engine, metrics
from paraeval.daemon import Daemon
from paraeval.llama import LlamaServer
from paraeval.skills import Skill, load_skills


def items_for(skill: Skill, sets: tuple[str, ...]) -> list[dict[str, Any]]:
    items = common.dataset(skill.base_id, sets)
    tone = skill.id.split(".", 1)[1] if "." in skill.id else None
    if tone:
        items = [i for i in items if i.get("tone", tone) == tone]
    return items


def score(skill: Skill, item: dict[str, Any], output: str) -> dict[str, Any]:
    if skill.base_id == "proofread":
        return asdict(metrics.score_proofread(item, output))
    if skill.base_id == "reply":
        return asdict(metrics.score_reply(item, output))
    if skill.base_id == "rewrite":
        return asdict(metrics.score_rewrite(item, output))
    return {}


def _pct(n: int, d: int) -> float:
    return round(100 * n / d, 1) if d else 0.0


def summarise(skill: Skill, rows: list[dict[str, Any]]) -> dict[str, Any]:
    if not rows:
        return {"n": 0}
    s = [r["score"] for r in rows]
    n = len(rows)
    out: dict[str, Any] = {
        "n": n,
        "lang_floor_fail": [r["id"] for r in rows if not r["score"].get("lang_ok", True)],
        "ttft_s_median": round(statistics.median(r["ttft_s"] for r in rows), 3),
        "total_s_median": round(statistics.median(r["total_s"] for r in rows), 3),
        "gen_tps_median": round(statistics.median(r["gen_tps"] for r in rows), 1),
        # Engine language check (skill schema v2): retried once, still failing after it.
        "retried_pct": _pct(sum(r.get("retried", False) for r in rows), n),
        "lang_warned": [r["id"] for r in rows if r.get("warning")],
    }
    if skill.base_id == "proofread":
        tp, fp, fn = (sum(x[k] for x in s) for k in ("tp", "fp", "fn"))
        p, r, f = metrics.f_beta(tp, fp, fn)
        clean = [x for x in s if x["clean_input"]]
        kept, prot = sum(x["kept"] for x in s), sum(x["protected"] for x in s)
        out |= {
            "precision": round(p, 3),
            "recall": round(r, 3),
            "f0_5": round(f, 3),
            "exact_pct": _pct(sum(x["exact"] for x in s), n),
            "false_alarm_pct": _pct(sum(x["false_alarm"] for x in clean), len(clean)),
            "clean_items": len(clean),
            "protected_kept_pct": _pct(kept, prot),
            "preamble_pct": _pct(sum(x["preamble"] for x in s), n),
        }
    elif skill.base_id == "reply":
        valid = [x for x in s if x["valid"]]
        out |= {
            "valid_pct": _pct(len(valid), n),
            "distinct_pct": _pct(sum(x["distinct"] for x in valid), len(valid)),
            "over_15_words_pct": _pct(sum(x["max_words"] > 15 for x in valid), len(valid)),
        }
        drafts = [x for x in s if x.get("draft_kept") is not None]
        if drafts:
            out["draft_kept_pct"] = _pct(sum(x["draft_kept"] for x in drafts), len(drafts))
        injected = [x for x in s if x.get("obeyed") is not None]
        if injected:
            out["obeyed_injection_pct"] = _pct(sum(x["obeyed"] for x in injected), len(injected))
    elif skill.base_id == "rewrite":
        kept, prot = sum(x["kept"] for x in s), sum(x["protected"] for x in s)
        out |= {
            "changed_pct": _pct(sum(x["changed"] for x in s), n),
            "protected_kept_pct": _pct(kept, prot),
            "preamble_pct": _pct(sum(x["preamble"] for x in s), n),
            "length_ratio_median": round(statistics.median(x["length_ratio"] for x in s), 2),
        }
    return out


def split(rows: list[dict[str, Any]], key: Callable[[dict[str, Any]], str]) -> dict[str, list[Any]]:
    groups: dict[str, list[Any]] = defaultdict(list)
    for r in rows:
        groups[key(r)].append(r)
    return dict(sorted(groups.items()))


def report_md(model_id: str, skill: Skill, profile: str, summary: dict[str, Any]) -> str:
    lines = [
        f"# {skill.id} with {model_id}",
        "",
        f"Profile `{profile}`, skill version {skill.version}, run {summary['when']}.",
        "",
    ]
    groups: dict[str, dict[str, Any]] = summary["groups"]
    keys = [k for k in next(iter(groups.values())) if k not in ("lang_floor_fail", "lang_warned")]
    lines.append("| group | " + " | ".join(keys) + " | language floor failures |")
    lines.append("|---" * (len(keys) + 2) + "|")
    for name, g in groups.items():
        if not g.get("n"):
            continue
        vals = " | ".join(str(g.get(k, "")) for k in keys)
        fails = ", ".join(g["lang_floor_fail"]) or "none"
        lines.append(f"| {name} | {vals} | {fails} |")
    return "\n".join(lines) + "\n"


def write_summary(
    model_id: str,
    skill: Skill,
    profile_name: str,
    tag: str,
    rows: list[dict[str, Any]],
    meta: dict[str, Any],
) -> dict[str, Any]:
    groups = {"all": summarise(skill, rows)}
    for lang, rs in split(rows, lambda r: r["lang"]).items():
        groups[f"lang={lang}"] = summarise(skill, rs)
    for src, rs in split(rows, lambda r: r["source"]).items():
        groups[f"set={src}"] = summarise(skill, rs)
    summary = {
        "model": model_id,
        "skill": skill.id,
        "skill_version": skill.version,
        "skill_revision": skill.revision,
        "profile": profile_name,
        **meta,
        "tag": tag,
        "when": time.strftime("%Y-%m-%d %H:%M"),
        "groups": groups,
    }
    out_dir = common.REPORTS_DIR / tag
    out_dir.mkdir(parents=True, exist_ok=True)
    # Model ids contain dots, so names are built as strings, never with Path.with_suffix.
    name = f"{model_id}__{skill.id}"
    (out_dir / f"{name}.json").write_text(json.dumps(summary, indent=2) + "\n")
    (out_dir / f"{name}.md").write_text(report_md(model_id, skill, profile_name, summary))
    return summary


def rescore(model_id: str, skill_ids: list[str], tag: str, profile_name: str) -> None:
    """Recompute scores and summaries from saved raw outputs (after a metric change)."""
    skills = load_skills(common.SKILLS_DIR)
    for skill_id in skill_ids:
        skill = skills[skill_id]
        items = {i["id"]: i for i in common.dataset(skill.base_id)}
        path = common.RAW_DIR / tag / f"{model_id}__{skill_id}.jsonl"
        rows = common.load_jsonl(path)
        for r in rows:
            r["score"] = score(skill, items[r["id"]], r["output"])
        common.write_jsonl(path, rows)
        summary = write_summary(model_id, skill, profile_name, tag, rows, {"rescored": True})
        print(model_id, skill_id, json.dumps(summary["groups"]["all"], ensure_ascii=False))


def run(
    model_id: str,
    skill_ids: list[str],
    profile_name: str,
    sets: tuple[str, ...],
    limit: int | None,
    tag: str,
    overrides: list[Path] | None = None,
    backend: str = "llama-server",
    build_dir: Path | None = None,
) -> list[dict[str, Any]]:
    skills = load_skills(common.SKILLS_DIR, overrides)
    info = common.model(model_id)
    profile = common.profiles()[profile_name]
    summaries = []
    parallel = max(skills[s].variants for s in skill_ids)
    if backend == "daemon" and overrides:
        raise SystemExit(
            "--skill-override needs the llama-server backend: the daemon reads skills/"
        )
    server: LlamaServer | Daemon = (
        Daemon(model_id, profile, build_dir)
        if backend == "daemon"
        else LlamaServer(info.path, profile, ctx=info.ctx, parallel=parallel)
    )
    with server:
        print(f"{model_id} on {profile_name}: loaded in {server.load_s:.2f} s")
        for skill_id in skill_ids:
            skill = skills[skill_id]
            items = items_for(skill, sets)[:limit] if limit else items_for(skill, sets)
            if not items:
                raise SystemExit(f"no dataset items for {skill_id}")
            rows = []
            for i, item in enumerate(items, 1):
                values = common.skill_inputs(skill.base_id, item)
                res = (
                    server.run(skill, values)
                    if isinstance(server, Daemon)
                    else engine.run(server, skill, values)
                )
                c = res.completion
                rows.append(
                    {
                        "id": item["id"],
                        "lang": item["lang"],
                        "source": item["source"],
                        "output": res.output,
                        "finish": c.finish_reason,
                        "ttft_s": res.calls[0].ttft_s,
                        "total_s": res.total_s,
                        "gen_tps": c.gen_tps,
                        "prompt_tokens": c.prompt_tokens,
                        "gen_tokens": c.gen_tokens,
                        "retried": res.retried,
                        "warning": res.warning,
                        "score": score(skill, item, res.output),
                    }
                )
                print(f"\r  {skill_id}: {i}/{len(items)}", end="", flush=True)
            print()
            common.write_jsonl(common.RAW_DIR / tag / f"{model_id}__{skill_id}.jsonl", rows)
            meta = {
                "load_s": round(server.load_s, 2),
                "peak_rss_mb": round(server.peak_rss_mb),
                "backend": backend,
            }
            summary = write_summary(model_id, skill, profile_name, tag, rows, meta)
            print(json.dumps(summary["groups"]["all"], ensure_ascii=False))
            summaries.append(summary)
    return summaries


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--model", required=True)
    ap.add_argument("--skill", nargs="+", required=True)
    ap.add_argument("--profile", default="B-vulkan")
    ap.add_argument(
        "--set",
        choices=("generated", "hard", "human", "holdout", "all"),
        default="all",
        help="all: generated, hard and human; holdout is kept out of tuning and runs alone",
    )
    ap.add_argument("--limit", type=int)
    ap.add_argument("--tag", default="m0")
    ap.add_argument("--rescore", action="store_true", help="re-score saved outputs, no model")
    ap.add_argument(
        "--skill-override",
        type=Path,
        action="append",
        default=[],
        help="a candidate skill file that replaces the shipped skills it defines",
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
    if args.rescore:
        rescore(args.model, args.skill, args.tag, args.profile)
        return 0
    sets = ("generated", "hard", "human") if args.set == "all" else (args.set,)
    run(
        args.model,
        args.skill,
        args.profile,
        sets,
        args.limit,
        args.tag,
        args.skill_override,
        args.backend,
        args.build_dir,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
