#!/usr/bin/env python3
"""Execute the freestanding RAFCODEPHI gate under QEMU user-mode.

This validator closes the gap between a static ELF build and an actually
executed control gate without pretending that host emulation is Android
device evidence.

Proved here:
  * target ELF starts and serves --help;
  * an empty PREFIX reports required executables as TOKEN_VAZIO and exits 26;
  * a synthetic executable-complete PREFIX reports OBSERVED and exits 0;
  * --run on a missing payload fails closed with exit 126.

Not proved here:
  * Android installation/runtime;
  * pkg/PRoot/Ninja/QEMU payload execution;
  * physical-device behavior.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

TOOLS: tuple[tuple[str, bool], ...] = (
    ("pkg", True),
    ("proot", True),
    ("proot-distro", True),
    ("ninja", True),
    ("clang", True),
    ("cmake", True),
    ("qemu-system-x86_64", False),
)
REQUIRED_COUNT = sum(1 for _, required in TOOLS if required)
EXPECTED_EMPTY_PREFIX_RC = 20 + REQUIRED_COUNT


def fail(message: str) -> "NoReturn":
    raise SystemExit(f"[freestanding-emulated-runtime][ERROR] {message}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run_gate(
    emulator: str,
    binary: Path,
    args: list[str],
    prefix: Path,
) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env["PREFIX"] = str(prefix)
    return subprocess.run(
        [emulator, str(binary), *args],
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def require_line(text: str, line: str, context: str) -> None:
    lines = text.splitlines()
    if line not in lines:
        fail(f"{context}: expected line missing: {line!r}; got={lines!r}")


def write_log(path: Path, result: subprocess.CompletedProcess[str]) -> None:
    path.write_text(
        f"exit_code={result.returncode}\n"
        "--- stdout ---\n"
        f"{result.stdout}"
        "--- stderr ---\n"
        f"{result.stderr}",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--arch", choices=("aarch64", "armv7"), required=True)
    parser.add_argument("--emulator")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    binary = args.binary.resolve()
    output = args.output.resolve()
    if not binary.is_file():
        fail(f"binary missing: {binary}")
    if binary.read_bytes()[:4] != b"\x7fELF":
        fail(f"binary is not ELF: {binary}")

    default_emulator = "qemu-aarch64" if args.arch == "aarch64" else "qemu-arm"
    emulator_name = args.emulator or default_emulator
    emulator = shutil.which(emulator_name)
    if not emulator:
        fail(f"QEMU user emulator not found: {emulator_name}")

    output.parent.mkdir(parents=True, exist_ok=True)
    log_root = output.parent / f"{output.stem}.logs"
    log_root.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix=f"raf-fs-{args.arch}-") as temp:
        root = Path(temp)
        empty_prefix = root / "empty-prefix"
        full_prefix = root / "full-prefix"
        (empty_prefix / "bin").mkdir(parents=True)
        (full_prefix / "bin").mkdir(parents=True)

        help_result = run_gate(emulator, binary, ["--help"], empty_prefix)
        write_log(log_root / "help.log", help_result)
        if help_result.returncode != 0:
            fail(f"--help exit={help_result.returncode}")
        require_line(
            help_result.stdout,
            "rafproot-fs --probe | --pkg-bootstrap | --pkg-vectras | --run TOOL [args...]",
            "--help",
        )

        empty_probe = run_gate(emulator, binary, ["--probe"], empty_prefix)
        write_log(log_root / "probe-empty.log", empty_probe)
        if empty_probe.returncode != EXPECTED_EMPTY_PREFIX_RC:
            fail(
                f"empty-prefix --probe exit={empty_probe.returncode}; "
                f"expected={EXPECTED_EMPTY_PREFIX_RC}"
            )
        require_line(empty_probe.stdout, f"RAFCODEPHI prefix={empty_prefix}", "empty probe")
        for tool, required in TOOLS:
            suffix = " [required]" if required else " [optional]"
            require_line(
                empty_probe.stdout,
                f"TOKEN_VAZIO executable: {tool}{suffix}",
                "empty probe",
            )

        for tool, _required in TOOLS:
            marker = full_prefix / "bin" / tool
            marker.write_bytes(b"RAF_EXEC_MARKER\n")
            marker.chmod(0o700)

        full_probe = run_gate(emulator, binary, ["--probe"], full_prefix)
        write_log(log_root / "probe-full.log", full_probe)
        if full_probe.returncode != 0:
            fail(f"full-prefix --probe exit={full_probe.returncode}")
        require_line(full_probe.stdout, f"RAFCODEPHI prefix={full_prefix}", "full probe")
        for tool, _required in TOOLS:
            require_line(full_probe.stdout, f"OBSERVED executable: {tool}", "full probe")

        missing_exec = run_gate(
            emulator,
            binary,
            ["--run", "ninja", "--version"],
            empty_prefix,
        )
        write_log(log_root / "run-missing.log", missing_exec)
        if missing_exec.returncode != 126:
            fail(f"missing --run exit={missing_exec.returncode}; expected=126")
        require_line(
            missing_exec.stdout,
            "TOKEN_VAZIO executable: ninja",
            "missing exec boundary",
        )

    payload = {
        "schema": "raf.freestanding-runtime-gate.emulated/v1",
        "architecture": args.arch,
        "binary": str(binary),
        "binary_sha256": sha256(binary),
        "emulator": Path(emulator).name,
        "build_state": "BUILD_PROVEN",
        "host_emulated_runtime_state": "RUNTIME_PROVEN_EMULATED",
        "device_runtime_state": "TOKEN_VAZIO",
        "package_payload_runtime_state": "TOKEN_VAZIO",
        "claim_allowed": False,
        "tests": {
            "help_exit": 0,
            "empty_prefix_probe_exit": EXPECTED_EMPTY_PREFIX_RC,
            "empty_prefix_required_missing": REQUIRED_COUNT,
            "full_prefix_probe_exit": 0,
            "missing_exec_exit": 126,
        },
        "boundary": (
            "QEMU user-mode proves execution semantics of the freestanding control gate only; "
            "it is not Android device proof and does not prove pkg/PRoot/Ninja/QEMU payload runtime."
        ),
    }
    output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(payload, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
