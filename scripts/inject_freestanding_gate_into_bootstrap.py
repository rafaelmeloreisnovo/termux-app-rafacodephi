#!/usr/bin/env python3
"""Inject the RAFCODEPHI static freestanding gate into source-built ARM bootstraps.

This script is intentionally source-agnostic about how the real bootstrap was
built. Its input must already be a real-pkg bootstrap. It binds the resulting
child artifact to the source-bootstrap manifest hash and never widens device
claims on the host.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "bootstrap" / "proot_freestanding.c"
ARCHES = {
    "arm": ("armv7a-linux-androideabi21", ["-marm"]),
    "aarch64": ("aarch64-linux-android21", []),
}


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def die(message: str) -> None:
    raise SystemExit(f"[freestanding-bootstrap-inject][ERROR] {message}")


def compile_gate(arch: str, out_dir: Path) -> Path:
    compiler = shutil.which(os.environ.get("RAFCODEPHI_FREESTANDING_CC", "clang"))
    if not compiler:
        die("clang not found")
    target, extra = ARCHES[arch]
    out_dir.mkdir(parents=True, exist_ok=True)
    out = out_dir / f"rafproot-fs-{arch}"
    subprocess.run([
        compiler, f"--target={target}", *extra,
        "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-ffreestanding", "-fno-builtin", "-fno-stack-protector", "-fomit-frame-pointer",
        "-nostdlib", "-static", "-Wl,-no-pie,-e,_start,--gc-sections",
        str(SOURCE), "-o", str(out),
    ], cwd=ROOT, check=True)
    validate_static_elf(out)
    return out


def validate_static_elf(path: Path) -> None:
    data = path.read_bytes()
    if data[:4] != b"\x7fELF":
        die(f"not ELF: {path}")
    readelf = shutil.which("readelf") or shutil.which("llvm-readelf")
    if not readelf:
        return
    ph = subprocess.run([readelf, "-l", str(path)], check=True, text=True,
                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
    if "INTERP" in ph:
        die(f"PT_INTERP present: {path}")
    dyn = subprocess.run([readelf, "-d", str(path)], check=False, text=True,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
    if "NEEDED" in dyn:
        die(f"DT_NEEDED present: {path}")
    symbols = subprocess.run([readelf, "-Ws", str(path)], check=True, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
    for line in symbols.splitlines():
        fields = line.split()
        if len(fields) >= 8 and fields[6] == "UND" and fields[7]:
            die(f"undefined symbol {fields[7]} in {path}")


def parse_info(raw: bytes) -> dict[str, str]:
    fields: dict[str, str] = {}
    for line in raw.decode("utf-8", "strict").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            fields[key] = value
    return fields


def rewrite_info(raw: bytes, gate_sha: str, source_manifest_sha: str) -> bytes:
    lines = [line for line in raw.decode("utf-8", "strict").splitlines()
             if not line.startswith("BOOTSTRAP_FREESTANDING_")]
    lines.extend([
        "BOOTSTRAP_FREESTANDING_GATE_READY=1",
        "BOOTSTRAP_FREESTANDING_GATE_PATH=libexec/rafproot-fs",
        f"BOOTSTRAP_FREESTANDING_GATE_SHA256={gate_sha}",
        f"BOOTSTRAP_FREESTANDING_SOURCE_MANIFEST_SHA256={source_manifest_sha}",
        "BOOTSTRAP_FREESTANDING_BUILD_STATE=BUILD_PROVEN",
        "BOOTSTRAP_FREESTANDING_DEVICE_STATE=TOKEN_VAZIO",
        "BOOTSTRAP_FREESTANDING_CLAIM_ALLOWED=false",
    ])
    return ("\n".join(lines).rstrip() + "\n").encode("utf-8")


def clone_info(info: zipfile.ZipInfo) -> zipfile.ZipInfo:
    out = zipfile.ZipInfo(info.filename, info.date_time)
    out.compress_type = info.compress_type
    out.comment = info.comment
    out.extra = info.extra
    out.create_system = info.create_system
    out.create_version = info.create_version
    out.extract_version = info.extract_version
    out.flag_bits = info.flag_bits
    out.volume = info.volume
    out.internal_attr = info.internal_attr
    out.external_attr = info.external_attr
    return out


def inject(zip_path: Path, gate: Path, source_manifest_sha: str) -> dict:
    source_zip_sha = digest(zip_path)
    gate_sha = digest(gate)
    with tempfile.NamedTemporaryFile(dir=zip_path.parent, prefix=zip_path.name + ".", suffix=".tmp", delete=False) as tmp:
        tmp_path = Path(tmp.name)
    try:
        with zipfile.ZipFile(zip_path, "r") as src:
            names = src.namelist()
            if "BOOTSTRAP_INFO" not in names:
                die(f"BOOTSTRAP_INFO missing: {zip_path}")
            before = parse_info(src.read("BOOTSTRAP_INFO"))
            if before.get("BOOTSTRAP_PROFILE") == "bridge" or before.get("BOOTSTRAP_PACKAGE_LAYER") == "bridge":
                die(f"bridge-only bootstrap rejected: {zip_path}")
            with zipfile.ZipFile(tmp_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as dst:
                for info in src.infolist():
                    if info.filename == "libexec/rafproot-fs":
                        continue
                    data = src.read(info.filename)
                    if info.filename == "BOOTSTRAP_INFO":
                        data = rewrite_info(data, gate_sha, source_manifest_sha)
                    dst.writestr(clone_info(info), data)
                if "libexec/" not in names:
                    d = zipfile.ZipInfo("libexec/")
                    d.create_system = 3
                    d.external_attr = (0o040000 | 0o700) << 16
                    dst.writestr(d, b"")
                g = zipfile.ZipInfo("libexec/rafproot-fs")
                g.create_system = 3
                g.compress_type = zipfile.ZIP_DEFLATED
                g.external_attr = (0o100000 | 0o700) << 16
                dst.writestr(g, gate.read_bytes())
        os.replace(tmp_path, zip_path)
    finally:
        tmp_path.unlink(missing_ok=True)

    with zipfile.ZipFile(zip_path, "r") as verify:
        info = parse_info(verify.read("BOOTSTRAP_INFO"))
        embedded = verify.read("libexec/rafproot-fs")
        embedded_sha = hashlib.sha256(embedded).hexdigest()
        if embedded_sha != gate_sha:
            die(f"embedded gate digest mismatch: {zip_path}")
        required = {
            "BOOTSTRAP_FREESTANDING_GATE_READY": "1",
            "BOOTSTRAP_FREESTANDING_GATE_PATH": "libexec/rafproot-fs",
            "BOOTSTRAP_FREESTANDING_GATE_SHA256": gate_sha,
            "BOOTSTRAP_FREESTANDING_SOURCE_MANIFEST_SHA256": source_manifest_sha,
            "BOOTSTRAP_FREESTANDING_BUILD_STATE": "BUILD_PROVEN",
            "BOOTSTRAP_FREESTANDING_DEVICE_STATE": "TOKEN_VAZIO",
            "BOOTSTRAP_FREESTANDING_CLAIM_ALLOWED": "false",
        }
        for key, value in required.items():
            if info.get(key) != value:
                die(f"metadata mismatch {key}: {zip_path}")
    return {
        "source_zip_sha256": source_zip_sha,
        "output_zip_sha256": digest(zip_path),
        "gate_sha256": gate_sha,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--arm-zip", type=Path, required=True)
    parser.add_argument("--aarch64-zip", type=Path, required=True)
    parser.add_argument("--source-manifest", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    for path in (args.arm_zip, args.aarch64_zip, args.source_manifest):
        if not path.is_file():
            die(f"input missing: {path}")
    source_manifest_sha = digest(args.source_manifest)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    reports = []
    for arch, zip_path in (("arm", args.arm_zip), ("aarch64", args.aarch64_zip)):
        gate = compile_gate(arch, args.output_dir / "gates")
        result = inject(zip_path, gate, source_manifest_sha)
        report = {
            "schema": "raf.freestanding-sourcebuilt-bootstrap.v1",
            "architecture": arch,
            "bootstrap_zip": str(zip_path),
            "source_manifest": str(args.source_manifest),
            "source_manifest_sha256": source_manifest_sha,
            **result,
            "wire_state": "WIRED",
            "build_state": "BUILD_PROVEN",
            "device_state": "TOKEN_VAZIO",
            "claim_allowed": False,
        }
        report_path = args.output_dir / f"freestanding-embedded-{arch}.json"
        report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        reports.append(report)
    matrix = {
        "schema": "raf.freestanding-sourcebuilt-bootstrap-matrix.v1",
        "source_manifest_sha256": source_manifest_sha,
        "reports": reports,
        "build_state": "BUILD_PROVEN",
        "device_state": "TOKEN_VAZIO",
        "claim_allowed": False,
    }
    (args.output_dir / "freestanding-embedded-matrix.json").write_text(
        json.dumps(matrix, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(matrix, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
