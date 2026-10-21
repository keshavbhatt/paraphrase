#!/usr/bin/env python3
"""Download a model from models.json by id and verify every file's sha256.

Files come from Hugging Face at the pinned revision (PLAN D8) and land in
models/<id>/. Partial downloads resume. A file is only renamed into place after
its size and sha256 match the catalog; a mismatch deletes it and fails.

Usage:
    scripts/fetch_model.py <id> [<id> ...]
    scripts/fetch_model.py --list
    scripts/fetch_model.py --verify <id>     re-hash files already on disk
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
import time
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent.parent
CATALOG = ROOT / "models.json"
MODELS_DIR = ROOT / "models"
CHUNK = 4 * 1024 * 1024
USER_AGENT = "paraphrase-fetch/0 (+https://github.com/keshavbhatt/paraphrase)"


@dataclass(frozen=True)
class ModelFile:
    file: str
    size: int
    sha256: str


@dataclass(frozen=True)
class Model:
    id: str
    hf_repo: str
    revision: str
    files: tuple[ModelFile, ...]

    def url(self, f: ModelFile) -> str:
        return f"https://huggingface.co/{self.hf_repo}/resolve/{self.revision}/{f.file}"


def load_catalog(path: Path = CATALOG) -> dict[str, Model]:
    data: dict[str, Any] = json.loads(path.read_text(encoding="utf-8"))
    out: dict[str, Model] = {}
    for m in data["models"]:
        files = tuple(ModelFile(f["file"], int(f["size"]), f["sha256"].lower()) for f in m["files"])
        out[m["id"]] = Model(m["id"], m["hf_repo"], m["revision"], files)
    return out


def model_path(model_id: str, models_dir: Path = MODELS_DIR) -> Path:
    """Path llama.cpp should load: the first file (split GGUFs find the rest)."""
    model = load_catalog()[model_id]
    return models_dir / model_id / model.files[0].file


def sha256_of(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        while chunk := fh.read(CHUNK):
            h.update(chunk)
    return h.hexdigest()


def _stamp(path: Path) -> Path:
    return path.with_name(path.name + ".sha256")


def is_verified(path: Path, f: ModelFile) -> bool:
    """True when the file was verified before and has not changed since (size and mtime)."""
    stamp = _stamp(path)
    if not path.is_file() or not stamp.is_file():
        return False
    st = path.stat()
    return stamp.read_text().strip() == f"{f.sha256} {st.st_size} {st.st_mtime_ns}"


def _write_stamp(path: Path, f: ModelFile) -> None:
    st = path.stat()
    _stamp(path).write_text(f"{f.sha256} {st.st_size} {st.st_mtime_ns}\n")


def _download(url: str, part: Path, size: int) -> None:
    have = part.stat().st_size if part.exists() else 0
    if have > size:
        part.unlink()
        have = 0
    if have == size:
        return
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    if have:
        req.add_header("Range", f"bytes={have}-")
    with urllib.request.urlopen(req, timeout=60) as resp:
        if have and resp.status != 206:
            have = 0  # server ignored the range: start over
        mode = "ab" if have else "wb"
        done = have
        last = 0.0
        start = time.monotonic()
        with part.open(mode) as out:
            while chunk := resp.read(CHUNK):
                out.write(chunk)
                done += len(chunk)
                now = time.monotonic()
                if now - last > 2 or done == size:
                    last = now
                    rate = (done - have) / max(now - start, 1e-6) / 1e6
                    pct = 100 * done / size
                    print(
                        f"\r  {pct:5.1f}%  {done / 1e9:.2f}/{size / 1e9:.2f} GB  {rate:.1f} MB/s",
                        end="",
                        flush=True,
                    )
    print()


def fetch(model: Model, models_dir: Path = MODELS_DIR, retries: int = 3) -> None:
    dest_dir = models_dir / model.id
    dest_dir.mkdir(parents=True, exist_ok=True)
    for f in model.files:
        dest = dest_dir / f.file
        if is_verified(dest, f):
            print(f"{model.id}: {f.file} already verified")
            continue
        url = model.url(f)
        print(f"{model.id}: {f.file} from {url}")
        part = dest.with_name(dest.name + ".part")
        for attempt in range(1, retries + 1):
            try:
                _download(url, part, f.size)
                break
            except (urllib.error.URLError, TimeoutError, ConnectionError) as exc:
                print(f"\n  attempt {attempt} failed: {exc}", file=sys.stderr)
                if attempt == retries:
                    raise
                time.sleep(5 * attempt)
        if part.stat().st_size != f.size:
            raise SystemExit(f"{f.file}: size {part.stat().st_size} != catalog {f.size}")
        digest = sha256_of(part)
        if digest != f.sha256:
            part.unlink()
            raise SystemExit(f"{f.file}: sha256 mismatch ({digest}), file deleted")
        part.replace(dest)
        _write_stamp(dest, f)
        print(f"  sha256 ok {f.sha256}")


def verify(model: Model, models_dir: Path = MODELS_DIR) -> bool:
    ok = True
    for f in model.files:
        dest = models_dir / model.id / f.file
        if not dest.is_file():
            print(f"{f.file}: missing")
            ok = False
            continue
        good = sha256_of(dest) == f.sha256
        print(f"{f.file}: {'ok' if good else 'MISMATCH'}")
        if good:
            _write_stamp(dest, f)
        ok = ok and good
    return ok


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("ids", nargs="*", help="model ids from models.json")
    ap.add_argument("--list", action="store_true", help="list catalog ids and download sizes")
    ap.add_argument("--verify", action="store_true", help="re-hash files on disk, do not download")
    args = ap.parse_args(argv)
    catalog = load_catalog()
    if args.list:
        for m in catalog.values():
            gb = sum(f.size for f in m.files) / 1e9
            print(f"{m.id:28} {gb:6.2f} GB  {m.hf_repo}")
        return 0
    if not args.ids:
        ap.error("give at least one model id (see --list)")
    unknown = [i for i in args.ids if i not in catalog]
    if unknown:
        ap.error(f"unknown model id(s): {', '.join(unknown)}")
    ok = True
    for model_id in args.ids:
        if args.verify:
            ok = verify(catalog[model_id]) and ok
        else:
            fetch(catalog[model_id])
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
