from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REWRITTEN_ZIPS = [
    "rewritten-bootstrap-aarch64.zip",
    "rewritten-bootstrap-arm.zip",
    "rewritten-bootstrap-i686.zip",
    "rewritten-bootstrap-x86_64.zip",
]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_rewritten_bootstraps_are_declared_and_preflight_materialized() -> None:
    build_gradle = read("app/build.gradle")
    asm = read("app/src/main/cpp/termux-bootstrap-zip.S")
    build_script = read("scripts/build_rafaelia_bootstraps.sh")
    preflight = read("scripts/prepare_bootstrap_env.sh")

    for zip_name in REWRITTEN_ZIPS:
        assert zip_name in build_gradle
        assert zip_name in build_script
        assert f'.incbin "{zip_name}"' in asm

    assert "verifyBootstrapZipsPresent" in build_gradle
    assert "externalNativeBuild" in build_gradle
    assert "validateSideBySideContract" in build_gradle
    assert "Bootstrap source:" in preflight
    assert "bash scripts/build_bootstrap_profile.sh" in preflight
    assert "Verifying bootstrap contract" in preflight


def test_gradle_version_helpers_remain_safe() -> None:
    build_gradle = read("app/build.gradle")
    assert "def validateVersionName(String candidateVersionName)" in build_gradle
    assert "def hasReleaseTaskRequested()" in build_gradle
    assert "def effectiveVersionName = appVersionName ?: \"0.118.0\"" in build_gradle
    assert "validateVersionName(effectiveVersionName)" in build_gradle
    assert "versionName effectiveVersionName" in build_gradle
    assert "validateVersionName(versionName)" not in build_gradle


def test_runtime_paths_are_derived_from_android_assigned_files_dir() -> None:
    source = read("termux-shared/src/main/java/com/termux/shared/termux/TermuxRuntimePaths.java")
    for token in [
        "context.getFilesDir()",
        "prefixDirPath()",
        "stagingPrefixDirPath()",
        "RELOCATED_ANDROID_ASSIGNED",
        "realPkgRelocationClaimAllowed()",
        "return false;",
    ]:
        assert token in source


def test_wizard_exposes_real_bootstrap_zip_document_route_through_compatibility_entry() -> None:
    entry = read("app/src/main/java/com/termux/app/activities/Android15WizardActivity.java")
    wizard = read("app/src/main/java/com/termux/app/activities/BetaBootstrapWizardActivity.java")
    readiness = read("app/src/main/java/com/termux/app/BootstrapReadinessGate.java")
    source = read("app/src/main/java/com/termux/app/BootstrapWizardSource.java")

    assert "extends BetaBootstrapWizardActivity" in entry

    for token in [
        "Intent.ACTION_OPEN_DOCUMENT",
        "Select real bootstrap.zip",
        "BootstrapWizardSource.accept(this, uri)",
        "TermuxRuntimePaths.filesDirPath()",
        "TermuxRuntimePaths.prefixDirPath()",
        "compiled PREFIX",
        "isBlockingStep",
        "Install / Repair Real Bootstrap + APT",
        "BootstrapReadinessGate.evaluate(this).isPass()",
        "BootstrapReadinessGate.evaluateStartup(this).isPass()",
    ]:
        assert token in wizard

    for token in [
        'SCHEMA = "rafcodephi.bootstrap-readiness/v1"',
        'PROFILE_SCHEMA = "rafcodephi-bootstrap-profile/v1"',
        'PROFILE_FILE = "BOOTSTRAP_PROFILE.json"',
        'PROFILE_READ_LIMIT = 64 * 1024',
        '"sh"',
        '"pkg"',
        '"apkmanager"',
        '"shellbash"',
        '"busybox-safe"',
        '"proot-safe"',
        "STARTUP_REQUIRED_EXECUTABLES",
        "OPTIONAL_COMPATIBILITY_WRAPPERS",
        "FIRST_BOOT_SHELL_AND_PKG_READY",
        "FULL_PACKAGE_RUNTIME",
        "TermuxRuntimePaths.storageHomeDir()",
        'context.getPackageName().equals(profile.optString("package_name", ""))',
        'prefix.getAbsolutePath().equals(profile.optString("prefix", ""))',
        'expectedBootstrapArch().equals(profile.optString("arch", ""))',
        '!profile.optBoolean("claim_allowed", true)',
        '!profile.optBoolean("release_allowed", true)',
        'TOKEN_VAZIO.equals(profile.optString("device_validation", ""))',
        'profile.optJSONArray("required_entries")',
        'canonicalTarget.startsWith(canonicalPrefix)',
        "claim_allowed_release=false",
    ]:
        assert token in readiness

    assert ".mkdirs()" not in readiness
    assert "Os.chmod" not in readiness
    assert ".delete()" not in readiness
    assert "setupBootstrapIfNeeded" not in readiness

    for token in [
        "HOST_ACCEPTED_CANONICAL_BOOTSTRAP",
        "expectedHashForCurrentAbi()",
        "blake3Hex",
        "BOOTSTRAP_PROFILE.json",
        "SYMLINKS.txt",
        "RELOCATED_RUNTIME_BLOCKED_FOR_NON_RELOCATABLE_BOOTSTRAP",
        "getFD().sync()",
        "renameTo(target)",
    ]:
        assert token in source


