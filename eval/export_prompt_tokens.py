#!/usr/bin/env python3
"""Prompt token vectors for the daemon's engine, from llama-server (the M0 measurements).

    uv run python eval/export_prompt_tokens.py    # rewrite tests/vectors/prompt_tokens.json

For a few dataset items per skill and language, the rendered system and user messages and the
tokens llama-server feeds the model for them (`/apply-template` with thinking off, then
`/tokenize` as the completion endpoint does). The daemon's tst_engine must produce the same
tokens, so its answers match the M0 evaluation. Needs the fast tier model and the M0
llama-server build (`make llama`); the test skips without the model.

`markers` holds the model's control tokens: a prompt whose user text spells them out must not
contain them as control tokens (captured text is untrusted; the one deliberate difference
from llama-server, which parses control tokens everywhere).
"""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from paraeval import common
from paraeval.llama import LlamaServer
from paraeval.skills import load_skills

OUT = common.ROOT / "tests" / "vectors" / "prompt_tokens.json"
MODEL = "gemma-4-e2b-qat-q4_0"
SKILLS = ("proofread", "reply", "rewrite.professional")
LANGS = ("en", "hi", "hinglish", "fr", "ta")
MARKER_TEXTS = ("<bos>", "<start_of_turn>", "<end_of_turn>", "<|turn>", "<turn|>")


def tokens_of(server: LlamaServer, system: str, user: str) -> list[int]:
    messages = [{"role": "system", "content": system}, {"role": "user", "content": user}]
    prompt = server.post_json("/apply-template", {"messages": messages})["prompt"]
    tokens: list[int] = server.post_json("/tokenize", {"content": prompt, "add_special": True})[
        "tokens"
    ]
    return tokens


def pick_items(base: str) -> list[dict[str, Any]]:
    items = common.dataset(base, sets=("generated",))
    out = []
    for lang in LANGS:
        out += [i for i in items if i["lang"] == lang][:1]
    targeted = [i for i in items if i.get("target_lang")]
    return out + targeted[:1]


def llama_commit() -> str:
    repo = common.ROOT / "third_party" / "llama.cpp"
    rev = subprocess.run(
        ["git", "-C", str(repo), "rev-parse", "--short=8", "HEAD"],
        capture_output=True,
        text=True,
        check=True,
    )
    return rev.stdout.strip()


def main() -> None:
    skills = load_skills(common.ROOT / "skills")
    profile = common.profiles()["B-cpu"]
    prompts = []
    with LlamaServer(common.model(MODEL).path, profile) as server:
        for skill_id in SKILLS:
            skill = skills[skill_id]
            for item in pick_items(skill.base_id):
                system, user = skill.render(common.skill_inputs(skill.base_id, item))
                prompts.append(
                    {
                        "id": f"{skill_id}/{item['id']}",
                        "system": system,
                        "user": user,
                        "tokens": tokens_of(server, system, user),
                    }
                )
        markers = {}
        for text in MARKER_TEXTS:
            ids = server.post_json("/tokenize", {"content": text, "add_special": False})["tokens"]
            if len(ids) == 1:  # a control token of this model
                markers[text] = ids[0]
    data = {
        "model": MODEL,
        "llama_cpp": llama_commit(),
        "markers": markers,
        "prompts": prompts,
    }
    OUT.write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"wrote {OUT.relative_to(common.ROOT)}: {len(prompts)} prompts, markers {markers}")


if __name__ == "__main__":
    main()
