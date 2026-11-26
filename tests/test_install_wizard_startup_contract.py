from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_storage_permission_callback_continues_into_service_and_bootstrap() -> None:
    activity = read("app/src/main/java/com/termux/app/TermuxActivity.java")

    assert "ensureStorageAccessOrRequest" in activity
    assert "PermissionUtils.REQUEST_GRANT_STORAGE_PERMISSION" in activity
    assert "requestStoragePermission(true)" in activity
    assert "TermuxInstaller.setupStorageSymlinks" in activity
    assert "startAndBindTermuxServiceOrFail()" in activity
    assert "TermuxInstaller.setupBootstrapIfNeeded" in activity

    permission_callback = activity.index("public void onRequestPermissionsResult")
    permission_storage = activity.index("requestStoragePermission(true)", permission_callback)
    permission_service = activity.index("startAndBindTermuxServiceOrFail()", permission_storage)
    assert permission_storage < permission_service

    activity_result = activity.index("protected void onActivityResult")
    result_storage = activity.index("requestStoragePermission(true)", activity_result)
    result_service = activity.index("startAndBindTermuxServiceOrFail()", result_storage)
    assert result_storage < result_service

    service_connected = activity.index("public void onServiceConnected")
    bootstrap_call = activity.index("TermuxInstaller.setupBootstrapIfNeeded", service_connected)
    initial_session = activity.index("createInitialSession(intent)", bootstrap_call)
    assert bootstrap_call < initial_session


def test_bootstrap_guard_requires_shell_package_and_internal_storage_contract() -> None:
    guard = read("app/src/main/java/com/termux/app/BootstrapBaremetalGuard.java")

    assert "validateAfterBootstrap" in guard
    assert "validateInstallFilesystemAndShell" in guard
    assert "TERMUX_HOME_DIR" in guard
    assert "TERMUX_STORAGE_HOME_DIR" in guard
    assert "ensureStoragePlaceholder" in guard
    assert "bin/sh" in guard
    assert "bin/pkg" in guard
    assert "verifyOwnerExecutable" in guard
    assert "armeabi-v7a" in guard


def test_storage_symlinks_are_best_effort_after_permission() -> None:
    installer = read("app/src/main/java/com/termux/app/TermuxInstaller.java")

    assert "setupStorageSymlinks" in installer
    assert "createSymlinkSafely" in installer
    assert "Environment.getExternalStorageDirectory" in installer
    assert "getExternalFilesDirs" in installer
    assert "getExternalMediaDirs" in installer
    assert "Runtime storage links ready at" in installer
