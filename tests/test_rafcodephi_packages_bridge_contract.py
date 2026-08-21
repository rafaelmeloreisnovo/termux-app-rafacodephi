import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BRIDGE = ROOT / "scripts/rafcodephi_packages_bridge.sh"
PIN = ROOT / "data/contracts/termux-packages-rafcodephi-pin.v1.json"
DEPRECATED_MAGIC_PIN = "7a26629938452c6d6fd80cf3fccce8c2056aabac"


def test_packages_bridge_points_to_rafcodephi_packages_repo_through_semantic_pin():
    source = BRIDGE.read_text(encoding="utf-8")
    contract = json.loads(PIN.read_text(encoding="utf-8"))
    assert "rafaelmeloreisnovo/termux-packages.git" in source
    assert "resolve_termux_packages_pin.py" in source
    assert 'RAFCODEPHI_PACKAGES_CHANNEL:-canonical' in source
    assert DEPRECATED_MAGIC_PIN not in source
    assert contract["schema"] == "rafcodephi.termux-packages-pin/v1"
    assert contract["channels"]["canonical"]["state"] == "MERGED_BASELINE"
    assert contract["channels"]["candidate"]["claim_allowed"] is False
    assert "REQUIRED_PACKAGES=(apt bash busybox proot dpkg ca-certificates coreutils termux-tools termux-api)" in source
    assert "REQUIRED_ARCHES=(aarch64 arm)" in source


def test_packages_bridge_validates_required_package_recipes_and_records_selector():
    source = BRIDGE.read_text(encoding="utf-8")
    assert "packages/${pkg}/build.sh" in source
    assert "TERMUX_PKG_VERSION" in source
    assert "workflow-dispatch-packages.txt" in source
    assert "checkout --detach -q FETCH_HEAD" in source
    assert "resolved packages commit" in source
    assert "packages-repo-selector.txt" in source
    assert "packages-repo-required-ref.txt" in source
