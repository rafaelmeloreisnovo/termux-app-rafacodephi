#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIN = ROOT / "data/contracts/termux-packages-rafcodephi-pin.v1.json"
RESOLVER = ROOT / "scripts/resolve_termux_packages_pin.py"
DEPRECATED_MAGIC_PIN = "7a26629938452c6d6fd80cf3fccce8c2056aabac"

ACTIVE_ROUTES = (
    ROOT / "scripts/validate_rafcodephi_packages_source.sh",
    ROOT / "scripts/rafcodephi_packages_bridge.sh",
    ROOT / ".github/workflows/apk_matrix_build.yml",
    ROOT / ".github/workflows/rafcodephi-v1-termux-packages.yml",
    ROOT / ".github/workflows/rafcodephi-control-center-real-runtime.yml",
    ROOT / ".github/workflows/beta-build-libllvm18-unblock.yml",
)


def resolve(selector: str) -> str:
    return subprocess.check_output(
        [sys.executable, str(RESOLVER), selector],
        cwd=ROOT,
        text=True,
    ).strip()


def main() -> int:
    doc = json.loads(PIN.read_text(encoding="utf-8"))
    assert doc["schema"] == "rafcodephi.termux-packages-pin/v1"
    assert doc["repository"] == "https://github.com/rafaelmeloreisnovo/termux-packages.git"
    assert doc["semantic_role"] == "source-built-packages-bootstrap-and-apt"
    assert doc["package_name"] == "com.termux.rafacodephi"
    assert doc["prefix"] == "/data/data/com.termux.rafacodephi/files/usr"
    assert doc["required_abis"] == ["armeabi-v7a", "arm64-v8a"]

    canonical = doc["channels"]["canonical"]
    candidate = doc["channels"]["candidate"]
    assert canonical["state"] == "MERGED_BASELINE"
    assert canonical["commit"] == "d65cd6c2afa9d16196ba0375aaff36833d530b41"
    assert candidate["state"] == "BLOCKED_NO_ACTIVE_UNMERGED_CANDIDATE"
    assert candidate["promotion_target"] == "canonical"
    assert canonical["commit"] != candidate["commit"]
    # Post-merge provenance: the old PR174 source pin is historical, not an active producer.
    merged_history = {
        entry["merged_pr"] for entry in doc["pin_history"]
        if entry.get("event") == "SUPERSEDED_AFTER_MERGE"
    }
    assert candidate["commit"] == "TOKEN_VAZIO"
    assert 174 in merged_history
    assert canonical["source_lineage_prs"] == [174, 179, 183, 184]
    assert canonical["build_gate"] == "BLOCKED_MAIN_EXACT_SHA_APK_AND_SIGNED_APT_NOT_PROVEN"
    assert 183 in merged_history
    assert "pull_request" not in candidate
    assert any(entry.get("value", {}).get("commit") == canonical["previous_commit"]
               for entry in doc["pin_history"])
    for channel in (canonical, candidate):
        assert channel["claim_allowed"] is False
        assert channel["physical_android"] == "TOKEN_VAZIO"

    assert resolve("canonical") == canonical["commit"]
    disabled = subprocess.run(
        [sys.executable, str(RESOLVER), "candidate"],
        cwd=ROOT, text=True, capture_output=True, check=False,
    )
    assert disabled.returncode != 0
    assert "BLOCKED_NO_ACTIVE_UNMERGED_CANDIDATE" in disabled.stderr
    assert resolve(canonical["commit"]) == canonical["commit"]

    for path in ACTIVE_ROUTES:
        assert path.is_file(), f"missing active route: {path.relative_to(ROOT)}"
        text = path.read_text(encoding="utf-8")
        assert DEPRECATED_MAGIC_PIN not in text, (
            f"deprecated magic termux-packages pin leaked into active route: {path.relative_to(ROOT)}"
        )

    source_validator = (ROOT / "scripts/validate_rafcodephi_packages_source.sh").read_text()
    bridge = (ROOT / "scripts/rafcodephi_packages_bridge.sh").read_text()
    apk_matrix = (ROOT / ".github/workflows/apk_matrix_build.yml").read_text()
    v1 = (ROOT / ".github/workflows/rafcodephi-v1-termux-packages.yml").read_text()
    control = (ROOT / ".github/workflows/rafcodephi-control-center-real-runtime.yml").read_text()
    beta = (ROOT / ".github/workflows/beta-build-libllvm18-unblock.yml").read_text()

    assert "resolve_termux_packages_pin.py" in source_validator
    assert "resolve_termux_packages_pin.py" in bridge
    assert "TERMUX_PACKAGES_RAF_REF: canonical" in apk_matrix
    assert "default: canonical" in v1 and "resolve_termux_packages_pin.py" in v1
    assert "default: canonical" in control and "resolve_termux_packages_pin.py" in control
    assert "TERMUX_PACKAGES_RAF_CHANNEL: canonical" in beta
    assert "resolve_termux_packages_pin.py" in beta
    assert "bcb1dfec1ecc72cd3acee80b5bf3706cead4849c" not in beta
    assert "TERMUX_PACKAGES_RAF_REQUIRE_PINNED: 'true'" in beta

    print(
        "PASS: active Termux routes resolve semantic pin channels; historical/candidate magic SHAs are excluded from executable surfaces; candidate disabled fail-closed; canonical now follows merged producer"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
