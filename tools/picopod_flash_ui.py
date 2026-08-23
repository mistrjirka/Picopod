#!/usr/bin/env python3
"""Local browser UI for building and flashing Picopod firmware.

The UI intentionally uses only Python's standard library. It builds remote
production branches in isolated cached Git worktrees, so the user's current
checkout is never modified.
"""

from __future__ import annotations

import argparse
import contextlib
import dataclasses
import glob
import hashlib
import html
import json
import os
from pathlib import Path
import queue
import re
import secrets
import shlex
import shutil
import signal
import subprocess
import sys
import threading
import time
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Iterable

try:
    import fcntl
except ImportError:  # pragma: no cover - supported Arch/Linux has it
    fcntl = None
import urllib.parse
import webbrowser

DEFAULT_REPOSITORY = "https://github.com/mistrjirka/Picopod.git"
DEFAULT_SUFFIX = " [echo]"
MAX_SUFFIX_BYTES = 64
MAX_LOG_LINES = 6000
TERMINAL_JOB_STATES = {"succeeded", "failed", "cancelled"}


@dataclasses.dataclass(frozen=True)
class DeviceDefinition:
    key: str
    label: str
    branch: str
    environment: str
    default_node_id: int
    upload_hint: str


DEVICES: dict[str, DeviceDefinition] = {
    "watch": DeviceDefinition(
        key="watch",
        label="LilyGo T-Watch S3",
        branch="espwatch",
        environment="twatch-s3",
        default_node_id=3,
        upload_hint="Connect the watch by USB. Leave the port on Auto unless several serial devices are attached.",
    ),
    "heltec": DeviceDefinition(
        key="heltec",
        label="Heltec Wireless Stick Lite V3",
        branch="heltec-stick-lite-v3",
        environment="heltec_wireless_stick_lite_v3",
        default_node_id=12,
        upload_hint="Connect the Heltec board by USB. Normally PlatformIO detects its serial port automatically.",
    ),
    "rp2040": DeviceDefinition(
        key="rp2040",
        label="Custom RP2040 Picopod",
        branch="refactor-to-radiolib",
        environment="picopod_rp2040",
        default_node_id=6,
        upload_hint="Put the RP2040 into BOOTSEL mode when requested. Auto-detect is preferred for picotool.",
    ),
}


class UserInputError(ValueError):
    pass


class JobCancelled(RuntimeError):
    pass


def validate_node_id(value: Any) -> int:
    if isinstance(value, bool):
        raise UserInputError("Node ID must be an integer from 1 to 65535.")
    if isinstance(value, int):
        node_id = value
    elif isinstance(value, str) and re.fullmatch(r"[0-9]+", value.strip()):
        node_id = int(value.strip(), 10)
    else:
        raise UserInputError("Node ID must be an integer from 1 to 65535.")
    if not 1 <= node_id <= 65535:
        raise UserInputError("Node ID 0 is broadcast; choose a unique ID from 1 to 65535.")
    return node_id


def validate_suffix(value: Any, debug_echo: bool) -> str:
    suffix = str(value if value is not None else DEFAULT_SUFFIX)
    try:
        encoded = suffix.encode("utf-8")
    except UnicodeEncodeError as exc:
        raise UserInputError("The echo suffix must be valid UTF-8 text.") from exc
    if debug_echo and not encoded:
        raise UserInputError("Debug echo needs a non-empty suffix.")
    if b"\x00" in encoded:
        raise UserInputError("The echo suffix cannot contain a NUL byte.")
    if len(encoded) > MAX_SUFFIX_BYTES:
        raise UserInputError(
            f"The echo suffix is {len(encoded)} UTF-8 bytes; the maximum is {MAX_SUFFIX_BYTES}."
        )
    return suffix


def validate_upload_port(value: Any) -> str:
    port = str(value or "").strip()
    if not port or port == "auto":
        return ""
    if len(port) > 240 or any(ord(char) < 32 or ord(char) == 127 for char in port):
        raise UserInputError("Invalid upload-port value.")
    if os.name == "posix":
        allowed = re.fullmatch(
            r"/dev/(?:tty(?:ACM|USB|S)[0-9]+|cu\.[A-Za-z0-9._-]+|serial/by-id/[A-Za-z0-9._:+-]+)",
            port,
        )
        if not allowed or os.path.normpath(port) != port:
            raise UserInputError(
                "On Linux, choose an enumerated /dev/tty*, /dev/cu.* or /dev/serial/by-id port."
            )
    return port


