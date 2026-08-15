from __future__ import annotations

from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
ANDROID = "{http://schemas.android.com/apk/res/android}"


def test_explicit_settings_activity_targets_are_declared_in_manifest() -> None:
    manifest_root = ET.parse(ROOT / "app/src/main/AndroidManifest.xml").getroot()
    preferences_root = ET.parse(ROOT / "app/src/main/res/xml/root_preferences.xml").getroot()

    declared_activities = {
        activity.get(ANDROID + "name")
        for activity in manifest_root.findall("./application/activity")
    }
    explicit_targets = {
        intent.get(ANDROID + "targetClass")
        for intent in preferences_root.findall(".//intent")
        if intent.get(ANDROID + "targetClass")
    }

    assert "com.termux.app.activities.BetaOrchestratorActivity" in explicit_targets
    assert explicit_targets <= declared_activities

    beta = next(
        activity
        for activity in manifest_root.findall("./application/activity")
        if activity.get(ANDROID + "name") == "com.termux.app.activities.BetaOrchestratorActivity"
    )
    assert beta.get(ANDROID + "exported") == "false"


if __name__ == "__main__":
    test_explicit_settings_activity_targets_are_declared_in_manifest()
    print("SETTINGS_MANIFEST_ACTIVITY_CONTRACT=PASS")
