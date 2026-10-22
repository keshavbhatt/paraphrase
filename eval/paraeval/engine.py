"""What the engine does around one model call, so the eval measures what the daemon will run.

The M1 daemon implements the same steps in C++ (PLAN D2); keep the two in step:

1. Render the skill's prompts (skills.Skill.render).
2. Decode `variants` completions from one prompt in one batch (llama-server `n_cmpl`).
3. Language check (skill schema v2, `output.language: same`): the output must be in the
   expected language and script, which is the input's language unless the request names a
   `target_lang` the user chose. On a mismatch, retry once with a fresh seed (greedy skills
   retry with a little temperature, or the retry would repeat the same output); if the retry
   also fails, return it with a warning instead of hiding it (owner, 2026-10-22). The retry
   adds one line after the prompt naming the language the answer must be in: a new seed
   alone kept giving English for Hinglish professional rewrites (M0 v5).
4. Speculative decoding per skill (schema v3, ADR-004): `speculative: none` asks the server
   not to draft for this request, even when it runs with a draft head.
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from typing import Any

from paraeval import langdetect, metrics
from paraeval.llama import Completion, LlamaServer
from paraeval.skills import Skill

RETRY_TEMPERATURE = 0.3  # for skills that decode greedily
RETRY_SEED_OFFSET = 7919


@dataclass
class Result:
    output: str  # text, or {"replies": [...]} JSON for a skill with several variants
    completion: Completion  # the call whose output is returned
    calls: list[Completion] = field(default_factory=list)
    retried: bool = False
    warning: str = ""  # set when the language check still fails after the retry

    @property
    def total_s(self) -> float:
        return sum(c.total_s for c in self.calls)


def expected_lang(inputs: dict[str, str]) -> str:
    """The language the output must be in: the user's explicit choice, else the input's."""
    return inputs.get("target_lang") or inputs["lang"]


def outputs_of(skill: Skill, c: Completion) -> tuple[str, list[str]]:
    """(output as stored and scored, the separate texts the language check looks at)."""
    if skill.variants > 1:
        texts = [t.strip() for t in c.texts]
        return json.dumps({"replies": texts}, ensure_ascii=False), texts
    if skill.output.get("type") == "json":
        parsed = metrics.parse_replies(c.text)
        return c.text, parsed if parsed is not None else [c.text]
    return c.text, [c.text]


def language_ok(skill: Skill, inputs: dict[str, str], texts: list[str]) -> tuple[bool, str]:
    if skill.output.get("language", "any") != "same":
        return True, ""
    want = expected_lang(inputs)
    source = inputs.get("context") or inputs.get("text", "")
    for t in texts:
        if want != inputs["lang"]:
            # An explicit target language: the output itself must read as that language.
            got = langdetect.detect(t)
            if got != want:
                return False, f"asked for {want}, got {got}"
            continue
        ok, why = metrics.language_check(want, source, t)
        if not ok:
            return False, why
    return True, ""


def run(server: LlamaServer, skill: Skill, inputs: dict[str, str]) -> Result:
    system, user = skill.render(inputs)
    sampling = dict(skill.sampling)
    extra = {"speculative.type": "none"} if skill.speculative == "none" else {}
    calls: list[Completion] = []
    for attempt in range(2):
        prompt = user if attempt == 0 else f"{user}\n\n{retry_hint(inputs)}"
        c = server.chat(
            system,
            prompt,
            sampling,
            skill.max_output_tokens,
            skill.grammar,
            n=skill.variants,
            extra=extra,
        )
        calls.append(c)
        output, texts = outputs_of(skill, c)
        ok, why = language_ok(skill, inputs, texts)
        if ok:
            return Result(output, c, calls, retried=attempt > 0)
        sampling = _retry_sampling(skill.sampling)
    return Result(output, c, calls, retried=True, warning=f"language: {why}")


def retry_hint(inputs: dict[str, str]) -> str:
    """The line the retry adds: which language the answer must be in, by its plain name."""
    want = expected_lang(inputs)
    name = langdetect.LANG_SHORT.get(want)
    if want != inputs["lang"]:
        return f"Important: write your answer in {name}, as the user asked."
    if not name:
        return "Important: write your answer in the same language and script as the original."
    return f"Important: write your answer in {name}, the language of the original."


def _retry_sampling(sampling: dict[str, Any]) -> dict[str, Any]:
    s = dict(sampling)
    if not s.get("temperature"):
        s["temperature"] = RETRY_TEMPERATURE
    s["seed"] = (s.get("seed") or 0) + RETRY_SEED_OFFSET
    return s
