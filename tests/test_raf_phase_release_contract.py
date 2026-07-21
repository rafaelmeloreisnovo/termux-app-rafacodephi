from __future__ import annotations

import copy
import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "phase_release_contract",
    ROOT / "scripts" / "validate_raf_phase_release_contract.py",
)
assert SPEC and SPEC.loader
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


def fixture():
    return module.load(ROOT)


class PhaseReleaseContractTests(unittest.TestCase):

    def assert_invalid(self, files, phrase):
        errors = module.validate_snapshot(files)
        self.assertTrue(errors)
        self.assertIn(phrase, "\n".join(errors))

    def test_repository_contract_passes(self):
        self.assertEqual([], module.validate_snapshot(fixture()))

    def test_window_cannot_shrink_below_eight_cycles(self):
        files = fixture()
        files["header"] = files["header"].replace(
            "RAF_PHASE_GATE_WINDOW_CYCLES 8u",
            "RAF_PHASE_GATE_WINDOW_CYCLES 7u",
        )
        self.assert_invalid(files, "WINDOW_CYCLES 8u")

    def test_four_phase_order_is_mandatory(self):
        files = fixture()
        files["header"] = files["header"].replace(
            "RAF_PHASE_GATE_PHASES_PER_CYCLE 4u",
            "RAF_PHASE_GATE_PHASES_PER_CYCLE 3u",
        )
        self.assert_invalid(files, "PHASES_PER_CYCLE 4u")

    def test_frequency_step_is_exactly_point_one_hz(self):
        files = fixture()
        files["header"] = files["header"].replace(
            "RAF_PHASE_GATE_STEP_MHZ 100u",
            "RAF_PHASE_GATE_STEP_MHZ 1000u",
        )
        self.assert_invalid(files, "STEP_MHZ 100u")

    def test_release_mask_cannot_be_removed(self):
        files = fixture()
        files["source"] = files["source"].replace(
            "gate->completed_mask != RAF_PHASE_GATE_WINDOW_MASK",
            "0u",
        )
        self.assert_invalid(files, "completed_mask")

    def test_frequency_must_follow_release(self):
        files = fixture()
        source = files["source"]
        source = source.replace(
            "gate->released_digest = gate->staged_digest;",
            "raf_phase_gate_advance_frequency(gate);\n      gate->released_digest = gate->staged_digest;",
            1,
        )
        files["source"] = source
        self.assert_invalid(files, "frequency must advance only after atomic release")

    def test_heap_allocation_is_rejected(self):
        files = fixture()
        files["source"] += "\nvoid *p = malloc(8);\n"
        self.assert_invalid(files, "malloc(")

    def test_physical_sleep_is_rejected(self):
        files = fixture()
        files["source"] += "\nusleep(100000);\n"
        self.assert_invalid(files, "usleep(")

    def test_floating_point_is_rejected(self):
        files = fixture()
        files["source"] += "\ndouble phase = 0.0;\n"
        self.assert_invalid(files, "double ")

    def test_missing_android_build_integration_is_rejected(self):
        files = fixture()
        files["android_mk"] = files["android_mk"].replace(
            "  raf_phase_release_gate.c \\\n",
            "",
        )
        self.assert_invalid(files, "Android.mk")

    def test_missing_native_test_execution_is_rejected(self):
        files = fixture()
        files["compile_gate"] = files["compile_gate"].replace(
            "test_phase_release_gate",
            "removed_phase_release_test",
        )
        self.assert_invalid(files, "test_phase_release_gate")

    def test_operational_evidence_cannot_be_promoted(self):
        files = fixture()
        files["contract"] = files["contract"].replace(
            '"physical_100ms_cadence": "TOKEN_VAZIO"',
            '"physical_100ms_cadence": "PASS"',
        )
        self.assert_invalid(files, "physical_100ms_cadence")

    def test_claims_cannot_be_enabled(self):
        files = fixture()
        files["contract"] = files["contract"].replace(
            '"claim_allowed": false',
            '"claim_allowed": true',
            1,
        )
        self.assert_invalid(files, "claims and release disabled")

    def test_missing_phase_order_fault_is_rejected(self):
        files = fixture()
        files["source"] = files["source"].replace(
            "RAF_PHASE_GATE_ERR_PHASE_ORDER",
            "RAF_PHASE_GATE_OK",
        )
        self.assert_invalid(files, "ERR_PHASE_ORDER")

    def test_missing_evidence_fault_is_rejected(self):
        files = fixture()
        files["source"] = files["source"].replace(
            "RAF_PHASE_GATE_ERR_EVIDENCE",
            "RAF_PHASE_GATE_OK",
        )
        self.assert_invalid(files, "ERR_EVIDENCE")


if __name__ == "__main__":
    unittest.main()
