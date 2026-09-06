#!/usr/bin/env python3
"""Inject the static RAFCODEPHI exec gate into source-built real-pkg bootstraps.

The input pair is treated as immutable parent evidence by custody hash. Callers
should pass copies. A child manifest is emitted with updated ZIP hashes plus the
parent-manifest/gate hashes, so the existing strict source-built importer can
validate the exact gate-bearing artifacts that enter the APK.
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
PACKAGE = "com.termux.rafacodephi"
PREFIX = f"/data/data/{PACKAGE}/files/usr"
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


def parse_manifest(path: Path) -> tuple[list[str], dict[str, str]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    fields: dict[str, str] = {}
    for raw in lines:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            die(f"manifest line is not key=value: {line!r}")
        key, value = line.split("=", 1)
        if key in fields:
            die(f"duplicate manifest key: {key}")
        fields[key] = value
    required = {
        "schema": "rafcodephi.real-bootstrap-sourcebuild/v1",
        "package_name": PACKAGE,
        "prefix": PREFIX,
        "bridge_allowed": "false",
        "legacy_prefix_allowed": "false",
        "claim_allowed_device_runtime": "false",
        "device_runtime_proof": "TOKEN_VAZIO",
    }
    for key, expected in required.items():
        if fields.get(key) != expected:
            die(f"parent manifest contract mismatch {key}: {fields.get(key)!r}")
    return lines, fields


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


def rewrite_info(raw: bytes, gate_sha: str, parent_manifest_sha: str) -> bytes:
    lines = [line for line in raw.decode("utf-8", "strict").splitlines()
             if not line.startswith("BOOTSTRAP_FREESTANDING_")]
    lines.extend([
        "BOOTSTRAP_FREESTANDING_GATE_READY=1",
        "BOOTSTRAP_FREESTANDING_GATE_PATH=libexec/rafproot-fs",
        f"BOOTSTRAP_FREESTANDING_GATE_SHA256={gate_sha}",
        f"BOOTSTRAP_FREESTANDING_PARENT_MANIFEST_SHA256={parent_manifest_sha}",
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


def inject(zip_path: Path, gate: Path, parent_manifest_sha: str, arch: str) -> dict:
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
            expected_before = {
                "TERMUX_PACKAGE_NAME": PACKAGE,
                "TERMUX_ARCH": arch,
                "RAFCODEPHI_BOOTSTRAP_PROFILE": "real-pkg",
                "RAFCODEPHI_PACKAGE_LAYER": "real-pkg",
                "RAFCODEPHI_DEVICE_VALIDATION": "TOKEN_VAZIO",
                "RAFCODEPHI_CLAIM_ALLOWED": "0",
            }
            for key, expected in expected_before.items():
                if before.get(key) != expected:
                    die(f"input is not strict real-pkg {arch}: {key}={before.get(key)!r}")
            with zipfile.ZipFile(tmp_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as dst:
                for info in src.infolist():
                    if info.filename == "libexec/rafproot-fs":
                        continue
                    data = src.read(info.filename)
                    if info.filename == "BOOTSTRAP_INFO":
                        data = rewrite_info(data, gate_sha, parent_manifest_sha)
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
        embedded_sha = hashlib.sha256(verify.read("libexec/rafproot-fs")).hexdigest()
        if embedded_sha != gate_sha:
            die(f"embedded gate digest mismatch: {zip_path}")
        required = {
            "BOOTSTRAP_FREESTANDING_GATE_READY": "1",
            "BOOTSTRAP_FREESTANDING_GATE_PATH": "libexec/rafproot-fs",
            "BOOTSTRAP_FREESTANDING_GATE_SHA256": gate_sha,
            "BOOTSTRAP_FREESTANDING_PARENT_MANIFEST_SHA256": parent_manifest_sha,
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


def write_child_manifest(parent_lines: list[str], parent_sha: str, reports: dict[str, dict], out: Path) -> None:
    replacements = {
        "sha256_arm": reports["arm"]["output_zip_sha256"],
        "sha256_aarch64": reports["aarch64"]["output_zip_sha256"],
    }
    rendered: list[str] = []
    seen: set[str] = set()
    for raw in parent_lines:
        stripped = raw.strip()
        if "=" in stripped and not stripped.startswith("#"):
            key = stripped.split("=", 1)[0]
            if key in replacements:
                rendered.append(f"{key}={replacements[key]}")
                seen.add(key)
                continue
            if key.startswith("freestanding_"):
                continue
        rendered.append(raw)
    missing = set(replacements) - seen
    if missing:
        die(f"parent manifest missing hash declarations: {sorted(missing)}")
    rendered.extend([
        f"freestanding_parent_manifest_sha256={parent_sha}",
        "freestanding_gate_path=libexec/rafproot-fs",
        f"freestanding_gate_sha256_arm={reports['arm']['gate_sha256']}",
        f"freestanding_gate_sha256_aarch64={reports['aarch64']['gate_sha256']}",
        "freestanding_build_state=BUILD_PROVEN",
        "freestanding_device_state=TOKEN_VAZIO",
        "freestanding_claim_allowed=false",
    ])
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(rendered).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--arm-zip", type=Path, required=True)
    parser.add_argument("--aarch64-zip", type=Path, required=True)
    parser.add_argument("--source-manifest", type=Path, required=True)
    parser.add_argument("--child-manifest", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    for path in (args.arm_zip, args.aarch64_zip, args.source_manifest):
        if not path.is_file():
            die(f"input missing: {path}")
    parent_lines, _ = parse_manifest(args.source_manifest)
    parent_sha = digest(args.source_manifest)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    by_arch: dict[str, dict] = {}
    reports: list[dict] = []
    for arch, zip_path in (("arm", args.arm_zip), ("aarch64", args.aarch64_zip)):
        gate = compile_gate(arch, args.output_dir / "gates")
        result = inject(zip_path, gate, parent_sha, arch)
        by_arch[arch] = result
        report = {
            "schema": "raf.freestanding-sourcebuilt-bootstrap.v1",
            "architecture": arch,
            "bootstrap_zip": str(zip_path),
            "parent_manifest": str(args.source_manifest),
            "parent_manifest_sha256": parent_sha,
            **result,
            "wire_state": "WIRED",
            "build_state": "BUILD_PROVEN",
            "device_state": "TOKEN_VAZIO",
            "claim_allowed": False,
        }
        (args.output_dir / f"freestanding-embedded-{arch}.json").write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        reports.append(report)
    write_child_manifest(parent_lines, parent_sha, by_arch, args.child_manifest)
    child_sha = digest(args.child_manifest)
    matrix = {
        "schema": "raf.freestanding-sourcebuilt-bootstrap-matrix.v2",
        "parent_manifest_sha256": parent_sha,
        "child_manifest": str(args.child_manifest),
        "child_manifest_sha256": child_sha,
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
