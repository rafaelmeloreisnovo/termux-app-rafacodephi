#!/usr/bin/env python3
"""Render a concise, elegant GitHub Actions beta operations dashboard.

The script intentionally uses only the Python standard library so it can run in a
fresh GitHub-hosted runner without extra dependencies.
"""

from __future__ import annotations

import argparse
import csv
import os
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

ROOT = Path.cwd()
OUT_DIR = ROOT / "dist" / "apk-matrix"
RUNTIME_OUT = ROOT / "out"

REQUIRED_STEPS = {
    "checkout_packages": True,
    "bind_packages": True,
    "build_real_bootstrap": True,
    "gate_real_bootstrap": True,
    "setup_android": True,
    "prepare_bootstrap": True,
    "real_profile_gate": True,
    "apk_matrix": True,
    "build_guard": True,
    "run_guard": False,
    "build_pss3": True,
    "run_pss3": False,
    "blocker_gate": True,
    "publish_summary": False,
    "upload_artifacts": False,
}

STEP_LABELS = {
    "checkout_packages": "termux-packages checkout",
    "bind_packages": "RAFCODEΦ package binding",
    "build_real_bootstrap": "Real ARM32 + ARM64 bootstrap source-build",
    "gate_real_bootstrap": "Real bootstrap provenance gate",
    "setup_android": "Android toolchain",
    "prepare_bootstrap": "Bootstrap hashes",
    "real_profile_gate": "Embedded real-pkg profile gate",
    "apk_matrix": "APK matrix",
    "build_guard": "Baremetal guard build",
    "run_guard": "Baremetal guard selftest",
    "build_pss3": "PSS3 lab build",
    "run_pss3": "PSS3 audit",
    "blocker_gate": "BLOCKER_BETA gate",
    "publish_summary": "Dashboard render",
    "upload_artifacts": "Artifact upload",
}


