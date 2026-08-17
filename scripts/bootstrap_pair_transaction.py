#!/usr/bin/env python3
"""Transactional install for the RAFCODEPHI ARM32+AArch64 bootstrap pair.

Invariant: downstream consumers may observe only a fully validated primary pair or
an independently validated LKG pair. A partial primary install is rolled back.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import signal
import tempfile
from pathlib import Path
from typing import Callable

ROOT = Path(__file__).resolve().parents[1]
IMPORTER_PATH = ROOT / "scripts" / "import_rafcodephi_real_bootstrap.py"


def _load_importer():
    spec = importlib.util.spec_from_file_location("raf_bootstrap_importer", IMPORTER_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load importer: {IMPORTER_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


IMPORTER = _load_importer()


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def atomic_json(path: Path, payload: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=path.parent)
    tmp = Path(name)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            json.dump(payload, stream, indent=2, sort_keys=True)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(tmp, path)
        dir_fd = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(dir_fd)
        finally:
            os.close(dir_fd)
    finally:
        if tmp.exists():
            tmp.unlink()


def snapshot_pair(destinations: dict[str, Path], snapshot_dir: Path) -> dict[str, dict[str, object]]:
    snapshot_dir.mkdir(parents=True, exist_ok=True)
    state: dict[str, dict[str, object]] = {}
    for arch, dest in destinations.items():
        existed = dest.is_file()
        entry: dict[str, object] = {"existed": existed, "path": str(dest)}
        if existed:
            snap = snapshot_dir / f"{arch}.zip"
            IMPORTER.atomic_copy(dest, snap)
            entry.update({"snapshot": str(snap), "sha256": sha256(snap), "bytes": snap.stat().st_size})
        state[arch] = entry
    return state


def restore_snapshot(destinations: dict[str, Path], snapshot: dict[str, dict[str, object]]) -> None:
    errors: list[str] = []
    for arch, dest in destinations.items():
        entry = snapshot[arch]
        try:
            if entry["existed"]:
                snap = Path(str(entry["snapshot"]))
                IMPORTER.atomic_copy(snap, dest)
                if sha256(dest) != entry["sha256"]:
                    raise RuntimeError(f"restored SHA256 mismatch for {arch}")
            elif dest.exists():
                dest.unlink()
                dir_fd = os.open(dest.parent, os.O_RDONLY)
                try:
                    os.fsync(dir_fd)
                finally:
                    os.close(dir_fd)
        except Exception as exc:
            errors.append(f"{arch}: {exc}")
    if errors:
        raise RuntimeError("rollback failed: " + "; ".join(errors))


def validate_pair(pair: dict[str, Path], manifest: Path) -> dict[str, dict[str, object]]:
    receipts: dict[str, dict[str, object]] = {}
    for arch in ("arm", "aarch64"):
        receipts[arch] = IMPORTER.validate(pair[arch], manifest, arch)
    return receipts


def write_import_receipts(
    receipts: dict[str, dict[str, object]], destinations: dict[str, Path], receipt_dir: Path, source: str
) -> None:
    for arch in ("arm", "aarch64"):
        payload = dict(receipts[arch])
        payload["embedded_rewritten_path"] = str(destinations[arch])
        payload["transaction_source"] = source
        atomic_json(receipt_dir / f"rafcodephi-real-bootstrap-import-{arch}.json", payload)


def install_pair(pair: dict[str, Path], destinations: dict[str, Path], *, inject_failure_after_arm: bool = False) -> None:
    IMPORTER.atomic_copy(pair["arm"], destinations["arm"])
    if inject_failure_after_arm:
        raise RuntimeError("injected failure after ARM32 replacement")
    IMPORTER.atomic_copy(pair["aarch64"], destinations["aarch64"])


def transactional_install(
    primary: dict[str, Path],
    primary_manifest: Path,
    destinations: dict[str, Path],
    receipt_dir: Path,
    report_path: Path,
    *,
    lkg: dict[str, Path] | None = None,
    lkg_manifest: Path | None = None,
    allow_lkg_failover: bool = False,
    inject_failure_after_arm: bool = False,
    validator: Callable[[dict[str, Path], Path], dict[str, dict[str, object]]] = validate_pair,
) -> str:
    report: dict[str, object] = {
        "schema": "rafcodephi.bootstrap-pair-transaction/v1",
        "state": "PREVALIDATING",
        "primary": {arch: {"path": str(path), "sha256": sha256(path)} for arch, path in primary.items()},
        "lkg_configured": bool(lkg and lkg_manifest),
        "lkg_failover_allowed": allow_lkg_failover,
        "failover_used": False,
        "rollback_performed": False,
        "claim_allowed_device_runtime": False,
        "device_runtime_proof": "TOKEN_VAZIO",
    }
    atomic_json(report_path, report)

    primary_receipts = validator(primary, primary_manifest)
    lkg_receipts: dict[str, dict[str, object]] | None = None
    if lkg is not None or lkg_manifest is not None:
        if lkg is None or lkg_manifest is None:
            raise RuntimeError("LKG requires arm+aarch64+manifest as one complete set")
        lkg_receipts = validator(lkg, lkg_manifest)
        report["lkg"] = {arch: {"path": str(path), "sha256": sha256(path)} for arch, path in lkg.items()}
        report["lkg_prevalidated"] = True

    with tempfile.TemporaryDirectory(prefix="rafcodephi-bootstrap-pair-") as td:
        snapshot = snapshot_pair(destinations, Path(td))
        report["before"] = snapshot
        report["state"] = "PRIMARY_COMMITTING"
        atomic_json(report_path, report)
        try:
            install_pair(primary, destinations, inject_failure_after_arm=inject_failure_after_arm)
            post = validator(destinations, primary_manifest)
            for arch in ("arm", "aarch64"):
                if sha256(destinations[arch]) != sha256(primary[arch]):
                    raise RuntimeError(f"post-commit source/destination SHA256 mismatch for {arch}")
            write_import_receipts(post, destinations, receipt_dir, "PRIMARY")
            report.update(
                state="PRIMARY_COMMITTED",
                after={arch: {"sha256": sha256(path), "bytes": path.stat().st_size} for arch, path in destinations.items()},
            )
            atomic_json(report_path, report)
            return "PRIMARY_COMMITTED"
        except BaseException as primary_error:
            report["primary_error"] = f"{type(primary_error).__name__}: {primary_error}"
            try:
                restore_snapshot(destinations, snapshot)
                report["rollback_performed"] = True
                report["state"] = "PRIMARY_ROLLED_BACK"
                atomic_json(report_path, report)
            except BaseException as rollback_error:
                report["state"] = "ROLLBACK_FAILED"
                report["rollback_error"] = f"{type(rollback_error).__name__}: {rollback_error}"
                atomic_json(report_path, report)
                raise RuntimeError("bootstrap pair rollback failed; workspace is BLOCKED") from rollback_error

            if allow_lkg_failover and lkg is not None and lkg_manifest is not None and lkg_receipts is not None:
                try:
                    install_pair(lkg, destinations)
                    post_lkg = validator(destinations, lkg_manifest)
                    for arch in ("arm", "aarch64"):
                        if sha256(destinations[arch]) != sha256(lkg[arch]):
                            raise RuntimeError(f"LKG source/destination SHA256 mismatch for {arch}")
                    write_import_receipts(post_lkg, destinations, receipt_dir, "LKG_FAILOVER")
                    report.update(
                        state="FAILOVER_LKG",
                        failover_used=True,
                        after={arch: {"sha256": sha256(path), "bytes": path.stat().st_size} for arch, path in destinations.items()},
                    )
                    atomic_json(report_path, report)
                    return "FAILOVER_LKG"
                except BaseException as lkg_error:
                    report["state"] = "FAILOVER_FAILED_BLOCKED"
                    report["lkg_error"] = f"{type(lkg_error).__name__}: {lkg_error}"
                    try:
                        restore_snapshot(destinations, snapshot)
                    except BaseException as final_rollback_error:
                        report["state"] = "FINAL_ROLLBACK_FAILED"
                        report["final_rollback_error"] = f"{type(final_rollback_error).__name__}: {final_rollback_error}"
                    atomic_json(report_path, report)
                    raise RuntimeError("primary and independently validated LKG failover both failed; BLOCKED") from lkg_error

            report["state"] = "ROLLED_BACK_BLOCKED"
            report["lkg_state"] = "TOKEN_VAZIO" if lkg is None else "DISABLED"
            atomic_json(report_path, report)
            raise RuntimeError("primary pair failed and no validated LKG failover was available; BLOCKED") from primary_error


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--arm", required=True, type=Path)
    parser.add_argument("--aarch64", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--dest-arm", type=Path, default=Path("app/src/main/cpp/rewritten-bootstrap-arm.zip"))
    parser.add_argument("--dest-aarch64", type=Path, default=Path("app/src/main/cpp/rewritten-bootstrap-aarch64.zip"))
    parser.add_argument("--receipt-dir", type=Path, default=Path("build/reports"))
    parser.add_argument("--state-report", type=Path, default=Path("build/reports/bootstrap-pair-transaction.json"))
    parser.add_argument("--lkg-arm", type=Path)
    parser.add_argument("--lkg-aarch64", type=Path)
    parser.add_argument("--lkg-manifest", type=Path)
    parser.add_argument("--allow-lkg-failover", action="store_true")
    parser.add_argument("--inject-failure-after-arm", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()

    primary = {"arm": args.arm, "aarch64": args.aarch64}
    for path in (*primary.values(), args.manifest):
        if not path.is_file():
            raise SystemExit(f"required bootstrap transaction input unavailable: {path}")

    lkg_args = (args.lkg_arm, args.lkg_aarch64, args.lkg_manifest)
    lkg = None
    if any(x is not None for x in lkg_args):
        if not all(x is not None for x in lkg_args):
            raise SystemExit("LKG failover requires --lkg-arm, --lkg-aarch64 and --lkg-manifest together")
        assert args.lkg_arm is not None and args.lkg_aarch64 is not None and args.lkg_manifest is not None
        for path in (args.lkg_arm, args.lkg_aarch64, args.lkg_manifest):
            if not path.is_file():
                raise SystemExit(f"LKG input unavailable: {path}")
        lkg = {"arm": args.lkg_arm, "aarch64": args.lkg_aarch64}

    def _term_handler(signum, frame):
        raise RuntimeError(f"termination signal {signum} received during bootstrap transaction")

    signal.signal(signal.SIGTERM, _term_handler)
    signal.signal(signal.SIGINT, _term_handler)

    state = transactional_install(
        primary,
        args.manifest,
        {"arm": args.dest_arm, "aarch64": args.dest_aarch64},
        args.receipt_dir,
        args.state_report,
        lkg=lkg,
        lkg_manifest=args.lkg_manifest,
        allow_lkg_failover=args.allow_lkg_failover,
        inject_failure_after_arm=args.inject_failure_after_arm,
    )
    print(f"RAFCODEPHI_BOOTSTRAP_PAIR_TRANSACTION={state}")
    print("claim_allowed_device_runtime=false")
    print("device_runtime_proof=TOKEN_VAZIO")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
