from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_first_boot_gate_requires_only_shell_and_pkg_while_full_apt_stays_fail_closed() -> None:
    gate = read("app/src/main/java/com/termux/app/BootstrapReadinessGate.java")

    startup = gate.split("public static Report evaluateStartup", 1)[1].split(
        "public static Report evaluate(Context context)", 1
    )[0]
    assert '"sh"' in gate
    assert '"pkg"' in gate
    assert "STARTUP_REQUIRED_EXECUTABLES" in startup
    assert "OPTIONAL_COMPATIBILITY_WRAPPERS" in startup
    assert "optional_storage_link_not_a_startup_gate" in startup
    assert "profileContract(context, checks, prefix)" not in startup
    assert "REQUIRED_REAL_ELFS" not in startup

    full = gate.split("public static Report evaluate(Context context)", 1)[1].split(
        "private static boolean directory", 1
    )[0]
    assert "profileContract(context, checks, prefix)" in full
    assert "REQUIRED_REAL_ELFS" in full
    assert "FULL_PACKAGE_RUNTIME" in full
    assert "BOOTSTRAP_REAL_PROFILE_CONTRACT_BLOCKED" in full


def test_repair_preserves_startup_runtime_when_package_repository_is_not_ready() -> None:
    repair = read("app/src/main/java/com/termux/app/BetaRealBootstrapRepair.java")

    callback = repair.split("TermuxInstaller.setupBootstrapIfNeeded(activity, () -> {", 1)[1].split(
        "Logger.logError", 1
    )[0]
    assert "BootstrapReadinessGate.evaluateStartup(activity)" in callback
    assert "BootstrapReadinessGate.evaluate(activity)" in callback
    assert "afterStartup.isPass()" in callback
    assert "STARTUP_PASS_PACKAGE_RUNTIME_BLOCKED" in callback
    assert "restoreBackupAfterRejectedInstall" not in callback


def test_installer_and_native_guard_do_not_treat_archive_symlink_metadata_as_a_runtime_file() -> None:
    installer = read("app/src/main/java/com/termux/app/TermuxInstaller.java")
    guard = read("app/src/main/java/com/termux/app/BootstrapBaremetalGuard.java")
    source = read("app/src/main/java/com/termux/app/BootstrapWizardSource.java")

    assert 'verifyRuntimeBinary(new File(staging, "bin/sh"), "sh", true)' in installer
    assert 'verifyRuntimeBinary(new File(staging, "bin/pkg"), "pkg", true)' in installer
    assert 'verifyRuntimeBinary(new File(staging, "bin/busybox"), "busybox", false)' in installer
    assert 'verifyRuntimeBinary(new File(staging, "bin/proot"), "proot", false)' in installer

    assert 'SOURCE_ONLY_SYMLINKS_FILE = "SYMLINKS.txt"' in guard
    assert "installed bootstrap profile must set runtime_materialized=true" in guard
    assert "if (SOURCE_ONLY_SYMLINKS_FILE.equals(relative)) continue;" in guard
    assert "Busybox/proot belong to the later full package-runtime" in source
    assert "if (!symlinks || !sh || !pkg)" in source


def test_settings_entries_are_bound_on_ui_thread_and_activity_launches_return_to_ui_thread() -> None:
    settings = read("app/src/main/java/com/termux/app/activities/SettingsActivity.java")

    preferences = settings.split("public void onCreatePreferences", 1)[1].split(
        "private void configureRafcodephiControlCenterPreference", 1
    )[0]
    assert "setPreferencesFromResource" in preferences
    assert "configureRafcodephiControlCenterPreference(context);" in preferences
    assert "configureVectraRuntimePreference(context);" in preferences
    assert "new Thread" not in preferences

    control_center = settings.split("private void configureRafcodephiControlCenterPreference", 1)[1].split(
        "private void configureAndroid15WizardPreference", 1
    )[0]
    assert 'findPreference("rafcodephi_control_center")' in control_center
    assert "BetaOrchestratorActivity.class" in control_center

    vectra = settings.split("private void configureVectraRuntimePreference", 1)[1].split(
        "private void configureTermuxAPIPreference", 1
    )[0]
    assert 'findPreference("vectra_runtime")' in vectra
    assert "VectraRuntimeActivity.class" in vectra

    about = settings.split("private void configureAboutPreference", 1)[1].split(
        "private void configureDonatePreference", 1
    )[0]
    assert "requireActivity().runOnUiThread" in about
    assert "ReportActivity.startReportActivity" in about


if __name__ == "__main__":
    tests = [value for name, value in sorted(globals().items()) if name.startswith("test_") and callable(value)]
    for test in tests:
        test()
        print(f"PASS {test.__name__}")
