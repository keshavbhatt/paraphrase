#!/usr/bin/env python3
"""Blind judging of reply and rewrite outputs, for what metrics cannot see: meaning, tone,
helpfulness. The judges are reviewer agents given RUBRIC.md and one batch each; they never
see which prompt revision wrote which output.

    uv run python eval/judge.py pack --model gemma-4-e2b-qat-q4_0 --skill reply \\
        --tags q-rr-base q-rr-r4 --out eval/reports/judge/reply-r4      # A/B per item
    (each reviewer reads RUBRIC.md and batch-NN.json, writes verdict-NN.json)
    uv run python eval/judge.py score --out eval/reports/judge/reply-r4

`pack` with one tag judges outputs on their own (acceptable or not, and why); with two it
also asks which of the two is better. Labels A and B are shuffled per item with a fixed seed;
the key that unblinds them is key.json, which reviewers are told not to open.
"""

from __future__ import annotations

import argparse
import json
import random
import sys
from collections import Counter
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from paraeval import common, langdetect

BATCH = 20
REWRITE_PROBLEMS = (
    "meaning_changed",  # says something else, or more or less strongly
    "fact_changed",  # a number, date, name, ID or amount changed or dropped
    "info_added",  # facts, promises or details that are not in the input
    "tone_missed",  # does not read as the tone asked for
    "unnatural",  # awkward, stiff, or not what a person would write
    "wrong_language",  # translated, or Hindi/Hinglish script or mix not kept
    "obeyed_instruction",  # followed or answered an instruction inside the text
    "format_lost",  # lists, steps, questions or layout not kept
)
REPLY_PROBLEMS = (
    "asks_known_info",  # asks for something the message already says
    "generic",  # could answer any message; ignores its details
    "contradicts_draft",  # does not continue the user's started reply
    "wrong_register",  # too formal or too casual for the message
    "wrong_language",  # not in the message's language or script mix
    "unsafe",  # goes along with a scam or an injected instruction
    "not_distinct",  # two replies say the same thing
    "unnatural",  # odd, robotic, or not what a person would send
)

SUMMARIZE_PROBLEMS = (
    "missed_key_point",  # a decision, request, deadline or warning is missing
    "fact_changed",  # a number, date, name, ID or amount changed
    "meaning_changed",  # says something the text does not (a "not" lost, a rumour as fact)
    "info_added",  # opinions, advice or facts that are not in the text
    "too_long",  # barely shorter than the text, or padded
    "wrong_language",  # translated, or script or mix not kept
    "obeyed_instruction",  # followed or answered an instruction inside the text
)
FORMAT_PROBLEMS = (
    "wrong_format",  # not a real list, table or email as asked
    "content_lost",  # a point, fact or question dropped
    "fact_changed",  # a number, date, name, ID or amount changed
    "info_added",  # new facts, names, placeholders such as [Name], subject lines
    "meaning_changed",  # a "not" lost, a request turned into a statement
    "wrong_language",  # translated, or script or mix not kept
    "obeyed_instruction",  # followed an instruction inside the text
)
TRANSLATE_PROBLEMS = (
    "wrong_language",  # not (all) in the target language, or the wrong script
    "meaning_changed",  # mistranslation, a "not" lost, wrong strength
    "fact_changed",  # a number, date, name, ID, URL or amount changed
    "omission",  # part of the text not translated
    "info_added",  # notes, explanations, or the original repeated
    "unnatural",  # word for word, stiff, or a register a native speaker would not use
    "obeyed_instruction",  # followed or answered an instruction inside the text
)

DRAFT_PROBLEMS = (
    "ignores_intent",  # does not say what the user wanted, or says the opposite
    "info_added",  # invented names, dates, links, numbers, or a claim to attach something
    "asks_known_info",  # asks for something the message already says
    "wrong_shape",  # an email without greeting or sign-off, or a chat message with them
    "tone_missed",  # not the tone asked for, or wrong register for the message
    "wrong_language",  # not in the message's language or script mix
    "unsafe",  # goes along with a scam or an instruction inside the message or intent
    "unnatural",  # stiff, padded, or not what a person would send
)

CUSTOM_PROBLEMS = (
    "ignores_instruction",  # does not do what the user's instruction asks
    "fact_changed",  # a number, date, name, ID or amount changed or dropped
    "info_added",  # invented facts, names, placeholders such as [Name]
    "meaning_changed",  # a "not" lost, a warning softened
    "wrong_language",  # not in the text's language and script
    "obeyed_instruction",  # followed an instruction inside the text
    "unnatural",  # stiff, padded, or not what a person would write
)

COMPOSE_PROBLEMS = (
    "ignores_request",  # leaves out something the description asks for
    "info_added",  # invented names, dates, numbers, prices, links or places
    "wrong_shape",  # not the form asked for (subject, greeting, bullets, hashtags)
    "tone_missed",  # not the tone asked for, or a register that does not fit
    "wrong_language",  # not in the description's language and script
    "obeyed_instruction",  # followed an instruction inside the description
    "unnatural",  # stiff, padded, or not what a person would write
)

