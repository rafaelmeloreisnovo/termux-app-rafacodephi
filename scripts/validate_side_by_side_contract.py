#!/usr/bin/env python3
"""
Validate canonical side-by-side contract for Termux RAFCODEΦ.

This is a fast static gate that catches identity drift across build/runtime/manifest
without requiring Android SDK installation.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read_text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(pattern: str, text: str, message: str, *, multiline: bool = True) -> str | None:
    flags = re.MULTILINE if multiline else 0
    if not re.search(pattern, text, flags):
        return message
    return None


def main() -> int:
    errors: list[str] = []

    build_gradle = read_text("app/build.gradle")
    errors += filter(
        None,
        [
            require(
                r'appPackageName\s*=\s*System\.getenv\("TERMUX_APP_PACKAGE_NAME"\)\s*\?:\s*"com\.termux\.rafacodephi"',
                build_gradle,
                "app/build.gradle: canonical default TERMUX_APP_PACKAGE_NAME is not com.termux.rafacodephi",
            ),
            require(
                r'bootstrapMetadataPackageName\s*=\s*System\.getenv\("TERMUX_BOOTSTRAP_PACKAGE_NAME"\)\s*\?:\s*"com\.termux\.rafacodephi"',
                build_gradle,
                "app/build.gradle: canonical default TERMUX_BOOTSTRAP_PACKAGE_NAME is not com.termux.rafacodephi",
            ),
        ],
    )

    constants_java = read_text("termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java")
    errors += filter(
        None,
        [
            require(
                r'TERMUX_PACKAGE_NAME\s*=\s*"com\.termux\.rafacodephi"',
                constants_java,
                "TermuxConstants.java: TERMUX_PACKAGE_NAME is not canonical com.termux.rafacodephi",
            ),
            require(
                r'TERMUX_APP_CODE_PACKAGE_NAME\s*=\s*"com\.termux"',
                constants_java,
                "TermuxConstants.java: TERMUX_APP_CODE_PACKAGE_NAME missing or changed unexpectedly",
            ),
        ],
    )

    shortcuts = read_text("app/src/main/res/xml/shortcuts.xml")
    errors += filter(
        None,
        [
            require(
                r'android:targetPackage="com\.termux\.rafacodephi"',
                shortcuts,
                "shortcuts.xml: targetPackage must point to canonical app id",
            ),
            require(
                r'<extra android:name="com\.termux\.rafacodephi\.app\.failsafe_session"',
                shortcuts,
                "shortcuts.xml: failsafe extra key must use canonical package prefix",
            ),
        ],
    )

    manifest = read_text("app/src/main/AndroidManifest.xml")
    if 'android:name=".app.' in manifest or 'android:name=".shared.' in manifest or 'android:name=".filepicker.' in manifest:
        errors.append("AndroidManifest.xml: relative component names (.app/.shared/.filepicker) are forbidden in canonical mode")

    if errors:
        print("❌ Side-by-side contract validation failed:")
        for err in errors:
            print(f"  - {err}")
        return 1

    print("✅ Side-by-side contract validation passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
