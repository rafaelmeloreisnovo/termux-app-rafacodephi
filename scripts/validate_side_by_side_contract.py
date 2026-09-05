#!/usr/bin/env python3
"""
Validate canonical side-by-side contract for Termux RAFCODEΦ.

This is a fast static gate that catches identity drift across build/runtime/manifest
without requiring Android SDK installation.

The runtime package constants are generated through BuildConfig. The validator
therefore verifies the source of those values in Gradle and the Java binding to
BuildConfig instead of requiring stale hard-coded literals in Java source.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CANONICAL_PACKAGE = "com.termux.rafacodephi"
CANONICAL_CODE_PACKAGE = "com.termux.app"
REQUIRED_PAGE_SIZE = "16384"
PACKAGE_RE = re.compile(r"^[a-z][a-z0-9_]*(?:\.[a-z][a-z0-9_]*)+$")


def read_text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(pattern: str, text: str, message: str, *, multiline: bool = True) -> str | None:
    flags = re.MULTILINE if multiline else 0
    if not re.search(pattern, text, flags):
        return message
    return None


def gradle_default(name: str, text: str) -> str | None:
    pattern = re.compile(
        rf'{name}\s*=\s*System\.getenv\("[^"]+"\)\s*\?:\s*"([^"]+)"'
    )
    match = pattern.search(text)
    return match.group(1) if match else None


def require_gradle_default(
    label: str, name: str, expected: str, text: str, errors: list[str]
) -> None:
    actual = gradle_default(name, text)
    if actual != expected:
        errors.append(f"{label}: {name} default must be {expected}, got {actual!r}")


def require_valid_package(label: str, package_name: str | None, errors: list[str]) -> None:
    if not package_name or not PACKAGE_RE.fullmatch(package_name):
        errors.append(f"{label} must be a concrete Android package name, got {package_name!r}")


def main() -> int:
    errors: list[str] = []

    build_gradle = read_text("app/build.gradle")
    require_gradle_default("app/build.gradle", "appPackageName", CANONICAL_PACKAGE, build_gradle, errors)
    require_gradle_default(
        "app/build.gradle", "appCodePackageName", CANONICAL_CODE_PACKAGE, build_gradle, errors
    )
    require_gradle_default(
        "app/build.gradle", "bootstrapMetadataPackageName", CANONICAL_PACKAGE, build_gradle, errors
    )
    require_gradle_default(
        "app/build.gradle", "bootstrapRequiredPageSize", REQUIRED_PAGE_SIZE, build_gradle, errors
    )

    app_package = gradle_default("appPackageName", build_gradle)
    app_code_package = gradle_default("appCodePackageName", build_gradle)
    bootstrap_package = gradle_default("bootstrapMetadataPackageName", build_gradle)
    require_valid_package("app/build.gradle: appPackageName", app_package, errors)
    require_valid_package("app/build.gradle: appCodePackageName", app_code_package, errors)
    require_valid_package("app/build.gradle: bootstrapMetadataPackageName", bootstrap_package, errors)
    if app_package and bootstrap_package and app_package != bootstrap_package:
        errors.append(
            "app/build.gradle: appPackageName and bootstrapMetadataPackageName must match "
            "for side-by-side bootstrap paths"
        )

    shared_gradle = read_text("termux-shared/build.gradle")
    require_gradle_default(
        "termux-shared/build.gradle", "appPackageName", CANONICAL_PACKAGE, shared_gradle, errors
    )
    require_gradle_default(
        "termux-shared/build.gradle",
        "appCodePackageName",
        CANONICAL_CODE_PACKAGE,
        shared_gradle,
        errors,
    )

    bootstrap_profile = read_text("scripts/build_bootstrap_profile.sh")
    bootstrap_builder = read_text("scripts/build_rafaelia_bootstraps.sh")
    bootstrap_zip_builder = read_text("scripts/bootstrap_zip_builder.c")
    errors += filter(
        None,
        [
            require(
                r'manifestPlaceholders\.TERMUX_PACKAGE_NAME\s*=\s*project\.ext\.appPackageName',
                build_gradle,
                "app/build.gradle: manifest TERMUX_PACKAGE_NAME must be derived from appPackageName",
            ),
            require(
                r'buildConfigField\s+"String",\s*"TERMUX_PACKAGE_NAME",\s*buildConfigString\(project\.ext\.appPackageName\)',
                build_gradle,
                "app/build.gradle: TERMUX_PACKAGE_NAME BuildConfig must derive from appPackageName",
            ),
            require(
                r'buildConfigField\s+"String",\s*"TERMUX_APP_CODE_PACKAGE_NAME",\s*buildConfigString\(project\.ext\.appCodePackageName\)',
                build_gradle,
                "app/build.gradle: TERMUX_APP_CODE_PACKAGE_NAME BuildConfig must derive from appCodePackageName",
            ),
            require(
                r'buildConfigField\s+"String",\s*"TERMUX_PACKAGE_NAME",\s*buildConfigString\(project\.ext\.appPackageName\)',
                shared_gradle,
                "termux-shared/build.gradle: TERMUX_PACKAGE_NAME BuildConfig must derive from appPackageName",
            ),
            require(
                r'buildConfigField\s+"String",\s*"TERMUX_APP_CODE_PACKAGE_NAME",\s*buildConfigString\(project\.ext\.appCodePackageName\)',
                shared_gradle,
                "termux-shared/build.gradle: TERMUX_APP_CODE_PACKAGE_NAME BuildConfig must derive from appCodePackageName",
            ),
            require(
                rf'PACKAGE_NAME="\$\{{TERMUX_BOOTSTRAP_PACKAGE_NAME:-{re.escape(CANONICAL_PACKAGE)}\}}"',
                bootstrap_profile,
                "scripts/build_bootstrap_profile.sh: profile package must derive from TERMUX_BOOTSTRAP_PACKAGE_NAME",
            ),
            require(
                rf': "\$\{{TERMUX_BOOTSTRAP_PACKAGE_NAME:={re.escape(CANONICAL_PACKAGE)}\}}"',
                bootstrap_builder,
                "scripts/build_rafaelia_bootstraps.sh: bootstrap package default must be canonical",
            ),
            require(
                rf': "\$\{{TERMUX_BOOTSTRAP_PAGE_SIZE:={REQUIRED_PAGE_SIZE}\}}"',
                bootstrap_builder,
                "scripts/build_rafaelia_bootstraps.sh: bootstrap page size default must be 16384",
            ),
            require(
                r'prefix="/data/data/\$\{TERMUX_BOOTSTRAP_PACKAGE_NAME\}/files/usr"',
                bootstrap_builder,
                "scripts/build_rafaelia_bootstraps.sh: prefix must be derived from TERMUX_BOOTSTRAP_PACKAGE_NAME",
            ),
            require(
                r'TERMUX_PACKAGE_NAME=',
                bootstrap_zip_builder,
                "scripts/bootstrap_zip_builder.c: BOOTSTRAP_INFO must include TERMUX_PACKAGE_NAME",
            ),
            require(
                r'TERMUX_PAGE_SIZE=',
                bootstrap_zip_builder,
                "scripts/bootstrap_zip_builder.c: BOOTSTRAP_INFO must include TERMUX_PAGE_SIZE",
            ),
            require(
                r'hasReleaseTaskRequested\(\)\s*&&\s*bootstrapBaremetalStrictOverride\s*==\s*false',
                build_gradle,
                "app/build.gradle: release tasks must reject disabled baremetal bootstrap strict mode",
            ),
        ],
    )

    constants_java = read_text("termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java")
    errors += filter(
        None,
        [
            require(
                r'TERMUX_PACKAGE_NAME\s*=\s*BuildConfig\.TERMUX_PACKAGE_NAME\s*;',
                constants_java,
                "TermuxConstants.java: TERMUX_PACKAGE_NAME must bind to BuildConfig.TERMUX_PACKAGE_NAME",
            ),
            require(
                r'TERMUX_APP_CODE_PACKAGE_NAME\s*=\s*BuildConfig\.TERMUX_APP_CODE_PACKAGE_NAME\s*;',
                constants_java,
                "TermuxConstants.java: TERMUX_APP_CODE_PACKAGE_NAME must bind to BuildConfig.TERMUX_APP_CODE_PACKAGE_NAME",
            ),
            require(
                r'TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR_PATH\s*=\s*"/data/data/" \+ TERMUX_PACKAGE_NAME[\s\S]*TERMUX_FILES_DIR_PATH\s*=\s*TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR_PATH \+ "/files"[\s\S]*TERMUX_PREFIX_DIR_PATH\s*=\s*TERMUX_FILES_DIR_PATH \+ "/usr"',
                constants_java,
                "TermuxConstants.java: runtime prefix path must be derived from TERMUX_PACKAGE_NAME",
            ),
        ],
    )

    shortcuts = read_text("app/src/main/res/xml/shortcuts.xml")
    errors += filter(
        None,
        [
            require(
                rf'android:targetPackage="{re.escape(CANONICAL_PACKAGE)}"',
                shortcuts,
                "shortcuts.xml: targetPackage must point to canonical app id",
            ),
            require(
                rf'<extra android:name="{re.escape(CANONICAL_PACKAGE)}\.app\.failsafe_session"',
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