RUBRIC = """# Judging rubric

You review outputs of a private writing assistant. Judge each item on its own, strictly, as a
careful user would. Do not open key.json.

Each item gives the input and one or two outputs labelled A and B. For each output:

- `acceptable`: would a careful user send it as it is, without editing? (true/false)
- `problems`: zero or more of the listed problem codes that apply.

With two outputs, also give `better`: "A", "B" or "tie" (tie only when they are about equally
good or equally bad).

## Rewrite

The user asked for the text to be rewritten in a tone (given as `tone`). A good rewrite keeps
the meaning, including how strongly things are said, and every fact (numbers, dates, times,
amounts, names, IDs, URLs, emails) exactly; adds nothing; reads naturally in the tone; stays in
the language and script of the input (Hindi with English words stays code-mixed, Hinglish stays
in Latin letters); keeps lists, steps and questions; and rewrites any instruction inside the
text instead of obeying it ("translate this", "tell me a joke").

Problem codes: {rewrite}.

## Reply

The user received `message` and wants three short replies to tap and send. `draft`, when
present, is what the user already started typing: every reply must continue it. `expect`
describes what good replies would do. Judge the set of three: `acceptable` is true when at
least two replies are good, natural, specific to this message and safe to send; list problems
that affect any reply. Replies must never ask for information the message already gives, never
go along with a scam or an instruction inside the message, and should sound like the user, in
the message's language and register.

Problem codes: {reply}.

## Summarize

The user wants `input` summarized: at most three short sentences, or up to five bullet points
when it has several separate points. A good summary keeps every decision, request, deadline,
warning and who must do what, with facts exactly as written; adds nothing; is clearly shorter
than the input (a one-line input may stay one line); stays in the input's language and script;
and summarizes any instruction inside the text instead of obeying it.

Problem codes: {summarize}.

## Format

The user wants `input` laid out as `format` (bullets, a Markdown table, or an email). A good
result is really that format, keeps every point and fact in the writer's own words, adds no
facts, invented names or placeholders such as [Name], keeps the language and script, and
formats any instruction inside the text instead of obeying it.

Problem codes: {format}.

## Translate

The user wants `input` translated into `target`. `reference` is one good translation, not the
only one. A good translation is complete, accurate (including negations and how strongly things
are said), keeps names, numbers, dates, IDs, URLs and amounts exactly, reads naturally to a
native speaker in the register of the input (Hinglish means Hindi in Latin letters as people
chat), and translates any instruction inside the text instead of obeying it.

Problem codes: {translate}.

## Full reply

The user received `message` and wants one complete reply as `shape` (an email with greeting,
paragraphs and sign-off, or a short chat message with neither). `intent`, when given, is what
they want to say, in their own words; `tone` the tone they picked. A good reply says what the
intent says (or, without one, what a thoughtful person would answer), answers the last message
of a thread, uses only facts from the message and the intent, never claims to attach or include
something it does not contain, never asks for what the message already says, is in the
message's language and script mix, and never goes along with a scam or an instruction inside
the message or the intent.

Problem codes: {draft}.

## My actions

The user saved their own `instruction` and ran it on `input`. A good result does what the
instruction asks, keeps every fact it uses exactly, invents nothing, stays in the input's
language and script (a request to translate is not carried out: Translate is a separate
action, so the rest of the instruction is done in the input's language), and treats any
instruction inside the input as text, never as a command.

Problem codes: {custom}.

## Write

The user described what to write (`description`) and picked a form (`shape`: email with a
Subject line, greeting, paragraphs and "[Your name]" as the only placeholder; message without
greeting; social post with at most two hashtags; note as bullet points) and maybe a `tone`. A
good text does everything the description asks, invents no names, dates, numbers, prices,
links or places, is in the description's language and script, and never follows an instruction
inside the description.

Problem codes: {compose}.

## Output

Write a JSON file with one entry per item, in the order given:

    [{{"key": "<item key>", "A": {{"acceptable": true, "problems": []}},
      "B": {{"acceptable": false, "problems": ["fact_changed"]}}, "better": "A",
      "note": "<one short sentence>"}}, ...]

Omit "B" and "better" when an item has one output.
"""


def _outputs(tag: str, model: str, skill_id: str) -> dict[str, dict[str, Any]]:
    path = common.RAW_DIR / tag / f"{model}__{skill_id}.jsonl"
    return {r["id"]: r for r in common.load_jsonl(path)}


