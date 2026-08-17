#!/usr/bin/env python3
"""Deterministic regression tests for bootstrap-pair transaction/rollback/failover."""
from __future__ import annotations

import json
import tempfile
from pathlib import Path

import bootstrap_pair_transaction as tx


def fake_validator(pair: dict[str, Path], manifest: Path) -> dict[str, dict[str, object]]:
    assert manifest.is_file()
    out: dict[str, dict[str, object]] = {}
    for arch in ("arm", "aarch64"):
        path = pair[arch]
        if not path.is_file() or not path.read_bytes().startswith((b"PRIMARY-", b"LKG-", b"OLD-")):
            raise RuntimeError(f"invalid test payload: {arch}")
        out[arch] = {"arch": arch, "sha256": tx.sha256(path), "device_runtime_proof": "TOKEN_VAZIO"}
    return out


def write(path: Path, payload: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)


def make_fixture(root: Path):
    manifest = root / "primary.manifest"
    lkg_manifest = root / "lkg.manifest"
    manifest.write_text("test=primary\n", encoding="utf-8")
    lkg_manifest.write_text("test=lkg\n", encoding="utf-8")
    primary = {"arm": root / "primary-arm.zip", "aarch64": root / "primary-aarch64.zip"}
    lkg = {"arm": root / "lkg-arm.zip", "aarch64": root / "lkg-aarch64.zip"}
    dest = {"arm": root / "dest-arm.zip", "aarch64": root / "dest-aarch64.zip"}
    write(primary["arm"], b"PRIMARY-ARM-v2")
    write(primary["aarch64"], b"PRIMARY-AARCH64-v2")
    write(lkg["arm"], b"LKG-ARM-v1")
    write(lkg["aarch64"], b"LKG-AARCH64-v1")
    write(dest["arm"], b"OLD-ARM")
    write(dest["aarch64"], b"OLD-AARCH64")
    return manifest, lkg_manifest, primary, lkg, dest


def test_primary_commit() -> None:
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        manifest, _, primary, _, dest = make_fixture(root)
        state = tx.transactional_install(
            primary, manifest, dest, root / "receipts", root / "state.json", validator=fake_validator
        )
        assert state == "PRIMARY_COMMITTED"
        assert dest["arm"].read_bytes() == primary["arm"].read_bytes()
        assert dest["aarch64"].read_bytes() == primary["aarch64"].read_bytes()
        assert json.loads((root / "state.json").read_text())["state"] == "PRIMARY_COMMITTED"


def test_partial_primary_rolls_back_pair() -> None:
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        manifest, _, primary, _, dest = make_fixture(root)
        before = {arch: path.read_bytes() for arch, path in dest.items()}
        try:
            tx.transactional_install(
                primary,
                manifest,
                dest,
                root / "receipts",
                root / "state.json",
                inject_failure_after_arm=True,
                validator=fake_validator,
            )
        except RuntimeError as exc:
            assert "no validated LKG" in str(exc)
        else:
            raise AssertionError("injected partial primary failure must block")
        assert dest["arm"].read_bytes() == before["arm"]
        assert dest["aarch64"].read_bytes() == before["aarch64"]
        doc = json.loads((root / "state.json").read_text())
        assert doc["state"] == "ROLLED_BACK_BLOCKED"
        assert doc["rollback_performed"] is True
        assert doc["lkg_state"] == "TOKEN_VAZIO"


def test_partial_primary_fails_over_to_validated_lkg_pair() -> None:
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        manifest, lkg_manifest, primary, lkg, dest = make_fixture(root)
        state = tx.transactional_install(
            primary,
            manifest,
            dest,
            root / "receipts",
            root / "state.json",
            lkg=lkg,
            lkg_manifest=lkg_manifest,
            allow_lkg_failover=True,
            inject_failure_after_arm=True,
            validator=fake_validator,
        )
        assert state == "FAILOVER_LKG"
        assert dest["arm"].read_bytes() == lkg["arm"].read_bytes()
        assert dest["aarch64"].read_bytes() == lkg["aarch64"].read_bytes()
        doc = json.loads((root / "state.json").read_text())
        assert doc["state"] == "FAILOVER_LKG"
        assert doc["rollback_performed"] is True
        assert doc["failover_used"] is True


def main() -> int:
    tests = (
        test_primary_commit,
        test_partial_primary_rolls_back_pair,
        test_partial_primary_fails_over_to_validated_lkg_pair,
    )
    for test in tests:
        test()
        print(f"PASS {test.__name__}")
    print("BOOTSTRAP_PAIR_TRANSACTION_SELFTEST=PASS tests=3")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
