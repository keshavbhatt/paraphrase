"""Run llama.cpp's llama-server on a Unix socket and talk to it (M0 only).

The daemon links llama.cpp as a library from M1 (PLAN D2). For M0 the harness drives the
server binary instead, bound to a Unix socket in a private temporary directory: no TCP
port is ever opened (PLAN D7, CLAUDE.md).
"""

from __future__ import annotations

import http.client
import json
import os
import shutil
import socket
import subprocess
import tempfile
import threading
import time
from collections.abc import Iterator
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]


class UnixHTTPConnection(http.client.HTTPConnection):
    def __init__(self, path: str, timeout: float = 600) -> None:
        super().__init__("localhost", timeout=timeout)
        self._path = path

    def connect(self) -> None:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.settimeout(self.timeout)
        sock.connect(self._path)
        self.sock = sock


@dataclass
class Profile:
    """How to run the server: which build, threads, GPU offload, CPU pinning, memory cap."""

    name: str
    build: str = "cpu"  # cpu | vulkan
    threads: int | None = None
    gpu_layers: int = 0
    cpus: str | None = None  # taskset list, e.g. "4-7"
    memory_max: str | None = None  # systemd MemoryMax, e.g. "8G"
    label: str = ""
    extra_args: list[str] = field(default_factory=list)

    @staticmethod
    def from_dict(d: dict[str, Any]) -> Profile:
        return Profile(**d)


@dataclass
class Completion:
    text: str  # the first completion; all of them are in `texts`
    ttft_s: float
    total_s: float
    prompt_tokens: int
    gen_tokens: int
    prompt_ms: float
    gen_ms: float
    cached_tokens: int
    finish_reason: str
    texts: list[str] = field(default_factory=list)
    draft_tokens: int = 0  # speculative decoding: tokens drafted
    draft_accepted: int = 0  # ... and accepted by the main model

    @property
    def gen_tps(self) -> float:
        return self.gen_tokens / (self.gen_ms / 1000) if self.gen_ms > 0 else 0.0

    @property
    def prompt_tps(self) -> float:
        return self.prompt_tokens / (self.prompt_ms / 1000) if self.prompt_ms > 0 else 0.0


def limits_prefix(p: Profile) -> list[str]:
    """The memory cap and CPU pinning of a profile (machine A is emulated with both); both
    tools exec into the command they run, so its pid is the one started."""
    cmd: list[str] = []
    if p.memory_max:
        cmd += [
            "systemd-run",
            "--user",
            "--scope",
            "--quiet",
            "-p",
            f"MemoryMax={p.memory_max}",
            "-p",
            "MemorySwapMax=0",
        ]
    if p.cpus:
        cmd += ["taskset", "-c", p.cpus]
    return cmd


def server_binary(build: str) -> Path:
    path = ROOT / "build" / f"llama-{build}" / "bin" / "llama-server"
    if not path.is_file():
        raise FileNotFoundError(f"{path} missing: run `make llama`")
    return path


def evict_page_cache(model_file: Path) -> None:
    """Drop a model's pages from the page cache so the next load reads the disk."""
    for f in sorted(model_file.parent.glob("*.gguf")):
        fd = os.open(f, os.O_RDONLY)
        try:
            os.fsync(fd)
            os.posix_fadvise(fd, 0, 0, os.POSIX_FADV_DONTNEED)
        finally:
            os.close(fd)


