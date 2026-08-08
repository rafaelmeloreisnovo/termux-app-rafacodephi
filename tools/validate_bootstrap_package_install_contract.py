#!/usr/bin/env python3
"""Validate RAFCODEPHI bootstrap filesystem/install contract.

Structural only. This validator intentionally distinguishes:
- canonical compile/build prefix metadata;
- Android-assigned runtime app-private filesDir;
- wizard-selected bootstrap.zip provenance;
- relocated bridge runtime from unproven real-pkg relocation.

It does not claim device runtime. Physical filesystem + first-shell receipts are
required before the relocated runtime can be promoted from TOKEN_VAZIO.
"""
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD_GRADLE = ROOT / "app/build.gradle"
ASM = ROOT / "app/src/main/cpp/termux-bootstrap-zip.S"
BUILDER = ROOT / "scripts/bootstrap_zip_builder.c"
BUILD_SCRIPT = ROOT / "scripts/build_rafaelia_bootstraps.sh"
PREPARE = ROOT / "scripts/prepare_bootstrap_env.sh"
INSTALLER = ROOT / "app/src/main/java/com/termux/app/TermuxInstaller.java"
APPLICATION = ROOT / "app/src/main/java/com/termux/app/TermuxApplication.java"
WIZARD = ROOT / "app/src/main/java/com/termux/app/activities/Android15WizardActivity.java"
WIZARD_SOURCE = ROOT / "app/src/main/java/com/termux/app/BootstrapWizardSource.java"
RUNTIME_PATHS = ROOT / "termux-shared/src/main/java/com/termux/shared/termux/TermuxRuntimePaths.java"
SHELL_ENV = ROOT / "termux-shared/src/main/java/com/termux/shared/termux/shell/command/environment/TermuxShellEnvironment.java"
SESSION = ROOT / "termux-shared/src/main/java/com/termux/shared/termux/shell/command/runner/terminal/TermuxSession.java"
PROFILE_TOOL = ROOT / "tools/raf_bootstrap_profile.py"

REWRITTEN_ZIPS = (
    "rewritten-bootstrap-aarch64.zip",
    "rewritten-bootstrap-arm.zip",
    "rewritten-bootstrap-i686.zip",
    "rewritten-bootstrap-x86_64.zip",
)

COMMAND_WRAPPER_APPLETS = (
    "cat", "ls", "clear", "grep", "sed", "awk", "head", "tail", "wc",
    "mkdir", "rm", "cp", "mv", "ln", "chmod", "pwd", "env", "which",
    "find", "tar", "gzip", "gunzip", "zcat", "stat", "strings", "file", "whoami",
)


def read(path: Path, errors: list[str]) -> str:
    if not path.exists():
        errors.append(f"missing file: {path.relative_to(ROOT)}")
        return ""
    return path.read_text(encoding="utf-8")


def require(text: str, token: str, label: str, errors: list[str]) -> None:
    if token not in text:
        errors.append(f"{label}: missing token: {token}")