def test_installer_uses_runtime_prefix_and_keeps_real_pkg_claim_closed() -> None:
    installer = read("app/src/main/java/com/termux/app/TermuxInstaller.java")
    for token in [
        "TermuxRuntimePaths.init(activity)",
        "verifyRuntimeFilesDirectoryWritable(activity)",
        "BootstrapWizardSource.loadAcceptedBytes(context)",
        "verifyBootstrapZipIntegrity(zipBytes)",
        "verifyRelocationContract(zipBytes)",
        "materializeRuntimeBootstrapProfile(staging, prefix.getAbsolutePath())",
        'profile.put("source_prefix", sourcePrefix)',
        'profile.put("prefix", runtimePrefix)',
        'profile.put("real_pkg_relocation_claim_allowed", false)',
        "staging.renameTo(prefix)",
        "verifyRuntimeBinary(new File(staging, \"bin/sh\"), \"sh\", true)",
        "verifyRuntimeBinary(new File(staging, \"bin/pkg\"), \"pkg\", true)",
        "verifyRuntimeBinary(new File(staging, \"bin/busybox\"), \"busybox\", false)",
        "verifyRuntimeBinary(new File(staging, \"bin/proot\"), \"proot\", false)",
        "RELOCATED_RUNTIME_BLOCKED_FOR_REAL_OR_UNPROVEN_PACKAGE_LAYER",
        "rollbackFailedBootstrapInstall",
    ]:
        assert token in installer


def test_profile_materializer_places_manifest_inside_zip() -> None:
    profile = read("tools/raf_bootstrap_profile.py")
    assert 'PROFILE_FILE = "BOOTSTRAP_PROFILE.json"' in profile
    assert "out.writestr(zi, profile_data)" in profile
    assert '"claim_allowed": False' in profile
    assert '"device_validation": "TOKEN_VAZIO"' in profile


def test_application_environment_and_terminal_share_runtime_path_graph() -> None:
    app = read("app/src/main/java/com/termux/app/TermuxApplication.java")
    env = read("termux-shared/src/main/java/com/termux/shared/termux/shell/command/environment/TermuxShellEnvironment.java")
    session = read("termux-shared/src/main/java/com/termux/shared/termux/shell/command/runner/terminal/TermuxSession.java")

    for token in [
        "TermuxRuntimePaths.init(context)",
        "runtimeFilesDirectoryAccessible()",
        "new File(TermuxRuntimePaths.binDirPath(), \"sh\")",
        "new File(TermuxRuntimePaths.binDirPath(), \"pkg\")",
        "BootstrapBaremetalGuard.validateAfterBootstrap(TermuxRuntimePaths.prefixDirPath())",
    ]:
        assert token in app

    for token in [
        "ENV_HOME, TermuxRuntimePaths.homeDirPath()",
        "ENV_PREFIX, TermuxRuntimePaths.prefixDirPath()",
        "ENV_TMPDIR, TermuxRuntimePaths.tmpDirPath()",
        "ENV_PATH, TermuxRuntimePaths.binDirPath()",
        "TERMUX_REAL_PKG_RELOCATION_CLAIM_ALLOWED",
    ]:
        assert token in env

    assert "Do not hard-gate terminal startup on bash" in session
    assert "LOGIN_SHELL_BINARIES" in session
    assert "/system/bin/sh" in session


def test_validator_is_claim_bounded_and_checks_read_only_profile_gate() -> None:
    validator = read("tools/validate_bootstrap_package_install_contract.py")
    for token in [
        "bootstrap_package_install_contract=PASS",
        "claim_boundary=structural_only_physical_filesystem_and_first_shell_still_required",
        "wizard_bootstrap_document_source=fail_closed_b3_abi_profile_bound",
        "wizard_compatibility_entry=preserved",
        "wizard_readiness_gate=shared_fail_closed",
        "wizard_readiness_profile_contract=read_only_fail_closed",
        "runtime_filesystem=context_getFilesDir_resolved",
        "installed_profile=source_prefix_preserved_runtime_prefix_materialized",
        "relocated_bridge_runtime=structurally_supported_claim_still_closed",
        "real_pkg_relocation=BLOCKED",
        'for token in (".mkdirs()", "Os.chmod", ".delete()", "setupBootstrapIfNeeded")',
        '"bin/busybox"), "busybox", false',
        '"bin/proot"), "proot", false',
        '"Select real bootstrap.zip"',
        '"Install / Repair Real Bootstrap + APT"',
        '"BootstrapReadinessGate.evaluateStartup(this).isPass()"',
    ]:
        assert token in validator
