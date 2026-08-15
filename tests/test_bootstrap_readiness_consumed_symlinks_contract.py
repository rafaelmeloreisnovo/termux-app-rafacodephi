from pathlib import Path
import unittest


class BootstrapReadinessConsumedSymlinksContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        root = Path(__file__).resolve().parents[1]
        cls.readiness = (
            root / "app/src/main/java/com/termux/app/BootstrapReadinessGate.java"
        ).read_text(encoding="utf-8")
        cls.installer = (
            root / "app/src/main/java/com/termux/app/TermuxInstaller.java"
        ).read_text(encoding="utf-8")

    def test_installer_consumes_symlink_manifest_before_runtime_profile(self) -> None:
        self.assertIn('if ("SYMLINKS.txt".equals(name))', self.installer)
        self.assertIn("symlinks.add(Pair.create(target, link.getAbsolutePath()))", self.installer)
        self.assertIn("Os.symlink(link.first, link.second)", self.installer)
        self.assertIn("materializeRuntimeBootstrapProfile(staging, prefix.getAbsolutePath())", self.installer)

    def test_runtime_materialized_profile_treats_only_symlink_manifest_as_source_only(self) -> None:
        self.assertIn('SOURCE_ONLY_SYMLINKS_FILE = "SYMLINKS.txt"', self.readiness)
        self.assertIn('profile.optBoolean("runtime_materialized", false)', self.readiness)
        self.assertIn(
            "if (runtimeMaterialized && SOURCE_ONLY_SYMLINKS_FILE.equals(relative))",
            self.readiness,
        )

    def test_missing_other_required_entries_remain_fail_closed(self) -> None:
        self.assertIn('violations.add("missing_required_entry_" + i + "_" + relative)', self.readiness)
        self.assertIn('violations.add("unsafe_required_entry_" + i)', self.readiness)
        self.assertIn('violations.add("required_entry_escape_" + i)', self.readiness)


if __name__ == "__main__":
    unittest.main()
