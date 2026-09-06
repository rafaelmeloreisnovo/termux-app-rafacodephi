from datetime import date
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_real_pkg_profile_is_target_aware_and_keeps_debt_explicit() -> None:
    text = read("scripts/build_bootstrap_profile.sh")
    assert "profile_for_arch" in text
    assert "all:aarch64|all:arm|aarch64:aarch64|arm:arm" in text
    assert 'SKIP_BUILD="${RAF_BOOTSTRAP_SKIP_BUILD:-false}"' in text
    assert "BOOTSTRAP_PROFILE_REUSE_AUDITED_BYTES=true" in text
    assert '"reused_audited_bytes"' in text
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
    assert "Verifying bootstrap contract" in prepare
    assert "exact embedded rewritten archives" in prepare


def test_candidate_receipt_never_promotes_unobserved_runtime_or_release() -> None:
    text = read("scripts/write_bootstrap_candidate_receipt.py")
    assert '"device_validation": "TOKEN_VAZIO"' in text
    assert '"runtime_pkg_update": "NOT_MEASURED"' in text
    assert '"runtime_pkg_install_smoke": "NOT_MEASURED"' in text
    assert '"claim_allowed_device_runtime": False' in text
    assert '"claim_allowed_real_pkg_runtime": False' in text
    assert '"release_allowed": False' in text


def test_prefix_audit_promotes_binary_prefix_risk_to_explicit_p0_debt() -> None:
    text = read("scripts/validate_real_arm_bootstrap_core.py")
    assert 'SCHEMA = "rafcodephi-real-pkg-prefix-audit/v1"' in text
    assert 'BINARY_RISK = "LEGACY_PREFIX_BINARY_RISK"' in text
    assert '"BLOCKED", "UPSTREAM_BINARY_PREFIX_REBUILD_REQUIRED"' in text
    assert '"id": "TV_REAL_PKG_PREFIX_REBUILD"' in text
    assert '"priority": "P0"' in text
    assert '"next_required_action"' in text
    assert '"REBUILD_AFFECTED_PACKAGES_FOR_RAFCODEPHI_PREFIX"' in text
    assert "no binary replacement was performed" in text
    assert '"claim_allowed_device_runtime": False' in text
    assert '"claim_allowed_pkg_runtime": False' in text
    assert '"release_allowed": False' in text


def test_promotion_gate_receipt_binds_blocker_without_promoting_runtime() -> None:
    text = read("scripts/write_bootstrap_promotion_gate_receipt.py")
    assert 'SCHEMA = "rafcodephi-bootstrap-promotion-gate-receipt/v1"' in text
    assert '"prefix_audit_sha256": sha256(args.audit)' in text
    assert '"raw_bootstrap_sha256": sha256(args.bootstrap)' in text
    assert '"claim_allowed_apk_candidate": False' in text
    assert '"claim_allowed_device_runtime": False' in text
    assert '"claim_allowed_pkg_runtime": False' in text
    assert '"release_allowed": False' in text
    assert '"device_validation": "TOKEN_VAZIO"' in text


def test_real_pkg_workflow_is_a_fail_closed_same_observation_promotion_gate() -> None:
    text = read(".github/workflows/bootstrap-real-pkg-arm32-candidate.yml")
    assert "# ci_track: internal" in text
    assert "name: Bootstrap Real-Pkg ARM32 Promotion Gate" in text
    assert "Build raw ARM32 real-pkg payload without unsafe promotion" in text
    assert "validate_real_arm_bootstrap_core.py" in text
    assert "--json dist/bootstrap-real-pkg-arm32/prefix-audit.json" in text
    assert 'echo "promotion_state=BLOCKED" >> "$GITHUB_OUTPUT"' in text
    assert "write_bootstrap_promotion_gate_receipt.py" in text
    assert "if: steps.prefix_gate.outputs.promotion_state == 'PASS'" in text
    assert "Materialize exact real-pkg profile only after prefix gate PASS" in text
    assert "Build ARM32 APK candidate only after promotion gate PASS" in text
    assert "Hash-bind APK candidate only after promotion gate PASS" in text
    assert "RAF_BOOTSTRAP_SKIP_BUILD: 'true'" in text
    assert "d['reused_audited_bytes'] is True" in text
    assert "SAME_OBSERVATION_PROFILE_PROMOTION_PASS" in text
    assert "write_bootstrap_candidate_receipt.py" in text
    assert "A BLOCKED prefix gate is a valid safety observation" in text

    raw_pos = text.index("Build raw ARM32 real-pkg payload without unsafe promotion")
    audit_pos = text.index("Audit upstream binary prefix compatibility")
    materialize_pos = text.index("Materialize exact real-pkg profile only after prefix gate PASS")
    apk_pos = text.index("Build ARM32 APK candidate only after promotion gate PASS")
    assert raw_pos < audit_pos < materialize_pos < apk_pos


def test_action_reference_policy_is_current_and_tracks_previously_unknown_actions() -> None:
    text = read("scripts/audit_github_actions_refs.py")

    # Freshness is a property of the policy date, not one frozen historical
    # literal. The floor advances only when the policy itself is re-verified.
    match = re.search(r'^POLICY_VERIFIED_ON = "(\d{4}-\d{2}-\d{2})"$', text, re.MULTILINE)
    assert match is not None
    verified_on = date.fromisoformat(match.group(1))
    assert verified_on >= date(2026, 9, 6)

    assert '"actions/checkout": {"current": 7' in text
    assert '"actions/setup-java": {"current": 6' in text
    assert '"actions/setup-python": {"current": 7' in text
    assert '"android-actions/setup-android": {"current": 4' in text
    assert '"softprops/action-gh-release": {"current": 3' in text
