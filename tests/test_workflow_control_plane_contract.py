from pathlib import Path
import unittest


class WorkflowControlPlaneContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[1]
        cls.control = (cls.root / ".github/workflows/00-rafaelia-control-plane.yml").read_text(encoding="utf-8")
        cls.reusable = (cls.root / ".github/workflows/_reusable-arm32-compat.yml").read_text(encoding="utf-8")
        cls.arm32 = (cls.root / ".github/workflows/compatibility-arm32.yml").read_text(encoding="utf-8")
        cls.arm32_ndk29 = (cls.root / ".github/workflows/compatibility-arm32-ndk29.yml").read_text(encoding="utf-8")
        cls.scanner = (cls.root / "scripts/ci/workflow_control_plane.py").read_text(encoding="utf-8")

    def test_single_human_entrypoint_has_clear_missions(self) -> None:
        for token in ("diagnostico", "arm32-v7", "bootstrap-arm32", "completo-seguro"):
            self.assertIn(token, self.control)
        self.assertIn("Ω 6/6 Resultado simples", self.control)

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

    def test_scanner_discovers_all_workflow_yaml_files(self) -> None:
        self.assertIn('ROOT.glob("*.yml")', self.scanner)
        self.assertIn('ROOT.glob("*.yaml")', self.scanner)
        self.assertIn("TOKEN_VAZIO", self.scanner)
        self.assertIn("sha256", self.scanner)

    def test_control_plane_never_calls_device_smoke_as_physical_proof(self) -> None:
        self.assertNotIn("device-runtime-smoke.yml", self.control)
        self.assertIn("CI PASS não substitui recibo físico Android", self.control)


if __name__ == "__main__":
    unittest.main()