def read_tsv(path: Path) -> List[Dict[str, str]]:
    if not path.exists():
        return []
    with path.open("r", encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def read_text(path: Path, fallback: str = "not generated") -> str:
    if not path.exists():
        return fallback
    return path.read_text(encoding="utf-8", errors="replace").strip() or fallback


def human_size(raw: str | int | None) -> str:
    try:
        size = int(raw or 0)
    except (TypeError, ValueError):
        return "n/a"
    units = ["B", "KiB", "MiB", "GiB"]
    value = float(size)
    unit = units[0]
    for unit in units:
        if value < 1024 or unit == units[-1]:
            break
        value /= 1024
    if unit == "B":
        return f"{int(value)} {unit}"
    return f"{value:.2f} {unit}"


def status_badge(outcome: str, required: bool) -> str:
    normalized = (outcome or "not-run").lower()
    if normalized == "success":
        return "✅ success"
    if normalized in {"skipped", "not-run", ""}:
        return "⚪ skipped" if not required else "⚠️ not-run"
    if normalized in {"failure", "cancelled", "timed_out", "timed-out"}:
        return "🚨 failure" if required else "🟡 warning"
    return f"ℹ️ {normalized}"


def step_outcomes() -> Dict[str, str]:
    outcomes: Dict[str, str] = {}
    for key in REQUIRED_STEPS:
        env_key = f"{key.upper()}_OUTCOME"
        outcomes[key] = os.getenv(env_key, "not-run")
    return outcomes


def required_failed(outcomes: Dict[str, str]) -> bool:
    return any(
        required and (outcomes.get(key, "not-run") or "not-run").lower() != "success"
        for key, required in REQUIRED_STEPS.items()
    )


def release_diff_rows() -> List[Dict[str, str]]:
    rows = read_tsv(OUT_DIR / "APK_SIZE_DIFF_RELEASE.tsv")
    order = {"armeabi-v7a": 0, "arm64-v8a": 1, "x86_64": 2, "universal": 3}
    return sorted(rows, key=lambda row: order.get(row.get("abi", ""), 99))


def apk_size_rows() -> List[Dict[str, str]]:
    return read_tsv(OUT_DIR / "APK_SIZE_REPORT.tsv")


def manifest_values() -> Dict[str, str]:
    values: Dict[str, str] = {}
    manifest = OUT_DIR / "ARTIFACT_MANIFEST.txt"
    if not manifest.exists():
        return values
    for line in manifest.read_text(encoding="utf-8", errors="replace").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip()
    return values


def sha_count() -> int:
    sums = OUT_DIR / "SHA256SUMS.txt"
    if not sums.exists():
        return 0
    return sum(1 for line in sums.read_text(encoding="utf-8", errors="replace").splitlines() if line.strip())


def bar(size_bytes: str, maximum: int) -> str:
    try:
        size = int(size_bytes)
    except ValueError:
        return ""
    if maximum <= 0:
        return ""
    width = max(1, round((size / maximum) * 12))
    return "█" * width


def markdown_table(headers: Iterable[str], rows: Iterable[Iterable[str]]) -> List[str]:
    header_list = list(headers)
    lines = ["| " + " | ".join(header_list) + " |", "| " + " | ".join(["---"] * len(header_list)) + " |"]
    for row in rows:
        lines.append("| " + " | ".join(str(cell) for cell in row) + " |")
    return lines


def render() -> str:
    outcomes = step_outcomes()
    failed = required_failed(outcomes)
    status = "🚨 Needs attention" if failed else "✅ Operationally green"
    manifest = manifest_values()
    diff_rows = release_diff_rows()
    size_rows = apk_size_rows()
    signed_release_rows = [
        row for row in size_rows
        if row.get("type") == "signed" and "release" in row.get("apk", "")
    ]
    unsigned_release_rows = [
        row for row in size_rows
        if row.get("type") == "unsigned" and "release" in row.get("apk", "")
    ]
    max_signed = max((int(row.get("signed_size_bytes", "0") or 0) for row in diff_rows), default=0)

    lines: List[str] = []
    lines.append("# RAFCODEΦ Beta Operations Dashboard")
    lines.append("")
    lines.append(f"> {status} · direct CI view for beta readiness, APK generation, signing, guard checks and artifact publication.")
    lines.append("")

    lines.extend(markdown_table(
        ["Signal", "Value"],
        [
            ["Commit", os.getenv("GITHUB_SHA", "unknown")[:12]],
            ["Branch", os.getenv("GITHUB_REF_NAME", os.getenv("GITHUB_REF", "unknown"))],
            ["Run", os.getenv("GITHUB_RUN_ID", "unknown")],
            ["Track", manifest.get("release_track", os.getenv("RELEASE_TRACK", "internal"))],
            ["Generated UTC", manifest.get("generated_at_utc", "not generated")],
            ["Signed release APKs", str(len(signed_release_rows))],
            ["Unsigned release APKs", str(len(unsigned_release_rows))],
            ["SHA256 entries", str(sha_count())],
        ],
    ))
    lines.append("")

    lines.append("## Pipeline map")
    lines.append("")
    lines.append("```mermaid")
    lines.append("flowchart LR")
    lines.append("  A[termux-packages pin] --> B[Real bootstrap source-build]")
    lines.append("  B --> C[Provenance + real-pkg profile gates]")
    lines.append("  C --> D[Bootstrap hashes]")
    lines.append("  D --> E[Debug + Release APKs]")
    lines.append("  E --> F[Local signing + reports]")
    lines.append("  F --> G[Guard + PSS3 diagnostics]")
    lines.append("  G --> H[Artifacts + final gate]")
    lines.append("```")
    lines.append("")

    lines.append("## Execution gate")
    lines.append("")
    gate_rows = []
    for key, required in REQUIRED_STEPS.items():
        gate_rows.append([
            STEP_LABELS.get(key, key),
            "required" if required else "optional",
            status_badge(outcomes.get(key, "not-run"), required),
        ])
    lines.extend(markdown_table(["Stage", "Policy", "Outcome"], gate_rows))
    lines.append("")

    lines.append("## Release APK matrix")
    lines.append("")
    if diff_rows:
        matrix_rows = []
        for row in diff_rows:
            matrix_rows.append([
                row.get("abi", "n/a"),
                human_size(row.get("unsigned_size_bytes")),
                human_size(row.get("signed_size_bytes")),
                human_size(row.get("delta_bytes")),
                bar(row.get("signed_size_bytes", "0"), max_signed),
            ])
        lines.extend(markdown_table(["ABI", "Unsigned", "Signed", "Δ signing", "Visual"], matrix_rows))
    else:
        lines.append("> APK size diff report was not generated.")
    lines.append("")

    lines.append("## Artifact contract")
    lines.append("")
    artifact_rows = [
        ["Manifest", "✅ present" if (OUT_DIR / "ARTIFACT_MANIFEST.txt").exists() else "⚠️ missing"],
        ["APK size report", "✅ present" if (OUT_DIR / "APK_SIZE_REPORT.tsv").exists() else "⚠️ missing"],
        ["Release size diff", "✅ present" if (OUT_DIR / "APK_SIZE_DIFF_RELEASE.tsv").exists() else "⚠️ missing"],
        ["SHA256 sums", "✅ present" if (OUT_DIR / "SHA256SUMS.txt").exists() else "⚠️ missing"],
        ["Baremetal guard", "✅ present" if (RUNTIME_OUT / "bootstrap_baremetal_guard_smoke.txt").exists() else "⚠️ missing"],
        ["PSS3 report", "✅ present" if (RUNTIME_OUT / "pss3_failure_report.txt").exists() else "⚠️ missing"],
    ]
    lines.extend(markdown_table(["Artifact", "Status"], artifact_rows))
    lines.append("")

    guard_text = read_text(RUNTIME_OUT / "bootstrap_baremetal_guard_smoke.txt", "not generated")
    pss3_text = read_text(RUNTIME_OUT / "pss3_failure_report.txt", "not generated")

    lines.append("## Guard diagnostics")
    lines.append("")
    lines.append("```text")
    lines.append("\n".join(guard_text.splitlines()[:12]))
    lines.append("```")
    lines.append("")

    lines.append("## PSS3 audit")
    lines.append("")
    lines.append("```text")
    lines.append("\n".join(pss3_text.splitlines()[:12]))
    lines.append("```")
    lines.append("")

    lines.append("## Operator readout")
    lines.append("")
    if failed:
        lines.append("- 🚨 A required diagnostic failed or did not run. Inspect the first non-success required stage before trusting downstream stages.")
    else:
        lines.append("- ✅ All required source-build, provenance, APK and diagnostic gates reported success.")
    lines.append("- 📦 Publishable artifacts live under `dist/apk-matrix/` and are uploaded by the workflow.")
    lines.append("- 🔐 Local validation signing is internal-only; official signing still requires explicit release secrets.")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Render RAFCODEΦ beta operations dashboard")
    parser.add_argument("--out", default="out/beta-dashboard.md", help="Markdown output path")
    parser.add_argument("--summary", default=os.getenv("GITHUB_STEP_SUMMARY", ""), help="Optional GitHub step summary path")
    args = parser.parse_args()

    markdown = render()
    output = Path(args.out)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(markdown + "\n", encoding="utf-8")

    if args.summary:
        with Path(args.summary).open("a", encoding="utf-8") as handle:
            handle.write(markdown + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
