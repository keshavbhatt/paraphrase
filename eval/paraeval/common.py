"""Paths, the model catalog, run profiles and datasets shared by the eval scripts."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from paraeval import langdetect
from paraeval.llama import Profile

ROOT = Path(__file__).resolve().parents[2]
SKILLS_DIR = ROOT / "skills"
DATASETS_DIR = ROOT / "eval" / "datasets"
REPORTS_DIR = ROOT / "eval" / "reports"
RAW_DIR = REPORTS_DIR / "raw"
MODELS_DIR = ROOT / "models"
CATALOG = ROOT / "models.json"
MACHINES = ROOT / "eval" / "machines.json"


@dataclass(frozen=True)
class ModelInfo:
    id: str
    family: str
    role: str
    ctx: int
    path: Path


def catalog() -> dict[str, ModelInfo]:
    data = json.loads(CATALOG.read_text(encoding="utf-8"))
    out = {}
    for m in data["models"]:
        path = MODELS_DIR / m["id"] / m["files"][0]["file"]
        out[m["id"]] = ModelInfo(m["id"], m["family"], m["role"], int(m["ctx"]), path)
    return out


def model(model_id: str) -> ModelInfo:
    models = catalog()
    if model_id not in models:
        raise SystemExit(f"unknown model {model_id}; see models.json")
    info = models[model_id]
    if not info.path.is_file():
        raise SystemExit(f"{info.path} missing: run `make fetch MODEL={model_id}`")
    return info


def profiles() -> dict[str, Profile]:
    data = json.loads(MACHINES.read_text(encoding="utf-8"))
    return {k: Profile.from_dict(v) for k, v in data["profiles"].items()}


def load_jsonl(path: Path) -> list[dict[str, Any]]:
    if not path.is_file():
        return []
    lines = path.read_text(encoding="utf-8").splitlines()
    return [json.loads(line) for line in lines if line.strip()]


def write_jsonl(path: Path, rows: list[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as fh:
        for row in rows:
            fh.write(json.dumps(row, ensure_ascii=False) + "\n")


def dataset(
    base_skill: str, sets: tuple[str, ...] = ("generated", "hard", "human")
) -> list[dict[str, Any]]:
    """All items for a skill: <lang>.<set>.jsonl files under eval/datasets/<skill>/."""
    items: list[dict[str, Any]] = []
    for path in sorted((DATASETS_DIR / base_skill).glob("*.jsonl")):
        parts = path.name.split(".")
        if len(parts) == 3 and parts[1] in sets:
            for item in load_jsonl(path):
                item.setdefault("source", parts[1])
                items.append(item)
    return items


def skill_inputs(base_skill: str, item: dict[str, Any]) -> dict[str, str]:
    """Map a dataset item to the skill's input keys (PLAN 5.1)."""
    if base_skill == "reply":
        inputs = {"context": item["context"], "text": item.get("text", "")}
        return inputs | langdetect.inputs_for(item["context"])
    inputs = {"text": item["text"]} | langdetect.inputs_for(item["text"])
    if item.get("target_lang"):  # the user explicitly chose the output language
        inputs |= {
            "lang_name": langdetect.LANG_SHORT.get(inputs["lang"], "its own language"),
            "target_lang": item["target_lang"],
            "target_lang_name": langdetect.LANG_SHORT[item["target_lang"]],
        }
    return inputs
