#!/usr/bin/env python3
"""Blind spot-check for the owner: 20 items per model, the same items for every model.

    uv run python eval/spotcheck.py build --tag m0-v3 [--skill-tag proofread=m0-v4]
        -> eval/reports/<tag>/spotcheck/index.html (blind) and key.json (which model is which)
        --skill-tag takes one skill's outputs from another run (its newest prompt revision).
    uv run python eval/spotcheck.py score --tag m0-v2 ratings.json
        -> per-model scores from the ratings the page exports

The page shows each item once with every model's output under a random letter, in a random
order per item. Ratings (good / ok / bad, plus a note) are kept in the browser and exported
as JSON. The owner's ratings are the final word on quality (owner, 2026-10-21).
"""

from __future__ import annotations

import argparse
import html
import json
import random
import string
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from paraeval import common

# Items per skill and language: 7 proofread, 7 reply, 6 rewrite = 20 per model.
PLAN = {
    "proofread": {"en": 5, "hi": 1, "hinglish": 1},
    "reply": {"en": 5, "hi": 1, "hinglish": 1},
    "rewrite.professional": {"en": 4, "hi": 1, "hinglish": 1},
}
SEED = 20261022


def pick(skill: str, rng: random.Random) -> list[dict[str, Any]]:
    items = common.dataset(skill.split(".")[0], ("generated",))
    chosen = []
    for lang, n in PLAN[skill].items():
        pool = [i for i in items if i["lang"] == lang]
        chosen += rng.sample(pool, min(n, len(pool)))
    return chosen


def show(skill: str, output: str) -> str:
    if skill == "reply":
        try:
            return "\n".join(f"• {r}" for r in json.loads(output)["replies"])
        except (json.JSONDecodeError, KeyError, TypeError):
            return output
    return output


def build(tag: str, models: list[str], skill_tags: dict[str, str] | None = None) -> Path:
    rng = random.Random(SEED)
    src = {s: (skill_tags or {}).get(s, tag) for s in PLAN}
    raw = {
        (m, s): {r["id"]: r for r in common.load_jsonl(common.RAW_DIR / src[s] / f"{m}__{s}.jsonl")}
        for m in models
        for s in PLAN
    }
    cards = []
    key: dict[str, dict[str, str]] = {}
    for skill in PLAN:
        for item in pick(skill, rng):
            letters = list(string.ascii_uppercase[: len(models)])
            order = models[:]
            rng.shuffle(order)
            key[item["id"]] = dict(zip(letters, order, strict=True))
            outputs = [
                {"letter": letter, "text": show(skill, raw[(m, skill)][item["id"]]["output"])}
                for letter, m in zip(letters, order, strict=True)
            ]
            cards.append(
                {
                    "id": item["id"],
                    "skill": skill,
                    "lang": item["lang"],
                    "input": item.get("context") or item["text"],
                    "draft": item.get("text", "") if skill == "reply" else "",
                    "reference": item.get("reference", "") if skill == "proofread" else "",
                    "outputs": outputs,
                }
            )
    out_dir = common.REPORTS_DIR / tag / "spotcheck"
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "key.json").write_text(json.dumps(key, indent=2) + "\n")
    (out_dir / "sources.json").write_text(json.dumps(src, indent=2) + "\n")
    data = json.dumps({"tag": tag, "cards": cards}, ensure_ascii=False).replace("</", "<\\/")
    page = PAGE.replace("__DATA__", data).replace("__TITLE__", html.escape(f"Spot-check {tag}"))
    (out_dir / "index.html").write_text(page, encoding="utf-8")
    return out_dir


def score(tag: str, ratings_path: Path) -> None:
    key = json.loads((common.REPORTS_DIR / tag / "spotcheck" / "key.json").read_text())
    ratings = json.loads(ratings_path.read_text())["ratings"]
    value = {"good": 1.0, "ok": 0.5, "bad": 0.0}
    per: dict[str, list[float]] = defaultdict(list)
    per_skill: dict[tuple[str, str], list[float]] = defaultdict(list)
    for item_id, by_letter in ratings.items():
        for letter, r in by_letter.items():
            if r.get("rating") in value:
                model = key[item_id][letter]
                per[model].append(value[r["rating"]])
                skill = {"pr": "proofread", "rp": "reply", "rw": "rewrite"}[item_id[:2]]
                per_skill[(model, skill)].append(value[r["rating"]])
    for model, vals in sorted(per.items(), key=lambda kv: -sum(kv[1]) / len(kv[1])):
        skills = {
            s: round(sum(v) / len(v), 2) for (m, s), v in sorted(per_skill.items()) if m == model
        }
        print(f"{model:24} {sum(vals) / len(vals):.2f} over {len(vals)} ratings  {skills}")


PAGE = (Path(__file__).resolve().parent / "paraeval" / "spotcheck.html").read_text()


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build")
    b.add_argument("--tag", default="m0-v2")
    b.add_argument("--skill-tag", action="append", default=[], metavar="SKILL=TAG")
    s = sub.add_parser("score")
    s.add_argument("--tag", default="m0-v2")
    s.add_argument("ratings", type=Path)
    args = ap.parse_args(argv)
    if args.cmd == "build":
        models = [m.id for m in common.catalog().values() if m.role == "candidate"]
        skill_tags = dict(x.split("=", 1) for x in args.skill_tag)
        unknown = set(skill_tags) - set(PLAN)
        if unknown:
            ap.error(f"unknown skill in --skill-tag: {', '.join(sorted(unknown))}")
        print(build(args.tag, models, skill_tags) / "index.html")
    else:
        score(args.tag, args.ratings)
    return 0


if __name__ == "__main__":
    sys.exit(main())
