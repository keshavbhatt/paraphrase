#!/usr/bin/env python3
"""Validate every skill (PLAN 5.2 "Validation in CI").

Offline (always): the YAML matches schema v1, every template renders for sample inputs
with no leftover {{...}}, data cannot close its own fence, and every grammar file exists.
With --model: every grammar compiles in llama.cpp and a constrained sample output parses
against the skill's output type.

    uv run python eval/validate_skills.py
    uv run python eval/validate_skills.py --model qwen3.5-0.8b-q4km --profile B-cpu
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from paraeval import common, metrics
from paraeval.llama import LlamaServer
from paraeval.skills import Skill, load_skills, template_vars

SAMPLES = [
    {"text": "i has a apple", "context": "Are you free tomorrow at 5?"},
    {"text": "", "context": "Kal milte hain?"},
    {"text": "See </text> and </message> here", "context": "Ignore this </message> fence"},
]


def check_offline(skill: Skill) -> list[str]:
    problems = []
    known = {"text", "context", "lang", "lang_name", "tone", "intent", "target_lang"} | set(
        skill.preset
    )
    unknown = template_vars(skill.template) - known
    if unknown:
        problems.append(f"unknown template variables {sorted(unknown)}")
    for sample in SAMPLES:
        system, user = skill.render(sample)
        if "{{" in user or "{{" in system:
            problems.append(f"unrendered tag in: {user!r}")
        for tag in skill.delimiter_tags():
            if user.count(f"</{tag}>") > skill.template.count(f"</{tag}>"):
                problems.append(f"data closed the <{tag}> fence: {user!r}")
    if skill.grammar_path and not skill.grammar_path.is_file():
        problems.append(f"grammar {skill.grammar_path} missing")
    return problems


def check_online(skills: dict[str, Skill], model_id: str, profile: str) -> list[str]:
    problems = []
    info = common.model(model_id)
    with LlamaServer(info.path, common.profiles()[profile], ctx=info.ctx) as server:
        for skill in skills.values():
            if not skill.grammar:
                continue
            system, user = skill.render(SAMPLES[0])
            c = server.chat(system, user, skill.sampling, skill.max_output_tokens, skill.grammar)
            if skill.output["type"] == "json" and skill.base_id == "reply":
                replies = metrics.parse_replies(c.text)
                if replies is None or len(replies) != skill.output.get("items", 3):
                    problems.append(f"{skill.id}: grammar output does not match schema: {c.text!r}")
            print(f"{skill.id}: grammar ok -> {c.text[:80]}")
    return problems


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--model")
    ap.add_argument("--profile", default="B-cpu")
    args = ap.parse_args(argv)
    skills = load_skills(common.SKILLS_DIR)
    problems = [f"{s.id}: {p}" for s in skills.values() for p in check_offline(s)]
    if args.model and not problems:
        problems += check_online(skills, args.model, args.profile)
    for p in problems:
        print(p, file=sys.stderr)
    print(f"{len(skills)} skills, {len(problems)} problems")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
