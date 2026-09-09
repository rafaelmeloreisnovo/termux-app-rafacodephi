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


def test_packages_bridge_uses_layered_profiles_and_arm_development_lane():
    source = BRIDGE.read_text(encoding="utf-8")
    assert 'RAFCODEPHI_PACKAGE_PROFILE:-runtime' in source
    assert 'RAFCODEPHI_PACKAGE_ARCHES:-arm' in source
    assert "BOOTSTRAP_PACKAGES=(apt bash busybox dpkg ca-certificates coreutils termux-tools)" in source
    assert "RUNTIME_PACKAGES=(openssl curl git python procps)" in source
    assert "VECTRAS_PACKAGES=(proot)" in source
    assert "API_PACKAGES=(termux-api)" in source
    assert "bootstrap|runtime|vectras|full" in source


def test_packages_bridge_validates_recipe_identity_license_and_records_provenance():
    source = BRIDGE.read_text(encoding="utf-8")
    assert "packages/${pkg}/build.sh" in source
    assert "TERMUX_PKG_VERSION" in source
    assert "TERMUX_PKG_LICENSE" in source
    assert "required-package-recipes.sha256" in source
    assert "package-profile.txt" in source
    assert "workflow-dispatch-packages.txt" in source
    assert "checkout --detach -q FETCH_HEAD" in source
    assert "resolved packages commit" in source
    assert "packages-repo-selector.txt" in source
    assert "packages-repo-required-ref.txt" in source