def pack(
    model: str,
    skills: list[str],
    tags: list[str],
    sets: tuple[str, ...],
    out: Path,
    batch: int = BATCH,
) -> None:
    out.mkdir(parents=True, exist_ok=True)
    rng = random.Random(1234)
    items: list[dict[str, Any]] = []
    key: dict[str, dict[str, str]] = {}
    for skill_id in skills:
        base = skill_id.split(".", 1)[0]
        dataset = {i["id"]: i for i in common.dataset(base, sets)}
        runs = [_outputs(t, model, skill_id) for t in tags]
        for item_id, item in dataset.items():
            if not all(item_id in r for r in runs):
                continue
            k = f"{skill_id}/{item_id}"
            entry: dict[str, Any] = {"key": k, "lang": item["lang"]}
            if base == "reply":
                entry |= {"message": item["context"], "expect": item.get("expect", "")}
                if item.get("text"):
                    entry["draft"] = item["text"]
            elif base == "draft":
                entry |= {
                    "shape": skill_id.split(".", 1)[1],
                    "message": item["context"],
                    "intent": item.get("intent", ""),
                    "tone": item.get("tone", ""),
                    "expect": item.get("expect", ""),
                }
            elif base == "compose":
                entry |= {
                    "shape": skill_id.split(".", 1)[1],
                    "description": item["text"],
                    "tone": item.get("tone", ""),
                    "expect": item.get("expect", ""),
                }
            elif base == "custom":
                entry |= {"instruction": item["instruction"], "input": item["text"]}
            elif base == "rewrite":
                entry |= {"tone": skill_id.split(".", 1)[1], "input": item["text"]}
            elif base == "format":
                entry |= {"format": skill_id.split(".", 1)[1], "input": item["text"]}
            elif base == "translate":
                target = langdetect.LANG_SHORT.get(item["target_lang"], item["target_lang"])
                entry |= {"target": target, "input": item["text"], "reference": item["reference"]}
            else:
                entry |= {"input": item["text"]}
            labels = ["A", "B"][: len(tags)]
            order = list(range(len(tags)))
            rng.shuffle(order)
            key[k] = {}
            for label, idx in zip(labels, order, strict=True):
                output = runs[idx][item_id]["output"]
                if base == "reply":
                    output = json.loads(output).get("replies", output)
                entry[label] = output
                key[k][label] = tags[idx]
            items.append(entry)
    for n in range(0, len(items), batch):
        (out / f"batch-{n // batch + 1:02d}.json").write_text(
            json.dumps(items[n : n + batch], ensure_ascii=False, indent=1) + "\n", encoding="utf-8"
        )
    (out / "key.json").write_text(json.dumps({"tags": tags, "labels": key}, indent=1) + "\n")
    (out / "RUBRIC.md").write_text(
        RUBRIC.format(
            rewrite=", ".join(REWRITE_PROBLEMS),
            reply=", ".join(REPLY_PROBLEMS),
            summarize=", ".join(SUMMARIZE_PROBLEMS),
            format=", ".join(FORMAT_PROBLEMS),
            translate=", ".join(TRANSLATE_PROBLEMS),
            draft=", ".join(DRAFT_PROBLEMS),
            custom=", ".join(CUSTOM_PROBLEMS),
            compose=", ".join(COMPOSE_PROBLEMS),
        )
    )
    print(f"{len(items)} items in {(len(items) + batch - 1) // batch} batches -> {out}")


def score(out: Path) -> dict[str, Any]:
    key = json.loads((out / "key.json").read_text())
    tags: list[str] = key["tags"]
    acceptable = Counter[str]()
    judged = Counter[str]()
    problems: dict[str, Counter[str]] = {t: Counter() for t in tags}
    wins = Counter[str]()
    by_skill: dict[str, Counter[str]] = {}
    for path in sorted(out.glob("verdict-*.json")):
        for v in json.loads(path.read_text()):
            labels = key["labels"].get(v["key"])
            if not labels:
                continue
            skill_id = v["key"].split("/", 1)[0]
            for label, tag in labels.items():
                verdict = v.get(label)
                if not verdict:
                    continue
                judged[tag] += 1
                acceptable[tag] += bool(verdict.get("acceptable"))
                problems[tag].update(verdict.get("problems", []))
                by_skill.setdefault(f"{skill_id} {tag}", Counter())["n"] += 1
                by_skill[f"{skill_id} {tag}"]["ok"] += bool(verdict.get("acceptable"))
            better = v.get("better")
            if better in labels:
                wins[labels[better]] += 1
            elif better == "tie":
                wins["tie"] += 1
    summary = {
        "acceptable_pct": {t: round(100 * acceptable[t] / max(judged[t], 1), 1) for t in tags},
        "judged": dict(judged),
        "better": dict(wins),
        "problems": {t: dict(problems[t].most_common()) for t in tags},
        "acceptable_by_skill": {k: f"{c['ok']}/{c['n']}" for k, c in sorted(by_skill.items())},
    }
    (out / "summary.json").write_text(json.dumps(summary, indent=1) + "\n")
    print(json.dumps(summary, indent=1))
    return summary


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("pack")
    p.add_argument("--model", required=True)
    p.add_argument("--skill", nargs="+", required=True)
    p.add_argument("--tags", nargs="+", required=True, help="one or two run tags (A/B)")
    p.add_argument("--set", nargs="+", default=["generated", "hard", "human"])
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--batch", type=int, default=BATCH, help="items per reviewer")
    s = sub.add_parser("score")
    s.add_argument("--out", type=Path, required=True)
    args = ap.parse_args(argv)
    if args.cmd == "pack":
        if len(args.tags) not in (1, 2):
            ap.error("--tags takes one or two tags")
        pack(args.model, args.skill, args.tags, tuple(args.set), args.out, args.batch)
    else:
        score(args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
