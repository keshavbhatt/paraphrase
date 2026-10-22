"""The language data shared with the daemon: daemon/data/language.json, the single source for
word lists, markers and language names (DOCS/CODING_STANDARDS.md section 12)."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

PATH = Path(__file__).resolve().parents[2] / "daemon" / "data" / "language.json"
DATA: dict[str, Any] = json.loads(PATH.read_text(encoding="utf-8"))


def ordered_sets(key: str) -> dict[str, set[str]]:
    """A [[code, words], ...] list as an insertion-ordered dict: the order breaks ties."""
    return {code: set(words) for code, words in DATA[key]}
