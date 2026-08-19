from __future__ import annotations

from collections import Counter
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
ANDROID = "{http://schemas.android.com/apk/res/android}"
BETA_ACTIVITY = "com.termux.app.activities.BetaOrchestratorActivity"
BETA_THEME = "@style/Theme.TermuxApp.DayNight.DarkActionBar"


def test_explicit_settings_activity_targets_are_declared_in_manifest() -> None:
    manifest_root = ET.parse(ROOT / "app/src/main/AndroidManifest.xml").getroot()
    preferences_root = ET.parse(ROOT / "app/src/main/res/xml/root_preferences.xml").getroot()

    activities = manifest_root.findall("./application/activity")
    activity_names = [activity.get(ANDROID + "name") for activity in activities]
    declared_activities = set(activity_names)
    explicit_targets = {
        intent.get(ANDROID + "targetClass")
        for intent in preferences_root.findall(".//intent")
        if intent.get(ANDROID + "targetClass")
    }

    assert BETA_ACTIVITY in explicit_targets
    assert explicit_targets <= declared_activities

    counts = Counter(activity_names)
    assert counts[BETA_ACTIVITY] == 1, (
        "BetaOrchestratorActivity must have exactly one canonical manifest declaration; "
        f"observed={counts[BETA_ACTIVITY]}"
    )

    beta = next(
        activity
        for activity in activities
        if activity.get(ANDROID + "name") == BETA_ACTIVITY
    )
    assert beta.get(ANDROID + "exported") == "false"
    assert beta.get(ANDROID + "theme") == BETA_THEME


if __name__ == "__main__":
    test_explicit_settings_activity_targets_are_declared_in_manifest()
    print("SETTINGS_MANIFEST_ACTIVITY_CONTRACT=PASS")