def parse_bool(value: Any, field_name: str) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, int) and not isinstance(value, bool) and value in (0, 1):
        return bool(value)
    if isinstance(value, str):
        normalized = value.strip().lower()
        if normalized in {"1", "true", "yes", "on"}:
            return True
        if normalized in {"0", "false", "no", "off"}:
            return False
    raise UserInputError(f"{field_name} must be a boolean.")


def find_pio(explicit: str | None = None) -> str:
    candidates: list[Path | str] = []
    if explicit:
        candidates.append(explicit)
    env_value = os.environ.get("PICOPOD_PIO")
    if env_value:
        candidates.append(env_value)
    candidates.extend(
        [
            Path(sys.executable).with_name("pio"),
            Path.cwd() / ".venv" / "bin" / "pio",
            Path.home() / ".platformio" / "penv" / "bin" / "pio",
            "pio",
        ]
    )
    for candidate in candidates:
        if isinstance(candidate, Path):
            if candidate.is_file() and os.access(candidate, os.X_OK):
                return str(candidate.resolve())
        else:
            resolved = shutil.which(candidate)
            if resolved:
                return resolved
    raise UserInputError(
        "PlatformIO was not found. On Arch Linux, create a virtual environment and run "
        "`python -m venv .venv && .venv/bin/pip install platformio`."
    )


def run_checked(command: list[str], cwd: Path | None = None) -> str:
    completed = subprocess.run(
        command,
        cwd=str(cwd) if cwd else None,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"Command failed ({completed.returncode}): {shlex.join(command)}\n{completed.stdout}"
        )
    return completed.stdout


def discover_ports(pio: str | None = None) -> list[dict[str, str]]:
    seen: set[str] = set()
    ports: list[dict[str, str]] = []
    if pio:
        try:
            result = subprocess.run(
                [pio, "device", "list", "--json-output"],
                check=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                text=True,
                timeout=15,
            )
            if result.returncode == 0 and result.stdout.strip():
                data = json.loads(result.stdout)
                for item in data if isinstance(data, list) else []:
                    port = str(item.get("port", ""))
                    if not port or port in seen:
                        continue
                    seen.add(port)
                    description = str(item.get("description") or item.get("hwid") or "serial device")
                    ports.append({"port": port, "label": f"{port} — {description}"})
        except (OSError, ValueError, subprocess.SubprocessError):
            pass
    if os.name == "posix":
        patterns = (
            "/dev/serial/by-id/*",
            "/dev/ttyACM*",
            "/dev/ttyUSB*",
            "/dev/cu.usb*",
        )
        for pattern in patterns:
            for raw in sorted(glob.glob(pattern)):
                port = os.path.realpath(raw) if "/by-id/" in raw else raw
                if port in seen:
                    continue
                seen.add(port)
                ports.append({"port": port, "label": raw})
    return ports


@dataclasses.dataclass(frozen=True)
class BuildRequest:
    device: DeviceDefinition
    node_id: int
    debug_echo: bool
    suffix: str
    action: str
    upload_port: str
    clean: bool

    @classmethod
    def from_json(cls, payload: dict[str, Any]) -> "BuildRequest":
        allowed_fields = {
            "device", "node_id", "debug_echo", "suffix",
            "action", "upload_port", "clean",
        }
        unknown_fields = sorted(set(payload) - allowed_fields)
        if unknown_fields:
            raise UserInputError(
                "Unknown request field(s): " + ", ".join(unknown_fields)
            )
        device_key = str(payload.get("device", ""))
        if device_key not in DEVICES:
            raise UserInputError("Unknown device type.")
        action = str(payload.get("action", "flash"))
        if action not in {"build", "flash"}:
            raise UserInputError("Action must be build or flash.")
        debug_echo = parse_bool(payload.get("debug_echo", False), "debug_echo")
        return cls(
            device=DEVICES[device_key],
            node_id=validate_node_id(payload.get("node_id")),
            debug_echo=debug_echo,
            suffix=validate_suffix(payload.get("suffix", DEFAULT_SUFFIX), debug_echo),
            action=action,
            upload_port=validate_upload_port(payload.get("upload_port", "")),
            clean=parse_bool(payload.get("clean", False), "clean"),
        )

    def environment(self) -> dict[str, str]:
        environment = os.environ.copy()
        environment["PICOPOD_NODE_ID"] = str(self.node_id)
        environment["PICOPOD_DEBUG_ECHO"] = "1" if self.debug_echo else "0"
        environment["PICOPOD_DEBUG_ECHO_SUFFIX"] = self.suffix
        environment["PYTHONUNBUFFERED"] = "1"
        return environment


