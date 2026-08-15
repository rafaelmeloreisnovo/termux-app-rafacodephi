from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BRIDGE = ROOT / "scripts/rafcodephi_packages_bridge.sh"


def test_packages_bridge_points_to_rafcodephi_packages_repo():
    source = BRIDGE.read_text(encoding="utf-8")
    assert "rafaelmeloreisnovo/termux-packages.git" in source
    assert 'PACKAGES_REF="${RAFCODEPHI_PACKAGES_REF:-7a26629938452c6d6fd80cf3fccce8c2056aabac}"' in source
    assert "REQUIRED_PACKAGES=(apt bash busybox proot dpkg ca-certificates coreutils termux-tools termux-api)" in source
    assert "REQUIRED_ARCHES=(aarch64 arm)" in source


def test_packages_bridge_validates_required_package_recipes():
    source = BRIDGE.read_text(encoding="utf-8")
    assert "packages/${pkg}/build.sh" in source
    assert "TERMUX_PKG_VERSION" in source
    assert "workflow-dispatch-packages.txt" in source
    assert "checkout --detach -q FETCH_HEAD" in source
    assert "resolved packages commit" in source
