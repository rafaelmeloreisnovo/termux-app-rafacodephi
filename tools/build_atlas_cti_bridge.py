#!/usr/bin/env python3
"""Build a local CTI adapter only from the audited producer bytes; no downloads."""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import platform
import shutil
import subprocess
from atlas_contract_io import canonical, load_json, read_bytes, sha256, write_new

HERE = Path(__file__).resolve().parent
PIN = load_json(HERE / "atlas_cti_producer_pin.json")


def verify_producer(root: Path):
    receipts = {}
    for path, expected in PIN["git_blobs"].items():
        data = read_bytes(root / path, 2 << 20)
        blob = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
        if blob != expected:
            raise ValueError("producer_blob_mismatch:" + path)
        receipts[path] = {"git_blob": blob, "sha256": sha256(data)}
    return receipts


def build(root: Path, output: Path, compiler: str = "c++"):
    inputs = verify_producer(root)
    compiler_path = shutil.which(compiler)
    if not compiler_path:
        raise ValueError("compiler_unavailable")
    wrapper = HERE / "atlas_cti_bridge.cpp"
    # A fresh directory preserves prior binaries and build receipts.
    output.mkdir(mode=0o700, parents=True, exist_ok=False)
    binary = output / "atlas-cti-bridge"
    command = [compiler_path, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
               "-I" + str(root / "common"), "-I" + str(root / "vendor"),
               str(wrapper), str(root / "common/cti_memory.cpp"),
               str(root / "common/cti_privacy.cpp"), "-o", str(binary)]
    run = subprocess.run(command, capture_output=True, timeout=120, check=False)
    write_new(output / "compiler.log", run.stdout + run.stderr)
    if run.returncode:
        raise ValueError("native_build_failed")
    receipt = {"schema": "rafaelia.atlas_cti_build.v1", "producer": PIN,
               "source_hashes": inputs, "wrapper_sha256": sha256(wrapper.read_bytes()),
               "binary_sha256": sha256(binary.read_bytes()), "command": command,
               "compiler": subprocess.check_output([compiler_path, "--version"]).decode().splitlines()[0],
               "platform": platform.platform(), "machine": platform.machine(),
               "exit_code": run.returncode, "claim_allowed": False}
    write_new(output / "build_receipt.json", canonical(receipt))
    return receipt


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--producer-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--compiler", default="c++")
    args = parser.parse_args()
    try:
        result = build(args.producer_root.resolve(), args.output_dir.resolve(), args.compiler)
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(canonical({"status": "FAIL", "reason": type(exc).__name__,
                         "claim_allowed": False}).decode(), end="")
        return 2
    print(canonical({"status": "PASS", "binary_sha256": result["binary_sha256"],
                     "claim_allowed": False}).decode(), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
