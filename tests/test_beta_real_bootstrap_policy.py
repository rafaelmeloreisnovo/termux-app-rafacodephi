#!/usr/bin/env python3
from __future__ import annotations

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class BetaRealBootstrapPolicyTests(unittest.TestCase):
    def read(self, relative: str) -> str:
        return (ROOT / relative).read_text(encoding="utf-8")

    def test_beta_workflow_requires_source_built_real_pair(self):
        workflow = self.read(".github/workflows/beta-build.yml")
        self.assertIn("RAF_BOOTSTRAP_SOURCE: source-built-real", workflow)
        self.assertIn("--architectures arm,aarch64", workflow)
        self.assertIn("bridge_allowed=false", workflow)
        self.assertIn("legacy_prefix_allowed=false", workflow)
        self.assertIn("rafcodephi-bootstrap-arm.zip", workflow)
        self.assertIn("rafcodephi-bootstrap-aarch64.zip", workflow)
        self.assertNotIn("RAF_BOOTSTRAP_SOURCE: local\n          RELEASE_TRACK: internal", workflow)

    def test_beta_workflow_has_fail_closed_real_profile_gate(self):
        workflow = self.read(".github/workflows/beta-build.yml")
        self.assertIn("beta requires real-pkg profile", workflow)
        self.assertIn("profile.get('profile') != 'real-pkg'", workflow)
        self.assertIn("profile.get('package_layer') != 'real-pkg'", workflow)
        self.assertIn("classifications.get(name) != 'ELF'", workflow)
        self.assertIn("Beta build is BLOCKED", workflow)

    def test_apk_matrix_accepts_source_built_real_without_downgrade(self):
        script = self.read("scripts/build_apk_matrix.sh")
        self.assertIn("local|upstream|source-built-real", script)
        self.assertIn('export RAF_BOOTSTRAP_SOURCE="${BOOTSTRAP_SOURCE_REQUESTED}"', script)
        self.assertNotIn("RAF_BOOTSTRAP_SOURCE must be local or upstream,", script)

    def test_readiness_requires_real_pkg_runtime(self):
        gate = self.read("app/src/main/java/com/termux/app/BootstrapReadinessGate.java")
        self.assertIn('REQUIRED_BETA_PROFILE = "real-pkg"', gate)
        for binary in ("apt", "apt-get", "dpkg", "bash", "busybox", "proot"):
            self.assertIn(f'"{binary}"', gate)
        self.assertIn("isElf(target)", gate)
        self.assertIn("dpkg_status_missing_or_empty", gate)
        self.assertIn("apt_source_definition_missing", gate)

    def test_symlinks_manifest_is_source_only_not_runtime_file(self):
        gate = self.read("app/src/main/java/com/termux/app/BootstrapReadinessGate.java")
        installer = self.read("app/src/main/java/com/termux/app/TermuxInstaller.java")
        profile_tool = self.read("tools/raf_bootstrap_profile.py")
        self.assertIn('SOURCE_ONLY_SYMLINK_MANIFEST = "SYMLINKS.txt"', gate)
        self.assertIn("declared_install_manifest_consumed_by_TermuxInstaller_not_runtime_file", gate)
        self.assertIn('if ("SYMLINKS.txt".equals(name))', installer)
        self.assertIn("continue;", installer)
        self.assertIn("SYMLINKS_FILE", profile_tool)

    def test_beta_wizard_uses_governed_real_repair_not_weak_setup_skip(self):
        wizard = self.read("app/src/main/java/com/termux/app/activities/BetaBootstrapWizardActivity.java")
        self.assertIn("BetaRealBootstrapRepair.repair(this, this::updateWizardStep)", wizard)
        self.assertIn("Install / Repair Real Bootstrap", wizard)
        method = wizard.split("private void installBootstrapFilesystem()", 1)[1].split("private void openAuditActivity()", 1)[0]
        self.assertNotIn("TermuxInstaller.setupBootstrapIfNeeded", method)

    def test_repair_preserves_old_prefix_until_real_candidate_is_validated(self):
        repair = self.read("app/src/main/java/com/termux/app/BetaRealBootstrapRepair.java")
        validate_pos = repair.index("verifyRealPackageArchive(activity, candidate)")
        backup_pos = repair.index("prefix.renameTo(backup)")
        install_pos = repair.index("TermuxInstaller.setupBootstrapIfNeeded")
        self.assertLess(validate_pos, backup_pos)
        self.assertLess(backup_pos, install_pos)
        self.assertIn('BACKUP_NAME = ".usr-before-real-pkg-beta"', repair)
        self.assertIn("home_preserved=", repair)
        self.assertIn("restoreBackupAfterRejectedInstall", repair)

    def test_repair_archive_rejects_bridge_profile(self):
        repair = self.read("app/src/main/java/com/termux/app/BetaRealBootstrapRepair.java")
        self.assertIn('!"real-pkg".equals(profile.optString("profile", ""))', repair)
        self.assertIn('!"real-pkg".equals(profile.optString("package_layer", ""))', repair)
        self.assertIn("BETA_REQUIRES_REAL_PKG_PROFILE", repair)
        self.assertIn("BOOTSTRAP_PROFILE_CLAIM_BOUNDARY_OPEN", repair)

    def test_claim_boundary_remains_closed(self):
        workflow = self.read(".github/workflows/beta-build.yml")
        gate = self.read("app/src/main/java/com/termux/app/BootstrapReadinessGate.java")
        self.assertIn("CLAIM_ALLOWED: 'false'", workflow)
        self.assertIn("PHYSICAL_ANDROID: 'TOKEN_VAZIO'", workflow)
        self.assertIn('out.append("claim_allowed_release=false', gate)


if __name__ == "__main__":
    unittest.main(verbosity=2)
