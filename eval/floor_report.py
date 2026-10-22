#!/usr/bin/env python3
"""Hindi and Hinglish floor (owner, 2026-10-22): over every Hindi and Hinglish eval item,
reject a model only if more than 10% of its outputs switch language or are garbled.

    uv run python eval/floor_report.py --tag m0-v5 --model gemma-4-e2b-qat-q4_0 ...
        [--review eval/reports/m0-v5/floor-review.json]

Switched language: the output still fails the engine's language check after its one retry
(the `lang_ok` score, PLAN 11 Q6). Garbled: an automatic check below, or a reviewer's mark in
the optional review file ({"<model>": {"<skill>": {"<item id>": "why"}}}), which records who
reviewed (`"reviewed_by"`). Counted per output: one reply set counts once.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from paraeval import common, metrics

SKILLS = ("proofread", "reply", "rewrite.professional")
LANGS = ("hi", "hinglish")
LIMIT = 0.10


def texts_of(skill: str, output: str) -> list[str]:
    if skill == "reply":
        return metrics.parse_replies(output) or [output]
    return [output]


def garbled_auto(skill: str, row: dict[str, Any]) -> str:
    """Mechanical signs of a broken output. Meaning-level garbling needs a reader."""
    texts = [t.strip() for t in texts_of(skill, row["output"])]
    if not any(texts):
        return "empty"
    if any("�" in t for t in texts):
        return "replacement character"
    if skill == "reply" and len(set(texts)) < len(texts):
        return "repeated reply"
    if skill != "reply" and row.get("finish") == "length":
        return "cut off at the length limit"
    for t in texts:
        words = t.split()
        grams = [" ".join(words[i : i + 4]) for i in range(len(words) - 3)]
        if grams and max(grams.count(g) for g in set(grams)) >= 3:
            return "repeated phrase loop"
        lines = [ln.strip() for ln in t.splitlines() if ln.strip()]
        if len(lines) > 1 and len(set(lines)) < len(lines):
            return "repeated line"
        if re.search(r"(\S)\1{6,}", t):
            return "repeated character run"
    return ""


def report(tag: str, models: list[str], review: dict[str, Any]) -> str:
    out = [
        f"# Hindi and Hinglish floor ({tag})",
        "",
        "Rejected when more than 10% of a model's Hindi or Hinglish outputs switch language or "
        "are garbled. Cells: failures / outputs (switched + garbled).",
        "",
        f"Garbled marks by a reader: {review.get('reviewed_by', 'none')}.",
        "",
        "| model | "
        + " | ".join(f"{s} {lang}" for s in SKILLS for lang in LANGS)
        + " | total | verdict |",
        "|---|" + "---|" * (len(SKILLS) * len(LANGS) + 2),
    ]
    details: list[str] = []
    for m in models:
        cells, fails, total = [], 0, 0
        for s in SKILLS:
            rows = common.load_jsonl(common.RAW_DIR / tag / f"{m}__{s}.jsonl")
            marks = review.get(m, {}).get(s, {})
            for lang in LANGS:
                rs = [r for r in rows if r["lang"] == lang]
                sw = ga = 0
                for r in rs:
                    why_auto = garbled_auto(s, r)
                    why = why_auto or marks.get(r["id"], "")
                    if not r["score"].get("lang_ok", True):
                        sw += 1
                        reason = r["score"].get("lang_reason", "")
                        details.append(f"| {m} | {s} | {r['id']} | switched | {reason} |")
                    elif why:
                        ga += 1
                        src = "auto" if why_auto else "reader"
                        details.append(f"| {m} | {s} | {r['id']} | garbled ({src}) | {why} |")
                cells.append(f"{sw + ga} / {len(rs)} ({sw} + {ga})" if rs else "-")
                fails += sw + ga
                total += len(rs)
        share = fails / total if total else 0.0
        verdict = "reject" if share > LIMIT else "pass"
        out.append(
            f"| {m} | " + " | ".join(cells) + f" | {fails} / {total} ({share:.0%}) | {verdict} |"
        )
    out += ["", "| model | skill | item | failure | why |", "|---|---|---|---|---|", *details]
    return "\n".join(out) + "\n"


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--tag", required=True)
    ap.add_argument("--model", nargs="+", required=True)
    ap.add_argument("--review", type=Path)
    args = ap.parse_args(argv)
    review = json.loads(args.review.read_text()) if args.review else {}
    print(report(args.tag, args.model, review), end="")
    return 0


if __name__ == "__main__":
    sys.exit(main())
