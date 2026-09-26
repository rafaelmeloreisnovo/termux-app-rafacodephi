from pathlib import Path
import importlib.util
import unittest


class WorkflowControlPlaneContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[1]
        cls.control = (cls.root / ".github/workflows/00_START_HERE.yml").read_text(encoding="utf-8")
        cls.reusable = (cls.root / ".github/workflows/_reusable-arm32-compat.yml").read_text(encoding="utf-8")
        cls.arm32 = (cls.root / ".github/workflows/compatibility-arm32.yml").read_text(encoding="utf-8")
        cls.arm32_ndk29 = (cls.root / ".github/workflows/compatibility-arm32-ndk29.yml").read_text(encoding="utf-8")
        cls.vectra = (cls.root / ".github/workflows/vectra-grade-benchmarks.yml").read_text(encoding="utf-8")
        cls.top42 = (cls.root / ".github/workflows/top42_bench.yml").read_text(encoding="utf-8")

        scanner_path = cls.root / "scripts/ci/workflow_control_plane.py"
        spec = importlib.util.spec_from_file_location("workflow_control_plane", scanner_path)
        assert spec and spec.loader
        cls.scanner = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.scanner)

    def test_start_here_exposes_only_valid_human_routes(self) -> None:
        for token in (
            "01_DIAGNOSTICO",
            "02_ARM32_CANONICO",
            "03_ARM32_NDK29",
            "04_BOOTSTRAP_CANONICO",
            "05_BOOTSTRAP_NDK29",
            "06_EVIDENCIAS",
            "07_E2E",
            "08_VECTRA_V3",
            "09_ENTERPRISE",
        ):
            self.assertIn(token, self.control)
        self.assertIn("INVALID_ROUTE", self.control)
        self.assertIn("resolver rota válida", self.control)
        self.assertNotIn("inputs.ndk_lane", self.control)

    def test_final_gate_requires_only_route_required_jobs(self) -> None:
        self.assertIn("required_by_route", self.control)
        self.assertIn("failed_required_jobs", self.control)
        self.assertIn(
            "NOT_REQUESTED unless the route lists it as required",
            self.control,
        )
        self.assertIn("URGENCY_DOES_NOT_BYPASS_GATES", self.control)
        self.assertIn("Emitir receipt antes de decidir PASS/FAIL", self.control)

    def test_arm32_wrappers_delegate_to_one_reusable_pillar(self) -> None:
        target = "./.github/workflows/_reusable-arm32-compat.yml"
        self.assertIn(target, self.arm32)
        self.assertIn(target, self.arm32_ndk29)
        self.assertIn("ndk_lane: canonical", self.arm32)
        self.assertIn("ndk_lane: ndk29", self.arm32_ndk29)

    def test_arm32_pillar_preserves_device_claim_boundary(self) -> None:
        self.assertIn("abi=armeabi-v7a", self.reusable)
        self.assertIn("device_runtime_proof=TOKEN_VAZIO", self.reusable)
        self.assertIn("claim_allowed=false", self.reusable)
        self.assertIn("sha256sum", self.reusable)

    def test_scanner_recognizes_inline_and_block_trigger_forms(self) -> None:
        inline = "name: x\non: [push, pull_request, workflow_dispatch]\njobs: {}\n"
        block = "name: x\non:\n  push:\n  workflow_dispatch:\njobs: {}\n"
        flow_map = "name: x\non: {push: null, workflow_dispatch: null}\njobs: {}\n"
        self.assertEqual(
            set(self.scanner.detect_triggers(inline)),
            {"push", "pull_request", "workflow_dispatch"},
        )
        self.assertEqual(
            set(self.scanner.detect_triggers(block)),
            {"push", "workflow_dispatch"},
        )
        self.assertEqual(
            set(self.scanner.detect_triggers(flow_map)),
            {"push", "workflow_dispatch"},
        )
        self.assertIn("on: [push, pull_request, workflow_dispatch]", self.top42)

    def test_scanner_track_contract_includes_artifact(self) -> None:
        self.assertIn("artifact", self.scanner.ALLOWED_TRACKS)
        self.assertEqual(self.scanner.TOKEN_VAZIO, "TOKEN_VAZIO")

    def test_vectra_is_callable_but_physical_pa_remains_separate(self) -> None:
        self.assertIn("workflow_call:", self.vectra)
        self.assertIn("claim_allowed", self.vectra)
        self.assertIn("pa_physical_execution", self.vectra)
        self.assertIn("TOKEN_VAZIO", self.vectra)
        self.assertIn("./.github/workflows/vectra-grade-benchmarks.yml", self.control)

    def test_start_here_never_calls_device_smoke_as_physical_proof(self) -> None:
        self.assertNotIn("device-runtime-smoke.yml", self.control)
        self.assertIn("CI_PASS_DOES_NOT_EQUAL_PHYSICAL_ANDROID_PROOF", self.control)


if __name__ == "__main__":
    unittest.main()
