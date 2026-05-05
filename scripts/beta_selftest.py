#!/usr/bin/env python3
"""Host-side beta readiness checks for docs, lifecycle source contracts and artifacts."""
from __future__ import annotations

import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_DOCS = [
    "README_BETA.md",
    "INVENTARIO.md",
    "docs/BETA_READINESS_REPORT.md",
    "docs/BETA_KNOWN_LIMITATIONS.md",
]

SOURCE_CONTRACTS = {
    "terminal-emulator/src/main/java/com/termux/terminal/TerminalSession.java": [
        "Failed to create terminal subprocess",
        "Os.kill(-shellPid, OsConstants.SIGKILL)",
        "Os.kill(shellPid, OsConstants.SIGKILL)",
        "mTerminalFileDescriptor = -1",
        "mResourcesCleaned",
        "if (terminalFileDescriptor >= 0)",
    ],
    "terminal-emulator/src/main/jni/termux.c": [
        "#include <errno.h>",
        "errno == EINTR",
        "return -1;",
    ],
}

EXPERIMENTAL_TERMS = [
    "sqrt(3)/2",
    "Fibonacci",
    "Mandelbrot",
    "Poincare",
    "42K",
    "10x10x10",
    "BitOmega",
    "ZipRAF",
]


def fail(message: str) -> None:
    print(f"BETA_SELFTEST_FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def read(path: str) -> str:
    file_path = ROOT / path
    if not file_path.is_file():
        fail(f"missing required file: {path}")
    return file_path.read_text(encoding="utf-8")


def check_docs() -> None:
    combined = []
    for doc in REQUIRED_DOCS:
        text = read(doc)
        combined.append(text)
        for marker in ("BLOCKER_BETA", "RISK_BETA", "EXPERIMENTAL_NOT_BLOCKING"):
            if marker not in text and doc.endswith(("BETA_READINESS_REPORT.md", "BETA_KNOWN_LIMITATIONS.md", "README_BETA.md")):
                fail(f"{doc} does not mention {marker}")
    all_docs = "\n".join(combined)
    for term in EXPERIMENTAL_TERMS:
        if term not in all_docs:
            fail(f"experimental term not inventoried in beta docs: {term}")


def check_source_contracts() -> None:
    for path, needles in SOURCE_CONTRACTS.items():
        text = read(path)
        for needle in needles:
            if needle not in text:
                fail(f"{path} missing source contract: {needle}")


def check_artifacts_if_present() -> None:
    artifact_dirs = [ROOT / "dist" / "apk-matrix", ROOT / "dist" / "release-artifacts"]
    apks = sorted(apk for directory in artifact_dirs if directory.exists() for apk in directory.rglob("*.apk"))
    if not apks:
        print("BETA_SELFTEST_WARN: no APK artifacts under dist/ yet; run scripts/build_apk_matrix.sh to generate signed+unsigned APKs")
        return
    for apk in apks:
        digest = hashlib.sha256(apk.read_bytes()).hexdigest()
        print(f"BETA_ARTIFACT_SHA256 {apk.relative_to(ROOT)} {digest}")


def main() -> int:
    check_docs()
    check_source_contracts()
    check_artifacts_if_present()
    print("BETA_SELFTEST_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