class WorkspaceManager:
    def __init__(self, repository: str, cache_root: Path):
        self.repository = repository
        self.cache_root = cache_root.expanduser().resolve()
        self.git_dir = self.cache_root / "repository.git"
        self.worktrees = self.cache_root / "worktrees"

    def _git_dir_command(self, *arguments: str) -> list[str]:
        return ["git", f"--git-dir={self.git_dir}", *arguments]

    @contextlib.contextmanager
    def operation_lock(self):
        """Serialize cached worktree/config/upload use across UI processes."""
        self.cache_root.mkdir(parents=True, exist_ok=True)
        lock_path = self.cache_root / "operation.lock"
        with lock_path.open("a+") as lock_file:
            if fcntl is not None:
                try:
                    fcntl.flock(lock_file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
                except BlockingIOError as exc:
                    raise UserInputError(
                        "Another Picopod flasher process is using this build cache."
                    ) from exc
            try:
                yield
            finally:
                if fcntl is not None:
                    fcntl.flock(lock_file.fileno(), fcntl.LOCK_UN)

    def ensure_repository(self, log: callable) -> None:
        self.cache_root.mkdir(parents=True, exist_ok=True)
        self.worktrees.mkdir(parents=True, exist_ok=True)
        if not self.git_dir.exists():
            log(f"Cloning Picopod build repository into {self.git_dir}")
            run_checked(["git", "clone", "--bare", self.repository, str(self.git_dir)])
        log("Fetching current production branches")
        run_checked(
            self._git_dir_command(
                "fetch",
                "--prune",
                "origin",
                "+refs/heads/*:refs/remotes/origin/*",
            )
        )

    def prepare(self, device: DeviceDefinition, log: callable) -> tuple[Path, str]:
        self.ensure_repository(log)
        ref = f"refs/remotes/origin/{device.branch}"
        commit = run_checked(self._git_dir_command("rev-parse", ref)).strip()
        worktree = self.worktrees / device.key
        if (worktree / ".git").exists():
            log(f"Updating cached {device.label} source to {commit[:12]}")
            run_checked(["git", "-C", str(worktree), "reset", "--hard", commit])
            run_checked(
                [
                    "git",
                    "-C",
                    str(worktree),
                    "clean",
                    "-ffd",
                    "-e",
                    ".pio/",
                ]
            )
        else:
            if worktree.exists():
                shutil.rmtree(worktree)
            log(f"Creating isolated {device.label} worktree")
            run_checked(
                self._git_dir_command(
                    "worktree", "add", "--force", "--detach", str(worktree), commit
                )
            )
        log("Synchronizing DTProtocol submodule")
        run_checked(["git", "-C", str(worktree), "submodule", "sync", "--recursive"])
        run_checked(
            [
                "git",
                "-C",
                str(worktree),
                "submodule",
                "update",
                "--init",
                "--recursive",
            ]
        )
        for generated in (
            worktree / "include" / "PicopodGeneratedConfig.h",
            worktree / "include" / "PicopodGeneratedConfig.tmp",
        ):
            generated.unlink(missing_ok=True)
        return worktree, commit


class BuildJob:
    def __init__(
        self,
        request: BuildRequest,
        workspace_manager: WorkspaceManager,
        pio: str,
    ):
        self.request = request
        self.workspace_manager = workspace_manager
        self.pio = pio
        self.identifier = secrets.token_hex(8)
        self.created_at = time.time()
        self.started_at: float | None = None
        self.finished_at: float | None = None
        self.state = "queued"
        self.exit_code: int | None = None
        self.error: str | None = None
        self.commit: str | None = None
        self.artifact: str | None = None
        self._logs: list[str] = []
        self._log_base = 0
        self._lock = threading.Lock()
        self._process_lock = threading.Lock()
        self._process: subprocess.Popen[str] | None = None
        self._cancel_requested = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)

    def log(self, line: str) -> None:
        normalized = line.rstrip("\r\n")
        with self._lock:
            self._logs.append(normalized)
            excess = len(self._logs) - MAX_LOG_LINES
            if excess > 0:
                del self._logs[:excess]
                self._log_base += excess

    def start(self) -> None:
        self._thread.start()

    def cancel(self) -> None:
        self._cancel_requested.set()
        with self._process_lock:
            process = self._process
        if process is None or process.poll() is not None:
            return
        try:
            if os.name == "posix":
                os.killpg(process.pid, signal.SIGTERM)
            else:  # pragma: no cover - supported deployment is Arch Linux
                process.terminate()
        except (OSError, ProcessLookupError):
            return

        def force_kill_if_needed() -> None:
            try:
                process.wait(timeout=3)
                return
            except subprocess.TimeoutExpired:
                pass
            try:
                if os.name == "posix":
                    os.killpg(process.pid, signal.SIGKILL)
                else:  # pragma: no cover
                    process.kill()
            except (OSError, ProcessLookupError):
                pass

        threading.Thread(target=force_kill_if_needed, daemon=True).start()

    def _check_cancelled(self) -> None:
        if self._cancel_requested.is_set():
            raise JobCancelled("Cancelled by user.")

    def snapshot(self, offset: int = 0) -> dict[str, Any]:
        with self._lock:
            requested = max(0, int(offset))
            first_available = self._log_base
            last_available = first_available + len(self._logs)
            log_reset = requested < first_available
            start = 0 if log_reset else min(requested - first_available, len(self._logs))
            logs = self._logs[start:]
            next_offset = last_available
        return {
            "id": self.identifier,
            "state": self.state,
            "exit_code": self.exit_code,
            "error": self.error,
            "created_at": self.created_at,
            "started_at": self.started_at,
            "finished_at": self.finished_at,
            "commit": self.commit,
            "artifact": self.artifact,
            "logs": logs,
            "log_reset": log_reset,
            "next_offset": next_offset,
        }

    def _stream(self, command: list[str], cwd: Path, environment: dict[str, str]) -> int:
        self._check_cancelled()
        self.log("$ " + shlex.join(command))
        process = subprocess.Popen(
            command,
            cwd=str(cwd),
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            start_new_session=os.name == "posix",
        )
        with self._process_lock:
            self._process = process
        try:
            assert process.stdout is not None
            for line in process.stdout:
                self.log(line)
            code = process.wait()
        finally:
            with self._process_lock:
                if self._process is process:
                    self._process = None
        self._check_cancelled()
        return code

    def _run(self) -> None:
        self.state = "preparing"
        self.started_at = time.time()
        try:
            with self.workspace_manager.operation_lock():
                self._run_locked()
        except JobCancelled as exc:
            self.error = str(exc)
            self.log(str(exc))
            self.state = "cancelled"
            self.exit_code = 130
        except Exception as exc:  # surfaced verbatim in the local UI
            self.error = str(exc)
            self.log("ERROR: " + str(exc))
            self.state = "failed"
            if self.exit_code is None:
                self.exit_code = 1
        finally:
            self.finished_at = time.time()

    def _run_locked(self) -> None:
        self._check_cancelled()
        worktree, commit = self.workspace_manager.prepare(self.request.device, self.log)
        self.commit = commit
        environment = self.request.environment()
        generated = worktree / "include" / "PicopodGeneratedConfig.h"
        generated_tmp = worktree / "include" / "PicopodGeneratedConfig.tmp"
        generated.unlink(missing_ok=True)
        generated_tmp.unlink(missing_ok=True)
        build_dir = worktree / ".pio" / "build" / self.request.device.environment
        for stale_artifact in (
            build_dir / "firmware.bin",
            build_dir / "firmware.uf2",
            build_dir / "firmware.elf",
        ):
            stale_artifact.unlink(missing_ok=True)
        try:
            self.log(
                f"Configuration: device={self.request.device.label}, node={self.request.node_id}, "
                f"debug_echo={'on' if self.request.debug_echo else 'off'}"
            )
            if self.request.debug_echo:
                self.log(
                    f"Echo suffix: {len(self.request.suffix.encode('utf-8'))} UTF-8 bytes"
                )
            self._check_cancelled()
            if self.request.clean:
                self.state = "cleaning"
                code = self._stream(
                    [self.pio, "run", "-e", self.request.device.environment, "-t", "clean"],
                    worktree,
                    environment,
                )
                if code != 0:
                    raise RuntimeError(f"PlatformIO clean failed with exit code {code}.")

            self._check_cancelled()
            self.state = "building" if self.request.action == "build" else "flashing"
            command = [self.pio, "run", "-e", self.request.device.environment]
            if self.request.action == "flash":
                command.extend(["-t", "upload"])
                if self.request.upload_port:
                    command.extend(["--upload-port", self.request.upload_port])
            code = self._stream(command, worktree, environment)
            self.exit_code = code
            if code != 0:
                raise RuntimeError(f"PlatformIO exited with status {code}.")

            candidates = [
                build_dir / "firmware.bin",
                build_dir / "firmware.uf2",
                build_dir / "firmware.elf",
            ]
            artifact = next((candidate for candidate in candidates if candidate.exists()), None)
            if artifact:
                self.artifact = str(artifact)
                self.log(f"Built artifact: {artifact} ({artifact.stat().st_size} bytes)")
            self.state = "succeeded"
        finally:
            generated.unlink(missing_ok=True)
            generated_tmp.unlink(missing_ok=True)


