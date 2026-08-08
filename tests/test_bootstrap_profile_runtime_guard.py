from pathlib import Path
import unittest


class BootstrapProfileRuntimeGuardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[1]
        cls.guard = (cls.root / "app/src/main/java/com/termux/app/BootstrapBaremetalGuard.java").read_text(
            encoding="utf-8"
        )
        cls.prepare = (cls.root / "scripts/prepare_bootstrap_env.sh").read_text(encoding="utf-8")
        cls.builder = (cls.root / "scripts/build_bootstrap_profile.sh").read_text(encoding="utf-8")
        cls.embed = (cls.root / "app/src/main/cpp/termux-bootstrap-zip.S").read_text(encoding="utf-8")

    def test_runtime_guard_is_wired_after_bootstrap(self) -> None:
        self.assertIn("validateBootstrapProfileContract(prefix);", self.guard)
        self.assertIn("rafcodephi-bootstrap-profile/v1", self.guard)
        self.assertIn("BOOTSTRAP_PROFILE.json", self.guard)

    def test_real_pkg_runtime_checks(self) -> None:
        for token in (
            'verifyElf(new File(prefixDir, "bin/apt"), "apt")',
            'verifyElf(new File(prefixDir, "bin/apt-get"), "apt-get")',
            'verifyElf(new File(prefixDir, "bin/dpkg"), "dpkg")',
            "verifyLibApt(prefixDir)",
            "verifyAptRepository(prefixDir, profile)",
            "verifySourcesList",
            "RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED",
            "Enabled: no",
            "rejectBridgeMarker",
            "LEGACY_PREFIX",
        ):
            self.assertIn(token, self.guard)

    def test_claims_remain_closed(self) -> None:
        self.assertIn('profile.optBoolean("claim_allowed", true)', self.guard)
        self.assertIn('profile.optBoolean("release_allowed", true)', self.guard)
        self.assertIn('"TOKEN_VAZIO"', self.guard)

    def test_default_local_build_uses_profile_builder(self) -> None:
        self.assertIn("scripts/build_bootstrap_profile.sh", self.prepare)
        self.assertIn('PROFILE="${RAF_BOOTSTRAP_PROFILE:-bridge}"', self.builder)
        self.assertIn("termux-bootstrap-zip.S", self.builder)

    def test_integrity_hashes_exact_embedded_rewritten_archives(self) -> None:
        names = (
            "rewritten-bootstrap-aarch64.zip",
            "rewritten-bootstrap-arm.zip",
            "rewritten-bootstrap-i686.zip",
            "rewritten-bootstrap-x86_64.zip",
        )
        for name in names:
            self.assertIn(name, self.embed)
            self.assertEqual(2, self.prepare.count(f"'{name}'"))
        self.assertNotIn("': 'bootstrap-aarch64.zip'", self.prepare)
        self.assertNotIn("': 'bootstrap-arm.zip'", self.prepare)


if __name__ == "__main__":
    unittest.main()
