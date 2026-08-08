from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_real_pkg_profile_is_target_aware_and_keeps_debt_explicit() -> None:
    text = read("scripts/build_bootstrap_profile.sh")
    assert "profile_for_arch" in text
    assert "all:aarch64|all:arm|aarch64:aarch64|arm:arm" in text
    assert '"schema": "rafcodephi-bootstrap-profile-matrix/v2"' in text
    assert '"embedded_profile_by_arch"' in text
    assert '"TV_BOOTSTRAP_DEVICE_INSTALL"' in text
    assert '"TV_PKG_UPDATE"' in text
    assert '"TV_PKG_INSTALL_SMOKE"' in text
    assert '"claim_allowed": False' in text
    assert '"release_allowed": False' in text


def test_contract_verifies_the_exact_embedded_bootstrap_bytes() -> None:
    verify = read("scripts/verify_bootstrap_contract.sh")
    prepare = read("scripts/prepare_bootstrap_env.sh")
    assert "EMBEDDED_BOOTSTRAPS=(rewritten-bootstrap-aarch64.zip" in verify
    assert "tools/raf_bootstrap_profile.py validate" in verify
    assert "RAF_BOOTSTRAP_REQUIRE_PROFILE" in verify
    assert 'PROFILE_REQUIREMENT="required"' in prepare
    assert 'RAF_BOOTSTRAP_REQUIRE_PROFILE="$PROFILE_REQUIREMENT"' in prepare
    assert "exact embedded rewritten archives" in prepare


def test_candidate_receipt_never_promotes_unobserved_runtime_or_release() -> None:
    text = read("scripts/write_bootstrap_candidate_receipt.py")
    assert '"device_validation": "TOKEN_VAZIO"' in text
    assert '"runtime_pkg_update": "NOT_MEASURED"' in text
    assert '"runtime_pkg_install_smoke": "NOT_MEASURED"' in text
    assert '"claim_allowed_device_runtime": False' in text
    assert '"claim_allowed_real_pkg_runtime": False' in text
    assert '"release_allowed": False' in text


def test_candidate_workflow_builds_one_materialization_and_hash_bound_receipt() -> None:
    text = read(".github/workflows/bootstrap-real-pkg-arm32-candidate.yml")
    build_section = text.split("- name: Build ARM32-capable debug APK set", 1)[1].split(
        "- name: Resolve ARM32 APK", 1
    )[0]
    assert "prepare_bootstrap_env.sh" not in build_section
    assert "RAF_BOOTSTRAP_PROFILE: real-pkg" in text
    assert "RAFCODEPHI_REAL_PKG_ARCH: arm" in text
    assert "write_bootstrap_candidate_receipt.py" in text
    assert "rewritten-bootstrap-arm.zip" in text


def test_action_reference_policy_is_current_and_tracks_previously_unknown_actions() -> None:
    text = read("scripts/audit_github_actions_refs.py")
    assert 'POLICY_VERIFIED_ON = "2026-08-08"' in text
    assert '"actions/checkout": {"current": 7' in text
    assert '"actions/setup-python": {"current": 7' in text
    assert '"android-actions/setup-android": {"current": 4' in text
    assert '"softprops/action-gh-release": {"current": 3' in text
