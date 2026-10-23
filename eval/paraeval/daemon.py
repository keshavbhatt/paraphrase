"""Run skills through the M1 daemon instead of llama-server (`--backend daemon`).

The harness starts `paraphrase batch --model <id>` from the development build, which runs its
own private daemon (ADR-010), and sends one JSON line per request. The daemon does what
paraeval.engine does for llama-server (prompts, the language check and retry, per-skill
drafting), so the eval and the bench measure the shipped code. Timings are the client's:
first token and total include D-Bus, as a user would see them.

Profiles apply as for llama-server: a memory cap and CPU pinning wrap the CLI, and the daemon
inherits them; the daemon picks its threads from the CPUs it may use (ADR-016) and runs on the
CPU only, so Vulkan profiles do not apply.
"""

from __future__ import annotations

import json
import os
import subprocess
import tempfile
import threading
from pathlib import Path
from typing import Any

from paraeval import common
from paraeval.engine import Result
from paraeval.llama import Completion, Profile, limits_prefix
from paraeval.skills import Skill

INPUT_KEYS = ("text", "context", "target_lang", "intent", "tone", "instruction")
WARM_UP = {"skill": "proofread", "input": {"text": "Hello there."}}


def _children(pid: int) -> list[int]:
    try:
        text = Path(f"/proc/{pid}/task/{pid}/children").read_text()
    except OSError:
        return []
    return [int(p) for p in text.split()]


def to_result(r: dict[str, Any]) -> Result:
    """A `paraphrase batch` line as paraeval.engine.run returns a run: one Completion per model
    call, with the client's first-token time and total (D-Bus included) in place of the
    engine's, so both backends time what the user waits for."""
    calls = [
        Completion(
            text="",
            ttft_s=c["ttft_s"],
            total_s=c["total_s"],
            prompt_tokens=c["prompt_tokens"],
            gen_tokens=c["generated_tokens"],
            prompt_ms=c["ttft_s"] * 1000,
            gen_ms=max(0.0, c["total_s"] - c["ttft_s"]) * 1000,
            cached_tokens=c["cached_tokens"],
            finish_reason=r["finish"],
            draft_tokens=c["draft_tokens"],
            draft_accepted=c["draft_accepted"],
        )
        for c in r["calls"]
    ]
    if r.get("client_ttft_s"):
        calls[0].ttft_s = r["client_ttft_s"]
    calls[-1].total_s += r["client_total_s"] - sum(c.total_s for c in calls)
    calls[-1].text = r["output"]
    calls[-1].texts = r.get("replies") or [r["output"]]
    return Result(r["output"], calls[-1], calls, retried=r["retried"], warning=r["warning"])


class Daemon:
    """Context manager: a private daemon for one model, through `paraphrase batch`."""

    def __init__(self, model_id: str, profile: Profile, build_dir: Path | None = None) -> None:
        if profile.build != "cpu":
            raise SystemExit(f"profile {profile.name}: the daemon runs on the CPU only (ADR-016)")
        self.model_id = model_id
        self.profile = profile
        self.build_dir = build_dir or common.ROOT / "build" / "dev"
        self.proc: subprocess.Popen[str] | None = None
        self.load_s = 0.0
        self._peak_kb = 0
        self._stop = threading.Event()
        self._watch: threading.Thread | None = None
        # The CLI's and the daemon's messages, for an error report; closed in __exit__.
        self._log = tempfile.TemporaryFile(mode="w+")  # noqa: SIM115

    def command(self) -> list[str]:
        cli = [str(common.ROOT / "scripts" / "dev-run.sh"), "--", "batch", "--model", self.model_id]
        return limits_prefix(self.profile) + cli

    def __enter__(self) -> Daemon:
        env = dict(os.environ)
        env |= {
            "PARAPHRASE_RUN": "cli/paraphrase",
            "PARAPHRASE_BUILD_DIR": str(self.build_dir),
            # A portable build loads its CPU libraries from its bin/ (ADR-008).
            "PARAPHRASE_GGML_BACKEND_DIR": str(self.build_dir / "bin"),
            "PARAPHRASE_PRIVATE": "1",  # its own daemon: nothing else shares the model
            "PARAPHRASE_MODELS_DIR": str(common.MODELS_DIR),
            "QT_LOGGING_RULES": "paraphrase.*.debug=false",
        }
        self.proc = subprocess.Popen(
            self.command(),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=self._log,
            text=True,
            encoding="utf-8",
            env=env,
            cwd=common.ROOT,
        )
        self._watch = threading.Thread(target=self._watch_rss, daemon=True)
        self._watch.start()
        first = self._request(WARM_UP)  # loads the model
        self.load_s = float(first.get("load_s", 0.0))
        return self

    def __exit__(self, *exc: object) -> None:
        self._stop.set()
        if self.proc:
            if self.proc.stdin:
                self.proc.stdin.close()
            try:
                self.proc.wait(timeout=30)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        self._log.close()

    def _request(self, request: dict[str, Any]) -> dict[str, Any]:
        assert self.proc and self.proc.stdin and self.proc.stdout
        self.proc.stdin.write(json.dumps(request, ensure_ascii=False) + "\n")
        self.proc.stdin.flush()
        line = self.proc.stdout.readline()
        if not line:
            self._log.seek(0)
            raise RuntimeError(f"paraphrase batch ended:\n{self._log.read()[-3000:]}")
        result: dict[str, Any] = json.loads(line)
        if result.get("error") == "incomplete":
            return result  # the length check refused the answer: scored as an empty output
        if "error" in result:
            raise RuntimeError(f"{request['skill']}: {result['error']}: {result.get('message')}")
        return result

    def run(self, skill: Skill, inputs: dict[str, str]) -> Result:
        """One skill run, as paraeval.engine.run returns it."""
        request = {"skill": skill.id, "input": {k: inputs[k] for k in INPUT_KEYS if inputs.get(k)}}
        r = self._request(request)
        if r.get("error") == "incomplete":
            total = float(r.get("client_total_s", 0.0))
            c = Completion("", total, total, 0, 0, total * 1000, 0.0, 0, "incomplete")
            return Result("", c, [c], retried=True, warning="incomplete", incomplete=True)
        return to_result(r)

    def _daemon_pid(self) -> int | None:
        return next(iter(_children(self.proc.pid)), None) if self.proc else None

    def _watch_rss(self) -> None:
        while not self._stop.is_set():
            pid = self._daemon_pid()
            try:
                for line in Path(f"/proc/{pid}/status").read_text().splitlines():
                    if line.startswith("VmHWM:"):
                        self._peak_kb = max(self._peak_kb, int(line.split()[1]))
            except (OSError, ValueError):
                pass
            self._stop.wait(0.2)

    @property
    def peak_rss_mb(self) -> float:
        return self._peak_kb / 1024