class LlamaServer:
    """Context manager: start llama-server for one model, wait until it serves, stop it."""

    def __init__(
        self,
        model_file: Path,
        profile: Profile,
        ctx: int = 4096,
        parallel: int = 1,
        reasoning_off: bool = True,
    ) -> None:
        self.model_file = model_file
        self.profile = profile
        self.ctx = ctx
        self.parallel = parallel
        self.reasoning_off = reasoning_off
        self._tmp = tempfile.mkdtemp(prefix="paraphrase-llama-")
        self.sock = str(Path(self._tmp) / "llama.sock")
        self.proc: subprocess.Popen[bytes] | None = None
        self.load_s = 0.0
        self._peak_kb = 0
        self._stop = threading.Event()
        self._watch: threading.Thread | None = None

    def command(self) -> list[str]:
        p = self.profile
        cmd = limits_prefix(p)
        cmd += [
            str(server_binary(p.build)),
            "-m",
            str(self.model_file),
            "--host",
            self.sock,
            "-c",
            str(self.ctx * self.parallel),
            "-np",
            str(self.parallel),
            "-ngl",
            str(p.gpu_layers),
            "--no-webui",
            "--log-disable",
            "--metrics",
        ]
        if p.threads:
            cmd += ["-t", str(p.threads), "-tb", str(p.threads)]
        if self.reasoning_off:
            cmd += ["--reasoning", "off"]
        return cmd + p.extra_args

    def __enter__(self) -> LlamaServer:
        start = time.monotonic()
        self.proc = subprocess.Popen(
            self.command(), stdout=subprocess.DEVNULL, stderr=subprocess.PIPE
        )
        self._watch = threading.Thread(target=self._watch_rss, daemon=True)
        self._watch.start()
        while True:
            if self.proc.poll() is not None:
                err = self.proc.stderr.read().decode(errors="replace") if self.proc.stderr else ""
                raise RuntimeError(f"llama-server exited ({self.proc.returncode}):\n{err[-3000:]}")
            if self._healthy():
                break
            if time.monotonic() - start > 600:
                raise TimeoutError("llama-server did not become ready in 600 s")
            time.sleep(0.02)
        self.load_s = time.monotonic() - start
        if self.proc.stderr:  # keep the pipe drained so the server never blocks on it
            threading.Thread(target=self.proc.stderr.read, daemon=True).start()
        return self

    def __exit__(self, *exc: object) -> None:
        self._stop.set()
        if self.proc and self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=20)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        shutil.rmtree(self._tmp, ignore_errors=True)

    def _server_pid(self) -> int | None:
        """The llama-server pid (below systemd-run and taskset, which exec into it)."""
        return self.proc.pid if self.proc else None

    def _watch_rss(self) -> None:
        while not self._stop.is_set():
            pid = self._server_pid()
            try:
                status = Path(f"/proc/{pid}/status").read_text()
                for line in status.splitlines():
                    if line.startswith("VmHWM:"):
                        self._peak_kb = max(self._peak_kb, int(line.split()[1]))
            except (OSError, ValueError):
                pass
            self._stop.wait(0.2)

    @property
    def peak_rss_mb(self) -> float:
        return self._peak_kb / 1024

    def _healthy(self) -> bool:
        if not Path(self.sock).exists():
            return False
        try:
            conn = UnixHTTPConnection(self.sock, timeout=2)
            conn.request("GET", "/health")
            ok = conn.getresponse().status == 200
            conn.close()
            return ok
        except OSError:
            return False

    def _post(self, path: str, body: dict[str, Any]) -> http.client.HTTPResponse:
        conn = UnixHTTPConnection(self.sock)
        conn.request("POST", path, json.dumps(body), {"Content-Type": "application/json"})
        resp = conn.getresponse()
        if resp.status != 200:
            raise RuntimeError(f"{path}: HTTP {resp.status}: {resp.read()[:2000]!r}")
        return resp

    def post_json(self, path: str, body: dict[str, Any]) -> dict[str, Any]:
        """Any other endpoint (`/apply-template`, `/tokenize`): its JSON answer."""
        data: dict[str, Any] = json.loads(self._post(path, body).read())
        return data

    def chat(
        self,
        system: str,
        user: str,
        sampling: dict[str, Any],
        max_tokens: int,
        grammar: str | None = None,
        n: int = 1,
        extra: dict[str, Any] | None = None,
    ) -> Completion:
        """One chat request. With n > 1 the prompt is processed once and n completions are
        decoded in one batch (llama-server `n_cmpl`; the server needs n slots)."""
        if n > self.parallel:
            raise ValueError(f"n={n} needs a server started with parallel >= {n}")
        body: dict[str, Any] = {
            "messages": [{"role": "system", "content": system}, {"role": "user", "content": user}],
            "max_tokens": max_tokens,
            "stream": True,
            "cache_prompt": True,
            "timings_per_token": False,
        }
        for k, v in sampling.items():
            if v is not None:
                body[k] = v
        if grammar:
            body["grammar"] = grammar
        if n > 1:
            body["n_cmpl"] = n
        body.update(extra or {})
        start = time.monotonic()
        resp = self._post("/v1/chat/completions", body)
        ttft = 0.0
        parts: dict[int, list[str]] = {}
        timings: dict[str, Any] = {}
        all_timings: list[dict[str, Any]] = []
        finish = ""
        for event in _sse(resp):
            if "timings" in event:
                timings = event["timings"]
                all_timings.append(timings)
            for choice in event.get("choices", []):
                delta = choice.get("delta", {}).get("content")
                if delta:
                    if not parts:
                        ttft = time.monotonic() - start
                    parts.setdefault(int(choice.get("index", 0)), []).append(delta)
                if choice.get("finish_reason"):
                    finish = choice["finish_reason"]
        total = time.monotonic() - start
        texts = ["".join(parts.get(i, [])) for i in range(max(n, len(parts)))]
        first = min(all_timings, key=lambda t: -t.get("prompt_n", 0)) if all_timings else {}
        timings = first or timings
        return Completion(
            text=texts[0] if texts else "",
            texts=texts,
            draft_tokens=int(timings.get("draft_n", 0)),
            draft_accepted=int(timings.get("draft_n_accepted", 0)),
            ttft_s=ttft or total,
            total_s=total,
            prompt_tokens=int(timings.get("prompt_n", 0)),
            gen_tokens=int(timings.get("predicted_n", 0)),
            prompt_ms=float(timings.get("prompt_ms", 0.0)),
            gen_ms=float(timings.get("predicted_ms", 0.0)),
            cached_tokens=int(timings.get("cache_n", 0)),
            finish_reason=finish,
        )


def _sse(resp: http.client.HTTPResponse) -> Iterator[dict[str, Any]]:
    for raw in resp:
        line = raw.decode("utf-8").strip()
        if not line.startswith("data:"):
            continue
        data = line[5:].strip()
        if data == "[DONE]":
            break
        yield json.loads(data)
