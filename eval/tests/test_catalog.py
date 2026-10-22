import importlib.util
import json
import sys
from pathlib import Path
from types import ModuleType

from paraeval import common


def _fetch_module() -> ModuleType:
    path = common.ROOT / "scripts" / "fetch_model.py"
    spec = importlib.util.spec_from_file_location("fetch_model", path)
    assert spec and spec.loader
    mod = importlib.util.module_from_spec(spec)
    sys.modules["fetch_model"] = mod
    spec.loader.exec_module(mod)
    return mod


def test_catalog_entries_are_pinned() -> None:
    data = json.loads(common.CATALOG.read_text())
    ids = [m["id"] for m in data["models"]]
    assert len(ids) == len(set(ids))
    for m in data["models"]:
        assert len(m["revision"]) == 40, m["id"]
        assert m["licence"] and m["licence_url"].startswith("https://")
        for f in m["files"]:
            assert len(f["sha256"]) == 64 and f["size"] > 0
    assert {m["role"] for m in data["models"]} <= {"candidate", "judge", "experiment", "draft"}
    ids_set = set(ids)
    for m in data["models"]:
        if m["role"] == "draft":
            assert m["draft_for"] and set(m["draft_for"]) <= ids_set, m["id"]


def test_verified_stamp_tracks_changes(tmp_path: Path) -> None:
    fetch = _fetch_module()
    data = b"hello"
    f = tmp_path / "m.gguf"
    f.write_bytes(data)
    import hashlib

    entry = fetch.ModelFile("m.gguf", len(data), hashlib.sha256(data).hexdigest())
    assert not fetch.is_verified(f, entry)
    fetch._write_stamp(f, entry)
    assert fetch.is_verified(f, entry)
    f.write_bytes(b"jello")
    assert not fetch.is_verified(f, entry)
