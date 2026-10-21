#!/usr/bin/env python3
"""Build the M0 results tables (Markdown) from saved eval, bench and judge reports.

uv run python eval/summary_table.py --tag m0-v2 [--before m0-v1] > eval/reports/m0-v2/summary.md
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from paraeval import common

SKILLS = ("proofread", "reply", "rewrite.professional")
TARGETS = {  # PLAN 5.4: (A ttft, A total, B total) in seconds
    "reply": (0.8, 2.0, 0.8),
    "proofread": (0.8, 3.5, 1.2),
    "rewrite.professional": (1.2, 5.0, 1.5),
}


def load(path: Path) -> dict[str, Any] | None:
    return json.loads(path.read_text()) if path.is_file() else None


def report(tag: str, model: str, skill: str) -> dict[str, Any] | None:
    return load(common.REPORTS_DIR / tag / f"{model}__{skill}.json")


def candidates() -> list[str]:
    return [m.id for m in common.catalog().values() if m.role == "candidate"]


def floor(r: dict[str, Any] | None) -> str:
    if not r:
        return "-"
    g = r["groups"]
    parts = []
    for lang in ("en", "hi", "hinglish"):
        grp = g.get(f"lang={lang}")
        if grp:
            parts.append(f"{len(grp['lang_floor_fail'])}/{grp['n']}")
    return " · ".join(parts)


def quality_tables(tag: str, before: str | None) -> list[str]:
    out = ["## Quality", ""]
    out += [
        "Language floor failures are shown as en · hi · hinglish (failed/items). A model with any "
        "hi or hinglish failure misses the owner's hard floor.",
        "",
        "### Proofread",
        "",
        "| model | F0.5 en | F0.5 hi | F0.5 hinglish | exact % | false alarms on clean text | "
        "protected kept % | preamble % | language floor |",
        "|---|---|---|---|---|---|---|---|---|",
    ]
    for m in candidates():
        r = report(tag, m, "proofread")
        if not r:
            continue
        g = r["groups"]
        f = [g.get(f"lang={x}", {}).get("f0_5", "-") for x in ("en", "hi", "hinglish")]
        a = g["all"]
        out.append(
            f"| {m} | {f[0]} | {f[1]} | {f[2]} | {a['exact_pct']} | "
            f"{a['false_alarm_pct']}% of {a['clean_items']} | {a['protected_kept_pct']} | "
            f"{a['preamble_pct']} | {floor(r)} |"
        )
    out += [
        "",
        "### Reply",
        "",
        "| model | valid JSON % | 3 distinct % | over 15 words % | language floor |",
        "|---|---|---|---|---|",
    ]
    for m in candidates():
        r = report(tag, m, "reply")
        if not r:
            continue
        a = r["groups"]["all"]
        out.append(
            f"| {m} | {a['valid_pct']} | {a['distinct_pct']} | {a['over_15_words_pct']} | "
            f"{floor(r)} |"
        )
    out += [
        "",
        "### Rewrite (professional)",
        "",
        "| model | changed % | protected kept % | preamble % | length ratio | language floor |",
        "|---|---|---|---|---|---|",
    ]
    for m in candidates():
        r = report(tag, m, "rewrite.professional")
        if not r:
            continue
        a = r["groups"]["all"]
        out.append(
            f"| {m} | {a['changed_pct']} | {a['protected_kept_pct']} | {a['preamble_pct']} | "
            f"{a['length_ratio_median']} | {floor(r)} |"
        )
    if before:
        out += [
            "",
            f"### Language floor before and after the prompt revision ({before} -> {tag})",
            "",
            "| model | skill | before | after |",
            "|---|---|---|---|",
        ]
        for m in candidates():
            for s in SKILLS:
                b, a = report(before, m, s), report(tag, m, s)
                if b and a:
                    out.append(f"| {m} | {s} | {floor(b)} | {floor(a)} |")
    return out


def judge_tables(tag: str) -> list[str]:
    out: list[str] = []
    for s in ("reply", "rewrite.professional"):
        j = load(common.REPORTS_DIR / tag / "judge" / f"{s}.json")
        if not j:
            continue
        out += [
            "",
            f"### Judge: {s} ({j['judge']}, blind pairwise, both orders, {j['items']} items)",
            "",
            "| model | win rate | en | hi | hinglish |",
            "|---|---|---|---|---|",
        ]
        for m, w in j["win_rate"].items():
            bl = j["win_rate_by_lang"].get(m, {})
            out.append(
                f"| {m} | {w} | {bl.get('en', '-')} | {bl.get('hi', '-')} | "
                f"{bl.get('hinglish', '-')} |"
            )
    return out


def mark(value: float, target: float) -> str:
    return f"{value:.2f}" + ("" if value <= target else " ✗")


def bench_tables(tag: str) -> list[str]:
    bench_dir = common.REPORTS_DIR / tag / "bench"
    if not bench_dir.is_dir():
        return []
    profiles = common.profiles()
    out = [
        "",
        "## Latency",
        "",
        "Median of 5 runs on about 100-word inputs, after one warm-up per task (the daemon keeps "
        "each skill's system prompt cached). ✗ marks a miss against the PLAN 5.4 target "
        "(A: time to first token and total; B: total). Times in seconds.",
    ]
    for pname, prof in profiles.items():
        rows = [load(bench_dir / f"{m}__{pname}.json") for m in candidates()]
        if not any(rows):
            continue
        out += [
            "",
            f"### {prof.label}",
            "",
            "| model | reply ttft / total | proofread ttft / total | rewrite ttft / total | "
            "gen tok/s | prompt tok/s | load cold / warm | peak RSS MB |",
            "|---|---|---|---|---|---|---|---|",
        ]
        for m, r in zip(candidates(), rows, strict=True):
            if not r:
                continue
            cells = []
            for s in SKILLS:
                t = r["tasks"][s]
                a_ttft, a_total, b_total = TARGETS[s]
                if pname.startswith("A"):
                    cells.append(f"{mark(t['ttft_s'], a_ttft)} / {mark(t['total_s'], a_total)}")
                else:
                    cells.append(f"{t['ttft_s']:.2f} / {mark(t['total_s'], b_total)}")
            gen = r["tasks"]["reply"]["gen_tps"]
            pp = r["tasks"]["proofread"]["prompt_tps"]
            out.append(
                f"| {m} | {' | '.join(cells)} | {gen} | {pp:.0f} | "
                f"{r['load_cold_s']} / {r['load_warm_s']} | {r['peak_rss_mb']} |"
            )
    return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--tag", default="m0-v2")
    ap.add_argument("--before")
    args = ap.parse_args(argv)
    lines = [f"# M0 results ({args.tag})", ""]
    lines += quality_tables(args.tag, args.before)
    lines += judge_tables(args.tag)
    lines += bench_tables(args.tag)
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(main())
