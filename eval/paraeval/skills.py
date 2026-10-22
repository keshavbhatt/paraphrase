"""Skill files (skills/*.yaml, schema v3): loading, validation and prompt rendering.

This mirrors what the daemon's SkillRegistry will do in M1, so the template rules here are
the reference for the C++ port (DOCS/skills.md): `{{name}}` inserts a value and
`{{#if name}}...{{/if}}` keeps its body only when the value is non-empty. No other syntax,
no file or network access.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml

SCHEMA_VERSION = 4
SPECULATIVE = ("none", "draft")
OUTPUT_LANGUAGE = ("same", "any")
TIERS = ("fast", "quality", "any")
OUTPUT_TYPES = ("text", "json")
REQUIRED = (
    "id",
    "version",
    "title",
    "tier",
    "max_input_tokens",
    "max_output_tokens",
    "sampling",
    "output",
    "system",
    "template",
)
SAMPLING_KEYS = {"temperature", "top_p", "top_k", "min_p", "repeat_penalty", "seed"}

_VAR = re.compile(r"\{\{\s*([a-z_][a-z0-9_]*)\s*\}\}")
_IF = re.compile(r"\{\{#if\s+([a-z_][a-z0-9_]*)\s*\}\}(.*?)\{\{/if\}\}", re.DOTALL)
_UNLESS = re.compile(r"\{\{#unless\s+([a-z_][a-z0-9_]*)\s*\}\}(.*?)\{\{/unless\}\}", re.DOTALL)
_TAG = re.compile(r"</?([a-z_]+)>")


class SkillError(ValueError):
    pass


@dataclass(frozen=True)
class Skill:
    id: str
    base_id: str
    version: int
    revision: int
    title: str
    tier: str
    max_input_tokens: int
    max_output_tokens: int
    variants: int
    speculative: str
    sampling: dict[str, Any]
    output: dict[str, Any]
    system: str
    template: str
    grammar: str | None = None
    grammar_path: Path | None = None
    preset: dict[str, str] = field(default_factory=dict)

    def delimiter_tags(self) -> set[str]:
        """Tag names the template uses to fence data, e.g. text, message, draft."""
        return set(_TAG.findall(self.template))

    def render(self, inputs: dict[str, str]) -> tuple[str, str]:
        """Return (system, user) messages for these inputs."""
        values = {**self.preset, **inputs}
        values = {k: neutralise(v, self.delimiter_tags()) for k, v in values.items()}
        return self.system.strip(), render_template(self.template, values).strip()


def neutralise(value: str, tags: set[str]) -> str:
    """Stop data from closing or opening our fences: `</text>` becomes `< /text>`."""
    for tag in tags:
        value = re.sub(rf"<(/?)({tag})>", r"< \1\2>", value, flags=re.IGNORECASE)
    return value


def render_template(template: str, values: dict[str, str]) -> str:
    def cond(m: re.Match[str]) -> str:
        return m.group(2) if values.get(m.group(1), "").strip() else ""

    def unless(m: re.Match[str]) -> str:
        return "" if values.get(m.group(1), "").strip() else m.group(2)

    prev = None
    out = template
    while prev != out:  # nested blocks resolve from the inside out
        prev = out
        out = _UNLESS.sub(unless, _IF.sub(cond, out))
    return _VAR.sub(lambda m: values.get(m.group(1), ""), out)


def template_vars(template: str) -> set[str]:
    blocks = {m.group(1) for m in _IF.finditer(template)}
    return set(_VAR.findall(template)) | blocks | {m.group(1) for m in _UNLESS.finditer(template)}


def _check(raw: dict[str, Any], path: Path) -> None:
    missing = [k for k in REQUIRED if k not in raw]
    if missing:
        raise SkillError(f"{path.name}: missing {', '.join(missing)}")
    if raw["version"] != SCHEMA_VERSION:
        raise SkillError(f"{path.name}: version {raw['version']} != schema {SCHEMA_VERSION}")
    if raw["tier"] not in TIERS:
        raise SkillError(f"{path.name}: tier must be one of {TIERS}")
    if raw["output"].get("type") not in OUTPUT_TYPES:
        raise SkillError(f"{path.name}: output.type must be one of {OUTPUT_TYPES}")
    extra = set(raw["sampling"]) - SAMPLING_KEYS
    if extra:
        raise SkillError(f"{path.name}: unknown sampling keys {sorted(extra)}")
    if raw.get("speculative", "none") not in SPECULATIVE:
        raise SkillError(f"{path.name}: speculative must be one of {SPECULATIVE}")
    if raw["output"].get("language", "any") not in OUTPUT_LANGUAGE:
        raise SkillError(f"{path.name}: output.language must be one of {OUTPUT_LANGUAGE}")
    if int(raw.get("variants", 1)) > 1 and raw["output"]["type"] != "text":
        raise SkillError(f"{path.name}: with variants > 1 each variant is one text output")
    if "min_length" in raw["output"]:
        m = raw["output"]["min_length"]
        if not isinstance(m, (int, float)) or not 0 < m < 1:
            raise SkillError(f"{path.name}: output.min_length must be a number between 0 and 1")
        if raw["output"]["type"] != "text" or int(raw.get("variants", 1)) > 1:
            raise SkillError(f"{path.name}: output.min_length needs one text output")
    if raw["output"]["type"] == "json" and "grammar" not in raw:
        raise SkillError(f"{path.name}: json output needs a grammar (PLAN D5)")
    if "{{" in raw["system"]:
        raise SkillError(f"{path.name}: system must be constant so its KV cache is shared")


def load_skill_file(path: Path) -> list[Skill]:
    raw: dict[str, Any] = yaml.safe_load(path.read_text(encoding="utf-8"))
    _check(raw, path)
    grammar_path = None
    grammar = None
    if raw.get("grammar"):
        grammar_path = (path.parent / raw["grammar"]).resolve()
        if path.parent.resolve() not in grammar_path.parents:
            raise SkillError(f"{path.name}: grammar must live under skills/")
        grammar = grammar_path.read_text(encoding="utf-8")
    common: dict[str, Any] = {
        "base_id": raw["id"],
        "version": raw["version"],
        "revision": int(raw.get("revision", 1)),
        "tier": raw["tier"],
        "max_input_tokens": int(raw["max_input_tokens"]),
        "max_output_tokens": int(raw["max_output_tokens"]),
        "variants": int(raw.get("variants", 1)),
        "speculative": raw.get("speculative", "none"),
        "sampling": dict(raw["sampling"]),
        "output": dict(raw["output"]),
        "system": raw["system"],
        "template": raw["template"],
        "grammar": grammar,
        "grammar_path": grammar_path,
    }
    presets: dict[str, dict[str, str]] = raw.get("presets") or {}
    if not presets:
        return [Skill(id=raw["id"], title=raw["title"], **common)]
    skills = []
    for name, preset in presets.items():
        values = {k: str(v) for k, v in preset.items() if k != "title"}
        title = f"{raw['title']}: {preset.get('title', name)}"
        skills.append(Skill(id=f"{raw['id']}.{name}", title=title, preset=values, **common))
    return skills


def load_skills(skills_dir: Path, overrides: list[Path] | None = None) -> dict[str, Skill]:
    """All skills in skills_dir; each file in overrides replaces the skills it defines
    (used to evaluate a candidate revision before it replaces the shipped file)."""
    out: dict[str, Skill] = {}
    for path in sorted(skills_dir.glob("*.yaml")):
        for skill in load_skill_file(path):
            if skill.id in out:
                raise SkillError(f"duplicate skill id {skill.id}")
            out[skill.id] = skill
    for path in overrides or []:
        for skill in load_skill_file(path):
            out[skill.id] = skill
    return out
