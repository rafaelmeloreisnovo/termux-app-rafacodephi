from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_main_app_defines_signature_permission_for_api_broadcasts() -> None:
    manifest = (ROOT / "app/src/main/AndroidManifest.xml").read_text(encoding="utf-8")
    assert 'android:name="${TERMUX_PACKAGE_NAME}.permission.TERMUX_API"' in manifest
    assert 'android:protectionLevel="signature"' in manifest
    assert '<uses-permission android:name="${TERMUX_PACKAGE_NAME}.permission.TERMUX_API" />' in manifest
    assert "android:sharedUserId" not in manifest


def test_main_app_exposes_canonical_paired_signing_interface() -> None:
    gradle = (ROOT / "app/build.gradle").read_text(encoding="utf-8")
    for token in [
        "RAFCODEPHI_PAIRED_KEYSTORE_FILE",
        "RAFCODEPHI_PAIRED_KEYSTORE_PASSWORD",
        "RAFCODEPHI_PAIRED_KEY_ALIAS",
        "RAFCODEPHI_PAIRED_KEY_PASSWORD",
        "hasAnyPairedSigningMaterial && !hasPairedSigningMaterial",
    ]:
        assert token in gradle
    assert "releaseSigningValues = hasPairedSigningMaterial ? pairedSigningValues" in gradle


def test_api_android_identity_is_distinct_from_its_java_component_package() -> None:
    constants = (
        ROOT
        / "termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java"
    ).read_text(encoding="utf-8")
    assert 'TERMUX_API_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".api"' in constants
    assert 'TERMUX_API_CODE_PACKAGE_NAME = "com.termux.api"' in constants
    assert (
        'TERMUX_API_MAIN_ACTIVITY_NAME = TERMUX_API_CODE_PACKAGE_NAME + '
        '".activities.TermuxAPIMainActivity"'
    ) in constants
