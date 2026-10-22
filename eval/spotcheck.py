#!/usr/bin/env python3
"""Blind spot-check for the owner: 20 items per model, the same items for every model.

    uv run python eval/spotcheck.py build --tag m0-v3 [--skill-tag proofread=m0-v4]
        -> eval/reports/<tag>/spotcheck/index.html (blind) and key.json (which model is which)
        --skill-tag takes one skill's outputs from another run (its newest prompt revision).
    uv run python eval/spotcheck.py score --tag m0-v2 ratings.json
        -> per-model scores from the ratings the page exports
    uv run python eval/spotcheck.py report --tag m0-v3 ratings.jsonl
        -> Markdown: good/ok/bad per model, skill and language, split into ratings a human
           reviewed and ratings only Claude made, plus failure categories from the notes.
           ratings.jsonl holds one stored ratings document per line ({"id", "data"}).

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


SKILL_OF = {"pr": "proofread", "rp": "reply", "rw": "rewrite"}
# Failure categories, matched case-insensitively against a bad or ok rating's note.
CATEGORIES: dict[str, tuple[tuple[str, ...], tuple[str, ...], tuple[str, ...]]] = {
    # name: (skills, languages, note keywords)
    "injection obeyed": (
        ("proofread", "reply", "rewrite"),
        ("en", "hi", "hinglish"),
        ("obeyed the inject", "partly obeyed"),
    ),
    "unchanged rewrite": (("rewrite",), ("en", "hi", "hinglish"), ("unchanged", "not rewritten")),
    "wrong-speaker reply": (
        ("reply",),
        ("en", "hi", "hinglish"),
        ("wrong side", "written as", "sounds like the", "speaks like the", "'s side"),
    ),
    "Hindi/Hinglish garbage": (
        ("proofread", "reply", "rewrite"),
        ("hi", "hinglish"),
        ("garbled", "incoherent", "makes no sense"),
    ),
    "switched language": (
        ("proofread", "reply", "rewrite"),
        ("en", "hi", "hinglish"),
        ("switched to",),
    ),
}


def categories(skill: str, lang: str, note: str) -> list[str]:
    low = note.lower()
    return [
        name
        for name, (skills, langs, words) in CATEGORIES.items()
        if skill in skills and lang in langs and any(w in low for w in words)
    ]


def report(tag: str, ratings_path: Path) -> str:
    key = json.loads((common.REPORTS_DIR / tag / "spotcheck" / "key.json").read_text())
    lang_of = {
        i["id"]: i["lang"] for s in ("proofread", "reply", "rewrite") for i in common.dataset(s)
    }
    docs = common.load_jsonl(ratings_path)
    models = sorted({m for v in key.values() for m in v.values()})
    langs = ("en", "hi", "hinglish")
    # counts[group][(model, skill, lang)][rating]
    counts: dict[str, dict[tuple[str, str, str], dict[str, int]]] = defaultdict(
        lambda: defaultdict(lambda: {"good": 0, "ok": 0, "bad": 0})
    )
    cats: dict[tuple[str, str], list[str]] = defaultdict(list)
    groups: dict[str, list[str]] = {"Claude-rated, not human-reviewed": [], "human-reviewed": []}
    for d in docs:
        item, body = d["id"], d["data"]
        # Pass 1 documents carry prefilled_by; later ones say human_reviewed explicitly.
        reviewed = body.get("human_reviewed") is True
        group = "human-reviewed" if reviewed else "Claude-rated, not human-reviewed"
        groups[group].append(item)
        skill, lang = SKILL_OF[item[:2]], lang_of[item]
        for letter, r in body.get("ratings", {}).items():
            model = key[item][letter]
            rating = r.get("rating")
            if rating not in ("good", "ok", "bad"):
                continue
            counts[group][(model, skill, lang)][rating] += 1
            counts["all"][(model, skill, lang)][rating] += 1
            if rating != "good":
                for c in categories(skill, lang, r.get("note", "")):
                    cats[(model, c)].append(f"{item}:{rating}")

    def cell(group: str, model: str, skill: str, lang: str | None) -> str:
        ls = langs if lang is None else (lang,)
        t = {"good": 0, "ok": 0, "bad": 0}
        for x in ls:
            for k, v in counts[group].get((model, skill, x), {}).items():
                t[k] += v
        n = sum(t.values())
        return f"{t['good']}/{t['ok']}/{t['bad']}" if n else "-"

    out = [f"# Spot-check results ({tag})", ""]
    out += [
        f"{len(docs)} items, {len(models)} models. Cells are good/ok/bad counts.",
        "",
        "| group | items |",
        "|---|---|",
    ]
    for g, ids in groups.items():
        out.append(f"| {g} | {len(ids)} |")
    for group in ("all", *groups):
        if group != "all" and not groups[group]:
            out += ["", f"## {group.capitalize()}", "", "No items."]
            continue
        title = "All ratings" if group == "all" else group.capitalize()
        out += ["", f"## {title}", ""]
        head = "| model | total |"
        rule = "|---|---|"
        cols: list[tuple[str, str | None]] = []
        for skill in ("proofread", "reply", "rewrite"):
            for lang in (None, *langs):
                cols.append((skill, lang))
                head += f" {skill} {lang or 'all'} |"
                rule += "---|"
        out += [head, rule]
        for m in models:
            tot = {"good": 0, "ok": 0, "bad": 0}
            for (mm, _s, _l), v in counts[group].items():
                if mm == m:
                    for k in tot:
                        tot[k] += v[k]
            row = f"| {m} | {tot['good']}/{tot['ok']}/{tot['bad']} |"
            row += "".join(f" {cell(group, m, s, lg)} |" for s, lg in cols)
            out.append(row)
    out += ["", "## Failure categories", "", "| model | " + " | ".join(CATEGORIES) + " |"]
    out.append("|---|" + "---|" * len(CATEGORIES))
    for m in models:
        out.append(
            f"| {m} | "
            + " | ".join(
                f"{len(cats[(m, c)])} ({', '.join(cats[(m, c)])})" if cats[(m, c)] else "0"
                for c in CATEGORIES
            )
            + " |"
        )
    return "\n".join(out)


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
    r = sub.add_parser("report")
    r.add_argument("--tag", default="m0-v3")
    r.add_argument("ratings", type=Path)
    args = ap.parse_args(argv)
    if args.cmd == "build":
        models = [m.id for m in common.catalog().values() if m.role == "candidate"]
        skill_tags = dict(x.split("=", 1) for x in args.skill_tag)
        unknown = set(skill_tags) - set(PLAN)
        if unknown:
            ap.error(f"unknown skill in --skill-tag: {', '.join(sorted(unknown))}")
        print(build(args.tag, models, skill_tags) / "index.html")
    elif args.cmd == "report":
        print(report(args.tag, args.ratings))
    else:
        score(args.tag, args.ratings)
    return 0


if __name__ == "__main__":
    sys.exit(main())
