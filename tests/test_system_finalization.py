from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "system_finalization", ROOT / "tools" / "validate_system_finalization.py"
)
assert SPEC and SPEC.loader
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


class SystemFinalizationTests(unittest.TestCase):
    def test_safe_core_is_closed_without_promoting_release(self) -> None:
        report = module.evaluate(ROOT, "safe-core")
        self.assertTrue(report["profile_closed"], report["failed_required_checks"])
        self.assertEqual("SAFE_CORE_IMPLEMENTATION_CLOSED", report["state"])
        self.assertTrue(report["claim_allowed_scope"])
        self.assertFalse(report["release_allowed"])

    def test_functional_distribution_remains_blocked(self) -> None:
        report = module.evaluate(ROOT, "functional-distribution")
        self.assertFalse(report["profile_closed"])
        self.assertFalse(report["release_allowed"])
        self.assertIn("dual_arm_device_evidence", report["failed_required_checks"])
        self.assertIn("prefix_safe_real_package_stack", report["failed_required_checks"])
        self.assertIn("production_release_signing", report["failed_required_checks"])

    def test_full_platform_does_not_conflate_fixed_fixtures_with_complete_systems(self) -> None:
        report = module.evaluate(ROOT, "full-platform")
        self.assertFalse(report["profile_closed"])
        self.assertIn("browser_tls", report["failed_required_checks"])
        self.assertIn("complete_apkc_compilers", report["failed_required_checks"])
        self.assertIn("complete_vcpu_vm", report["failed_required_checks"])

    def test_loader_is_only_accepted_in_quarantined_state(self) -> None:
        check = module.check_loader_quarantine(ROOT)
        self.assertTrue(check["ok"], check)
        self.assertIn(check["state"], {"STUB_SAFE_BLOCKED", "FUNCTIONAL_SECURITY_GATED"})
        self.assertIn("release remains blocked", check["detail"])

    def test_device_instrumentation_is_not_device_evidence(self) -> None:
        instrumentation = module.check_zero_instrumentation(ROOT)
        physical = module.check_dual_arm_evidence(ROOT)
        self.assertTrue(instrumentation["ok"], instrumentation)
        self.assertFalse(physical["ok"])
        self.assertEqual("TOKEN_VAZIO", physical["state"])

    def test_runtime_lock_is_not_silently_promoted(self) -> None:
        check = module.check_runtime_lock(ROOT)
        self.assertFalse(check["ok"])
        self.assertEqual("STALE_OR_INCOMPLETE", check["state"])

    def test_unknown_profile_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            module.evaluate(ROOT, "unknown")


if __name__ == "__main__":
    unittest.main()