def validate() -> list[str]:
    errors: list[str] = []
    build_gradle = read(BUILD_GRADLE, errors)
    asm = read(ASM, errors)
    builder = read(BUILDER, errors)
    build_script = read(BUILD_SCRIPT, errors)
    prepare = read(PREPARE, errors)
    installer = read(INSTALLER, errors)
    application = read(APPLICATION, errors)
    wizard = read(WIZARD, errors)
    wizard_source = read(WIZARD_SOURCE, errors)
    runtime_paths = read(RUNTIME_PATHS, errors)
    shell_env = read(SHELL_ENV, errors)
    session = read(SESSION, errors)
    profile_tool = read(PROFILE_TOOL, errors)

    # Build/source provenance: rewritten archives are native-incbin inputs and
    # CI/local preflight is the materializer. Do not require obsolete Gradle task names.
    for zip_name in REWRITTEN_ZIPS:
        require(asm, f'.incbin "{zip_name}"', "native incbin", errors)
        require(build_gradle, zip_name, "gradle bootstrap input declaration", errors)
        require(build_script, zip_name, "bootstrap generation script", errors)
    for token in ("verifyBootstrapZipsPresent", "externalNativeBuild", "validateSideBySideContract"):
        require(build_gradle, token, "gradle bootstrap contract", errors)
    for token in ("prepare_bootstrap_env", "Bootstrap source:", "BOOTSTRAP_PROFILE_BUILD_PASS"):
        require(prepare, token, "bootstrap preflight materializer", errors)

    # Package payload and wrappers remain explicit.
    for token in (
        "bin/sh", "bin/pkg", "bin/busybox", "bin/proot", "bin/apkmanager",
        "bin/shellbash", "bin/busybox-safe", "bin/proot-safe", "SYMLINKS.txt",
        "BOOTSTRAP_PACKAGE_INSTALLABLE=1", "BOOTSTRAP_COMMAND_WRAPPERS_READY=1",
        "command_wrapper_names", "wrapper_paths",
    ):
        require(builder + "\n" + build_script, token, "bootstrap runtime payload", errors)
    for applet in COMMAND_WRAPPER_APPLETS:
        require(build_script + "\n" + builder, applet, "busybox wrapper applet", errors)

    # Runtime path source of truth must derive from Context.getFilesDir().
    for token in (
        "context.getFilesDir()", "prefixDirPath()", "stagingPrefixDirPath()",
        "RELOCATED_ANDROID_ASSIGNED", "realPkgRelocationClaimAllowed()", "return false;",
    ):
        require(runtime_paths, token, "runtime path resolver", errors)

    # Installer must probe the real assigned path, stage atomically, and never
    # assume that real apt/dpkg ELFs became relocatable.
    for token in (
        "TermuxRuntimePaths.init(activity)", "verifyRuntimeFilesDirectoryWritable(activity)",
        "BootstrapWizardSource.loadAcceptedBytes(context)", "verifyBootstrapZipIntegrity(zipBytes)",
        "verifyRelocationContract(zipBytes)", "staging.renameTo(prefix)",
        "verifyRuntimeBinary(new File(staging, \"bin/sh\"), \"sh\", true)",
        "verifyRuntimeBinary(new File(staging, \"bin/pkg\"), \"pkg\", true)",
        "verifyRuntimeBinary(new File(staging, \"bin/busybox\"), \"busybox\", true)",
        "verifyRuntimeBinary(new File(staging, \"bin/proot\"), \"proot\", true)",
        "BootstrapBaremetalGuard.validateAfterBootstrap(prefix.getAbsolutePath())",
        "TermuxShellEnvironment.writeEnvironmentToFile(activity)",
        "rollbackFailedBootstrapInstall", "RELOCATED_RUNTIME_BLOCKED_FOR_REAL_OR_UNPROVEN_PACKAGE_LAYER",
    ):
        require(installer, token, "runtime installer", errors)

    # Wizard document picker is a real source route, not decorative UI.
    for token in (
        "Intent.ACTION_OPEN_DOCUMENT", "Select bootstrap.zip", "BootstrapWizardSource.accept(this, uri)",
        "TermuxRuntimePaths.filesDirPath()", "TermuxRuntimePaths.prefixDirPath()",
        "Canonical compiled PREFIX", "isBlockingStep", "Install Filesystem",
    ):
        require(wizard, token, "wizard filesystem route", errors)

    # Selected file must be accepted with provenance and fail-closed on relocated layouts.
    for token in (
        "HOST_ACCEPTED_CANONICAL_BOOTSTRAP", "expectedHashForCurrentAbi()", "blake3Hex",
        "BOOTSTRAP_PROFILE.json", "SYMLINKS.txt", "bin/sh", "bin/pkg", "bin/busybox", "bin/proot",
        "RELOCATED_RUNTIME_BLOCKED_FOR_NON_RELOCATABLE_BOOTSTRAP", "claim_allowed", "false",
        "getFD().sync()", "renameTo(target)",
    ):
        require(wizard_source, token, "wizard bootstrap source", errors)

    # Profile materializer must actually place the manifest in the ZIP.
    for token in ("PROFILE_FILE = \"BOOTSTRAP_PROFILE.json\"", "out.writestr(zi, profile_data)", "claim_allowed"):
        require(profile_tool, token, "bootstrap profile manifest", errors)

    # Startup and shell environment must consume the same runtime path graph.
    for token in (
        "TermuxRuntimePaths.init(context)", "runtimeFilesDirectoryAccessible()",
        "new File(TermuxRuntimePaths.binDirPath(), \"sh\")",
        "new File(TermuxRuntimePaths.binDirPath(), \"pkg\")",
        "BootstrapBaremetalGuard.validateAfterBootstrap(TermuxRuntimePaths.prefixDirPath())",
        "TermuxShellEnvironment.writeEnvironmentToFile(this)",
    ):
        require(application, token, "application runtime bootstrap init", errors)
    for token in (
        "ENV_HOME, TermuxRuntimePaths.homeDirPath()", "ENV_PREFIX, TermuxRuntimePaths.prefixDirPath()",
        "ENV_TMPDIR, TermuxRuntimePaths.tmpDirPath()", "ENV_PATH, TermuxRuntimePaths.binDirPath()",
        "TERMUX_REAL_PKG_RELOCATION_CLAIM_ALLOWED", "false",
    ):
        require(shell_env, token, "runtime shell environment", errors)
    for token in (
        "Do not hard-gate terminal startup on bash", "LOGIN_SHELL_BINARIES", "/system/bin/sh",
        "TermuxRuntimePaths.layoutState()",
    ):
        require(session, token, "terminal startup fallback", errors)

    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("bootstrap_package_install_contract=FAIL")
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print("bootstrap_package_install_contract=PASS")
    print("claim_boundary=structural_only_physical_filesystem_and_first_shell_still_required")
    print("bootstrap_generation=preflight_materialized_rewritten_archives")
    print("native_incbin=rewritten_bootstrap_packages_declared")
    print("wizard_bootstrap_document_source=fail_closed_b3_abi_profile_bound")
    print("runtime_filesystem=context_getFilesDir_resolved")
    print("relocated_bridge_runtime=structurally_supported_claim_still_closed")
    print("real_pkg_relocation=BLOCKED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
