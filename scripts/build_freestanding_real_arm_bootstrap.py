#!/usr/bin/env python3
"""Build a real ARM RAFCODEPHI bootstrap and embed the freestanding exec gate.

This is the cold-start path for a fresh prefix:

    real Termux apt/dpkg/pkg/proot core
        +
    static no-libc RAFCODEPHI control/exec gate

The gate is built on the host and embedded as libexec/rafproot-fs, so the
Android prefix does not need clang before it can use the gate to inspect and
launch pkg/PRoot/Ninja/QEMU payloads.

Evidence is deliberately split: a successful host build is BUILD_PROVEN;
Android runtime remains TOKEN_VAZIO until the generated bootstrap is installed
and exercised on the target device.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REAL_BUILDER = ROOT / "scripts" / "build_real_arm_bootstrap_core.py"
REAL_VALIDATOR = ROOT / "scripts" / "validate_real_arm_bootstrap_core.py"
GATE_SOURCE = ROOT / "bootstrap" / "proot_freestanding.c"
DEFAULT_OUTPUT = ROOT / "out" / "real-arm-bootstrap-core"
DEFAULT_CACHE = ROOT / "out" / "termux-deb-cache"
DEFAULT_REPO = "https://packages.termux.dev/apt/termux-main"
ARCHES = ("aarch64", "arm")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def fail(message: str) -> None:
    raise SystemExit(f"[freestanding-real-arm-bootstrap][ERROR] {message}")


def target_for_arch(arch: str) -> tuple[str, list[str]]:
    if arch == "aarch64":
        return "aarch64-linux-android21", []
    if arch == "arm":
        # Existing real bootstrap declares API 28 for the 32-bit ARM payload.
        return "armv7a-linux-androideabi28", ["-marm"]
    fail(f"unsupported arch: {arch}")


def compile_gate(arch: str, output_dir: Path) -> Path:
    compiler = os.environ.get("RAFCODEPHI_FREESTANDING_CC", "clang")
    compiler_path = shutil.which(compiler)
    if not compiler_path:
        fail(f"freestanding compiler not found: {compiler}")
    if not GATE_SOURCE.is_file():
        fail(f"gate source missing: {GATE_SOURCE}")

    target, extra = target_for_arch(arch)
    output_dir.mkdir(parents=True, exist_ok=True)
    gate = output_dir / f"rafproot-fs-{arch}"
    command = [
        compiler_path,
        f"--target={target}",
        *extra,
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-ffreestanding",
        "-fno-builtin",
        "-fno-stack-protector",
        "-fomit-frame-pointer",
        "-nostdlib",
        "-static",
        "-Wl,-no-pie,-e,_start,--gc-sections",
        str(GATE_SOURCE),
        "-o",
        str(gate),
    ]
    subprocess.run(command, cwd=ROOT, check=True)
    assert_static_elf(gate)
    return gate


def assert_static_elf(path: Path) -> None:
    data = path.read_bytes()
    if len(data) < 64 or data[:4] != b"\x7fELF":
        fail(f"gate is not ELF: {path}")

    readelf = shutil.which("readelf") or shutil.which("llvm-readelf")
    if readelf:
        program_headers = subprocess.run(
            [readelf, "-l", str(path)],
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        ).stdout
        if "INTERP" in program_headers:
            fail(f"dynamic interpreter detected in {path}")

        dynamic = subprocess.run(
            [readelf, "-d", str(path)],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        ).stdout
        if "NEEDED" in dynamic:
            fail(f"dynamic dependency detected in {path}")


def augment_bootstrap_info(raw: bytes, gate_digest: str) -> bytes:
    text = raw.decode("utf-8")
    additions = {
        "BOOTSTRAP_FREESTANDING_GATE_READY": "1",
        "BOOTSTRAP_FREESTANDING_GATE_PATH": "libexec/rafproot-fs",
        "BOOTSTRAP_FREESTANDING_GATE_SHA256": gate_digest,
        "BOOTSTRAP_FREESTANDING_DEVICE_STATE": "TOKEN_VAZIO",
    }
    for key, value in additions.items():
        text = re.sub(rf"(?m)^{re.escape(key)}=.*$", "", text)
        text = text.rstrip() + f"\n{key}={value}\n"
    return text.encode("utf-8")


def clone_info(info: zipfile.ZipInfo) -> zipfile.ZipInfo:
    cloned = zipfile.ZipInfo(info.filename, info.date_time)
    cloned.compress_type = info.compress_type
    cloned.comment = info.comment
    cloned.extra = info.extra
    cloned.create_system = info.create_system
    cloned.create_version = info.create_version
    cloned.extract_version = info.extract_version
    cloned.flag_bits = info.flag_bits
    cloned.volume = info.volume
    cloned.internal_attr = info.internal_attr
    cloned.external_attr = info.external_attr
    return cloned


def inject_gate(zip_path: Path, gate: Path) -> None:
    if not zip_path.is_file():
        fail(f"bootstrap zip missing: {zip_path}")
    gate_digest = sha256(gate)

    with tempfile.NamedTemporaryFile(
        prefix=zip_path.name + ".", suffix=".tmp", dir=zip_path.parent, delete=False
    ) as temp_file:
        temp_path = Path(temp_file.name)

    try:
        with zipfile.ZipFile(zip_path, "r") as source, zipfile.ZipFile(
            temp_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9
        ) as target:
            names = {info.filename for info in source.infolist()}
            if "BOOTSTRAP_INFO" not in names:
                fail(f"BOOTSTRAP_INFO missing from {zip_path}")

            for info in source.infolist():
                if info.filename == "libexec/rafproot-fs":
                    continue
                data = source.read(info.filename)
                if info.filename == "BOOTSTRAP_INFO":
                    data = augment_bootstrap_info(data, gate_digest)
                target.writestr(clone_info(info), data)

            if "libexec/" not in names:
                directory = zipfile.ZipInfo("libexec/")
                directory.external_attr = (0o040000 | 0o700) << 16
                target.writestr(directory, b"")

            gate_info = zipfile.ZipInfo("libexec/rafproot-fs")
            gate_info.compress_type = zipfile.ZIP_DEFLATED
            gate_info.create_system = 3
            gate_info.external_attr = (0o100000 | 0o700) << 16
            target.writestr(gate_info, gate.read_bytes())

        os.replace(temp_path, zip_path)
    finally:
        temp_path.unlink(missing_ok=True)


def update_manifest(manifest: Path, zip_path: Path, gate: Path) -> None:
    if not manifest.is_file():
        fail(f"real-core manifest missing: {manifest}")
    text = manifest.read_text(encoding="utf-8")
    new_zip_sha = sha256(zip_path)
    text, count = re.subn(
        r"(?m)^sha256: `[^`]+`$",
        f"sha256: `{new_zip_sha}`",
        text,
        count=1,
    )
    if count != 1:
        fail(f"could not update bootstrap sha256 in manifest: {manifest}")
    section = (
        "\n## Freestanding control/exec gate\n\n"
        "- path: `libexec/rafproot-fs`\n"
        f"- sha256: `{sha256(gate)}`\n"
        "- host build state: `BUILD_PROVEN`\n"
        "- Android device runtime state: `TOKEN_VAZIO`\n"
        "- claim_allowed: `false` until physical-device execution\n"
    )
    text = re.sub(
        r"\n## Freestanding control/exec gate\n.*?(?=\n## |\Z)",
        "",
        text,
        flags=re.S,
    ).rstrip() + "\n" + section
    manifest.write_text(text, encoding="utf-8")


def write_receipt(output: Path, arch: str, zip_path: Path, gate: Path) -> Path:
    receipt = output / arch / "freestanding_gate_receipt.json"
    receipt.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "schema": "raf.freestanding-real-arm-bootstrap.v1",
        "architecture": arch,
        "gate_path": "libexec/rafproot-fs",
        "gate_sha256": sha256(gate),
        "bootstrap_zip": str(zip_path.relative_to(ROOT)),
        "bootstrap_sha256": sha256(zip_path),
        "source_state": "SOURCE_OBSERVED",
        "wire_state": "WIRED",
        "build_state": "BUILD_PROVEN",
        "runtime_state": "TOKEN_VAZIO",
        "device_state": "TOKEN_VAZIO",
        "claim_allowed": False,
    }
    receipt.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return receipt


def build_real_core(args: argparse.Namespace) -> None:
    command = [
        sys.executable,
        str(REAL_BUILDER),
        "--repo",
        args.repo,
        "--arch",
        args.arch,
        "--output",
        str(args.output),
        "--cache",
        str(args.cache),
    ]
    subprocess.run(command, cwd=ROOT, check=True)


def validate(zip_paths: list[Path]) -> None:
    if not REAL_VALIDATOR.is_file():
        fail(f"validator missing: {REAL_VALIDATOR}")
    subprocess.run(
        [sys.executable, str(REAL_VALIDATOR), *(str(path) for path in zip_paths)],
        cwd=ROOT,
        check=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default=DEFAULT_REPO)
    parser.add_argument("--arch", choices=ARCHES + ("all",), default="all")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--cache", type=Path, default=DEFAULT_CACHE)
    parser.add_argument("--skip-real-core-build", action="store_true")
    parser.add_argument("--skip-validator", action="store_true")
    args = parser.parse_args()

    args.output = args.output.resolve()
    args.cache = args.cache.resolve()

    if not args.skip_real_core_build:
        build_real_core(args)

    arches = ARCHES if args.arch == "all" else (args.arch,)
    zip_paths: list[Path] = []
    for arch in arches:
        gate = compile_gate(arch, args.output / arch / "freestanding")
        zip_path = ROOT / "app" / "src" / "main" / "cpp" / f"rewritten-bootstrap-{arch}.zip"
        manifest = args.output / arch / f"real_arm_bootstrap_core_{arch}.md"
        inject_gate(zip_path, gate)
        update_manifest(manifest, zip_path, gate)
        receipt = write_receipt(args.output, arch, zip_path, gate)
        zip_paths.append(zip_path)
        print(
            f"[freestanding-real-arm-bootstrap] arch={arch} "
            f"gate_sha256={sha256(gate)} bootstrap_sha256={sha256(zip_path)} "
            f"receipt={receipt}"
        )

    if not args.skip_validator:
        validate(zip_paths)

    print("[freestanding-real-arm-bootstrap] BUILD_PROVEN host artifact; DEVICE=TOKEN_VAZIO")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
