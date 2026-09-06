from __future__ import annotations

import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "derive_real_pkg_rebuild_scope.py"

spec = importlib.util.spec_from_file_location("derive_real_pkg_rebuild_scope", SCRIPT)
assert spec is not None and spec.loader is not None
scope = importlib.util.module_from_spec(spec)
spec.loader.exec_module(scope)


def audit(entries: list[str]) -> dict:
    return {
        "schema": scope.AUDIT_SCHEMA,
        "state": "BLOCKED",
        "reason": "UPSTREAM_BINARY_PREFIX_REBUILD_REQUIRED",
        "canonical_prefix": "/data/data/com.termux.rafacodephi/files/usr",
        "binary_risk_entry_count": len(entries),
        "binary_risk_entries": entries,
    }


def ownership(entries: dict[str, dict]) -> dict:
    return {
        "schema": scope.OWNERSHIP_SCHEMA,
        "entries": entries,
    }


def test_complete_ownership_collapses_files_into_packages_without_promoting_claims() -> None:
    report = scope.derive(
        audit(["bin/apt", "lib/libapt-pkg.so", "bin/bash"]),
        ownership({
            "bin/apt": {"package": "apt", "version": "2", "filename": "apt.deb", "deb_sha256": "a" * 64},
            "lib/libapt-pkg.so": {"package": "apt", "version": "2", "filename": "apt.deb", "deb_sha256": "a" * 64},
            "bin/bash": {"package": "bash", "version": "5", "filename": "bash.deb", "deb_sha256": "b" * 64},
        }),
    )

    assert report["state"] == "READY_TO_REBUILD"
    assert report["scope_complete"] is True
    assert report["binary_risk_entry_count"] == 3
    assert report["package_rebuild_count"] == 2
    assert report["attributed_binary_risk_entry_count"] == 3
    assert report["unattributed_binary_risk_entry_count"] == 0
    assert [item["package"] for item in report["packages"]] == ["apt", "bash"]
    assert report["claim_allowed_structural_real_pkg"] is False
    assert report["claim_allowed_device_runtime"] is False
    assert report["release_allowed"] is False


def test_missing_owner_stays_fail_closed_and_names_gap() -> None:
    report = scope.derive(
        audit(["bin/apt", "bin/unknown"]),
        ownership({
            "bin/apt": {"package": "apt", "version": "2", "filename": "apt.deb", "deb_sha256": "a" * 64},
        }),
    )

    assert report["state"] == "BLOCKED_OWNERSHIP_GAPS"
    assert report["scope_complete"] is False
    assert report["package_rebuild_count"] == 1
    assert report["unattributed_binary_risk_entry_count"] == 1
    assert report["unattributed_binary_risk_entries"] == ["bin/unknown"]
    assert report["next_required_action"] == "RESOLVE_PACKAGE_OWNERSHIP_GAPS_BEFORE_REBUILD"
    assert report["release_allowed"] is False