class JobManager:
    def __init__(self, workspace_manager: WorkspaceManager, pio: str):
        self.workspace_manager = workspace_manager
        self.pio = pio
        self._lock = threading.Lock()
        self._current: BuildJob | None = None

    def start(self, request: BuildRequest) -> BuildJob:
        with self._lock:
            if self._current and self._current.state not in TERMINAL_JOB_STATES:
                raise UserInputError("Another build or flash operation is still running.")
            self._current = BuildJob(request, self.workspace_manager, self.pio)
            self._current.start()
            return self._current

    def cancel(self) -> BuildJob:
        with self._lock:
            if not self._current or self._current.state in TERMINAL_JOB_STATES:
                raise UserInputError("There is no active build or flash operation.")
            job = self._current
            job.cancel()
            return job

    def current(self) -> BuildJob | None:
        with self._lock:
            return self._current


HTML_PAGE = r"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Picopod Firmware Flasher</title>
<style>
:root { color-scheme: dark; font-family: system-ui, sans-serif; }
body { margin: 0; background: #0b1220; color: #e7eefc; }
main { max-width: 880px; margin: auto; padding: 24px 16px 48px; }
.card { background: #111c30; border: 1px solid #263754; border-radius: 14px; padding: 18px; margin: 14px 0; }
h1 { margin: 0 0 6px; font-size: 1.7rem; }
p { color: #b9c7dc; line-height: 1.45; }
.grid { display: grid; grid-template-columns: repeat(auto-fit,minmax(240px,1fr)); gap: 14px; }
label { display: block; font-weight: 650; margin-bottom: 5px; }
input, select, button { box-sizing: border-box; width: 100%; font: inherit; border-radius: 9px; border: 1px solid #3a4b67; background: #0b1526; color: #eef5ff; padding: 10px; }
input[type=checkbox] { width: auto; margin-right: 8px; }
.inline { display: flex; align-items: center; min-height: 42px; }
.actions { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 12px; }
button { cursor: pointer; border: 0; background: #3f7ee8; font-weight: 700; }
button.secondary { background: #34445d; }
button:disabled { opacity: .55; cursor: progress; }
small { color: #93a5bf; }
#status { font-weight: 700; }
pre { white-space: pre-wrap; word-break: break-word; background: #07101e; border-radius: 9px; padding: 12px; min-height: 210px; max-height: 480px; overflow: auto; color: #d7e6ff; }
.success { color: #73e29c; } .failure { color: #ff8e8e; }
.warning { color: #ffd57a; }
</style>
</head>
<body><main>
<h1>Picopod firmware flasher</h1>
<p>Choose the hardware and a unique mesh node ID. The source is built in an isolated cache and your current Git checkout is never changed.</p>
<div class="card grid">
<div><label for="device">Device type</label><select id="device"></select><small id="hint"></small></div>
<div><label for="node">Node ID</label><input id="node" type="number" min="1" max="65535" required><small>ID 0 is reserved for broadcast.</small></div>
<div><label for="port">Upload port</label><select id="port"><option value="">Auto-detect</option></select><small>Leave Auto selected unless multiple devices are connected.</small></div>
<div class="inline"><label><input id="clean" type="checkbox">Clean build before compiling</label></div>
</div>
<div class="card">
<div class="inline"><label><input id="debug" type="checkbox">Debug echo node</label></div>
<p>When enabled, every received application message is sent back to its origin with a suffix. Returned packets carry a protocol marker, so another debug node will not repeat them even when it uses a different suffix.</p>
<label for="suffix">Echo suffix</label><input id="suffix" value=" [echo]" maxlength="64"><small>Maximum 64 UTF-8 bytes; the uploader validates this exactly.</small>
</div>
<div class="card actions">
<button id="build" class="secondary">Build only</button><button id="flash">Build and flash</button><button id="cancel" class="secondary" disabled>Cancel</button>
</div>
<div class="card">
<div id="status">Ready</div>
<small id="commit"></small>
<pre id="log"></pre>
</div>
</main>
<script>
const token = __TOKEN__;
const devices = __DEVICES__;
const device = document.getElementById('device');
const node = document.getElementById('node');
const hint = document.getElementById('hint');
const port = document.getElementById('port');
const debug = document.getElementById('debug');
const suffix = document.getElementById('suffix');
const clean = document.getElementById('clean');
const statusEl = document.getElementById('status');
const commitEl = document.getElementById('commit');
const logEl = document.getElementById('log');
const buttons = [document.getElementById('build'), document.getElementById('flash')];
const cancelButton = document.getElementById('cancel');
let logOffset = 0;
for (const item of devices) {
  const option = document.createElement('option'); option.value = item.key; option.textContent = item.label; device.appendChild(option);
}
function updateDevice(resetId=true) {
  const item = devices.find(x => x.key === device.value); hint.textContent = item.upload_hint;
  if (resetId) node.value = item.default_node_id;
}
device.addEventListener('change', () => updateDevice(true)); updateDevice(true);
debug.addEventListener('change', () => { suffix.disabled = !debug.checked; }); suffix.disabled = true;
async function api(path, options={}) {
  options.headers = Object.assign({'X-Picopod-Token': token, 'Content-Type': 'application/json'}, options.headers || {});
  const response = await fetch(path, options); const data = await response.json();
  if (!response.ok) throw new Error(data.error || `HTTP ${response.status}`); return data;
}
async function refreshPorts() {
  try { const data = await api('/api/ports');
    for (const item of data.ports) { const option=document.createElement('option'); option.value=item.port; option.textContent=item.label; port.appendChild(option); }
  } catch (error) { console.warn(error); }
}
refreshPorts();
function setBusy(value) { for (const button of buttons) button.disabled = value; cancelButton.disabled = !value; }
async function start(action) {
  logEl.textContent=''; logOffset=0; commitEl.textContent=''; setBusy(true); statusEl.className=''; statusEl.textContent='Starting…';
  try {
    await api('/api/start', {method:'POST', body:JSON.stringify({device:device.value,node_id:node.value,debug_echo:debug.checked,suffix:suffix.value,upload_port:port.value,clean:clean.checked,action})});
    poll();
  } catch (error) { statusEl.textContent=error.message; statusEl.className='failure'; setBusy(false); }
}
async function poll() {
  try {
    const data = await api(`/api/status?offset=${logOffset}`); logOffset=data.next_offset;
    if (data.log_reset) logEl.textContent='';
    if (data.logs.length) { logEl.textContent += data.logs.join('\n') + '\n'; logEl.scrollTop=logEl.scrollHeight; }
    statusEl.textContent = data.state; commitEl.textContent = data.commit ? `Source commit ${data.commit}` : '';
    if (data.state === 'succeeded') { statusEl.className='success'; setBusy(false); return; }
    if (data.state === 'failed') { statusEl.className='failure'; if(data.error) statusEl.textContent=`Failed: ${data.error}`; setBusy(false); return; }
    if (data.state === 'cancelled') { statusEl.className='warning'; statusEl.textContent='Cancelled'; setBusy(false); return; }
    setTimeout(poll, 600);
  } catch (error) { statusEl.textContent=error.message; statusEl.className='failure'; setBusy(false); }
}
async function cancelJob() {
  cancelButton.disabled=true;
  try { await api('/api/cancel', {method:'POST', body:'{}'}); statusEl.textContent='Cancelling…'; }
  catch (error) { statusEl.textContent=error.message; statusEl.className='failure'; }
}
document.getElementById('build').addEventListener('click', () => start('build'));
document.getElementById('flash').addEventListener('click', () => start('flash'));
cancelButton.addEventListener('click', cancelJob);
</script></body></html>"""


class App:
    def __init__(self, manager: JobManager, token: str):
        self.manager = manager
        self.token = token

    def page(self) -> bytes:
        devices = [dataclasses.asdict(device) for device in DEVICES.values()]
        page = HTML_PAGE.replace("__TOKEN__", json.dumps(self.token)).replace(
            "__DEVICES__", json.dumps(devices)
        )
        return page.encode("utf-8")


class RequestHandler(BaseHTTPRequestHandler):
    server_version = "PicopodFlasher/1.0"

    def _security_headers(self) -> None:
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header(
            "Content-Security-Policy",
            "default-src 'none'; style-src 'unsafe-inline'; "
            "script-src 'unsafe-inline'; connect-src 'self'; "
            "img-src 'self'; base-uri 'none'; frame-ancestors 'none'",
        )

    @property
    def app(self) -> App:
        return self.server.app  # type: ignore[attr-defined]

    def log_message(self, format: str, *args: Any) -> None:
        return

    def _authorized(self) -> bool:
        return self.headers.get("X-Picopod-Token") == self.app.token

    def _json(self, payload: dict[str, Any], status: HTTPStatus = HTTPStatus.OK) -> None:
        data = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self._security_headers()
        self.end_headers()
        self.wfile.write(data)

    def _error(self, message: str, status: HTTPStatus = HTTPStatus.BAD_REQUEST) -> None:
        self._json({"error": message}, status)

    def do_GET(self) -> None:
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/":
            body = self.app.page()
            self.send_response(HTTPStatus.OK)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self._security_headers()
            self.end_headers()
            self.wfile.write(body)
            return
        if not self._authorized():
            self._error("Unauthorized local request.", HTTPStatus.FORBIDDEN)
            return
        if parsed.path == "/api/ports":
            self._json({"ports": discover_ports(self.app.manager.pio)})
            return
        if parsed.path == "/api/status":
            job = self.app.manager.current()
            if not job:
                self._json({"state": "idle", "logs": [], "next_offset": 0})
                return
            query = urllib.parse.parse_qs(parsed.query)
            try:
                offset = int(query.get("offset", ["0"])[0])
            except ValueError:
                offset = 0
            self._json(job.snapshot(offset))
            return
        self._error("Not found.", HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:
        parsed = urllib.parse.urlparse(self.path)
        if not self._authorized():
            self._error("Unauthorized local request.", HTTPStatus.FORBIDDEN)
            return
        if parsed.path == "/api/cancel":
            try:
                job = self.app.manager.cancel()
                self._json({"job_id": job.identifier, "state": "cancelling"})
            except UserInputError as exc:
                self._error(str(exc))
            return
        if parsed.path != "/api/start":
            self._error("Not found.", HTTPStatus.NOT_FOUND)
            return
        try:
            if self.headers.get_content_type() != "application/json":
                raise UserInputError("Content-Type must be application/json.")
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > 16384:
                raise UserInputError("Invalid request size.")
            payload = json.loads(self.rfile.read(length))
            if not isinstance(payload, dict):
                raise UserInputError("Expected a JSON object.")
            request = BuildRequest.from_json(payload)
            job = self.app.manager.start(request)
            self._json({"job_id": job.identifier}, HTTPStatus.ACCEPTED)
        except UserInputError as exc:
            self._error(str(exc))
        except (ValueError, json.JSONDecodeError) as exc:
            self._error(f"Invalid request: {exc}")


def create_server(app: App, host: str, port: int) -> ThreadingHTTPServer:
    server = ThreadingHTTPServer((host, port), RequestHandler)
    server.app = app  # type: ignore[attr-defined]
    return server


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true")
    parser.add_argument("--repository", default=DEFAULT_REPOSITORY)
    parser.add_argument(
        "--cache",
        type=Path,
        default=Path.home() / ".cache" / "picopod-flasher",
    )
    parser.add_argument("--pio")
    args = parser.parse_args(argv)
    if args.host not in {"127.0.0.1", "localhost", "::1"}:
        parser.error("The flasher intentionally binds only to the local machine.")
    try:
        pio = find_pio(args.pio)
    except UserInputError as exc:
        print(exc, file=sys.stderr)
        return 2
    token = secrets.token_urlsafe(24)
    workspace = WorkspaceManager(args.repository, args.cache)
    manager = JobManager(workspace, pio)
    server = create_server(App(manager, token), args.host, args.port)
    url = f"http://{args.host}:{server.server_address[1]}/"
    print(f"Picopod flasher: {url}")
    print("Press Ctrl+C to stop it.")
    if not args.no_browser:
        threading.Timer(0.4, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever(poll_interval=0.3)
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
