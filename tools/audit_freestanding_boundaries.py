#!/usr/bin/env python3
"""Inventory and enforce RAFCODEPhi freestanding boundaries.

The runtime core stays independent from this tool. Python is used only as a
build/audit surface. SOURCE, ARTIFACT, EXECUTION, EVIDENCE and CLAIM remain
separate states.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "configs" / "freestanding-pure-core.v1.json"
SOURCE_EXTS = {".c", ".h", ".S", ".s", ".cc", ".cpp", ".hpp", ".inc"}
HEAP_RE = re.compile(r"\b(?:malloc|calloc|realloc|free)\s*\(")
SYSCALL_RE = re.compile(
    r"\b(?:syscall|__NR_|SYS_|proot_sys_|raf_sys_|fs_sc[0-6]|"
    r"freestanding_(?:read|write|open|close|exit))",
    re.I,
)
JNI_RE = re.compile(r"\b(?:JNIEnv|JNIEXPORT|Java_[A-Za-z0-9_]+)\b")
FLOW_RES = {
    "if": re.compile(r"\bif\s*\("),
    "for": re.compile(r"\bfor\s*\("),
    "while": re.compile(r"\bwhile\s*\("),
    "switch": re.compile(r"\bswitch\s*\("),
    "goto": re.compile(r"\bgoto\b"),
    "ternary": re.compile(r"\?"),
}
EXTERNAL_INCLUDE_RE = re.compile(r"^\s*#\s*include\s*<([^>]+)>", re.M)
ASM_RETURN = {"ret"}
ASM_EXACT_BRANCH = {
    "b", "bl", "blx", "br", "blr", "cbz", "cbnz", "tbz", "tbnz",
}
ARM_COND = {
    "beq", "bne", "bcs", "bcc", "bhs", "blo", "bmi", "bpl", "bvs",
    "bvc", "bhi", "bls", "bge", "blt", "bgt", "ble",
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def source_files() -> list[Path]:
    return sorted(
        p for p in ROOT.rglob("*")
        if p.is_file()
        and p.suffix in SOURCE_EXTS
        and ".git" not in p.parts
        and "build" not in p.parts
    )


def focus_files(cfg: dict) -> list[Path]:
    out: list[Path] = []
    for root_name in cfg["inventory_roots"]:
        base = ROOT / root_name
        if not base.exists():
            continue
        out.extend(
            p for p in base.rglob("*")
            if p.is_file() and p.suffix in SOURCE_EXTS
        )
    return sorted(set(out))


def lexical_record(path: Path) -> dict:
    text = path.read_text(encoding="utf-8", errors="replace")
    includes = EXTERNAL_INCLUDE_RE.findall(text)
    flow = {name: len(rx.findall(text)) for name, rx in FLOW_RES.items()}
    return {
        "path": rel(path),
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
        "external_includes": includes,
        "heap_calls": len(HEAP_RE.findall(text)),
        "syscall_markers": len(SYSCALL_RE.findall(text))
        + len(re.findall(r"\bsvc\s*#?0\b", text, flags=re.I))
        + len(re.findall(r"\bint\s+\$0x80\b", text, flags=re.I)),
        "jni_markers": len(JNI_RE.findall(text)),
        "flow": flow,
        "flow_total": sum(flow.values()),
    }


def classify(record: dict) -> str:
    includes = set(record["external_includes"])
    hosted_headers = {
        "jni.h", "dlfcn.h", "unistd.h", "pthread.h", "stdio.h",
        "stdlib.h", "time.h", "android/log.h",
    }
    if record["jni_markers"] or includes & hosted_headers:
        return "HOSTED_ADAPTER"
    if record["syscall_markers"]:
        return "PLATFORM_GATE"
    if record["heap_calls"]:
        return "HEAP_DEPENDENT"
    if record["external_includes"]:
        return "PORTABLE_C_WITH_TOOLCHAIN_HEADERS"
    return "FREESTANDING_CANDIDATE"


def strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


def audit_pure_source(cfg: dict) -> list[str]:
    errors: list[str] = []
    patterns = {
        name: re.compile(expr, re.M)
        for name, expr in cfg["pure_core_forbidden"].items()
    }
    for name in cfg["pure_core_files"]:
        path = ROOT / name
        if not path.exists():
            errors.append(f"missing pure-core file: {name}")
            continue
        text = strip_c_comments(path.read_text(encoding="utf-8"))
        for kind, rx in patterns.items():
            matches = list(rx.finditer(text))
            if matches:
                errors.append(
                    f"{name}: forbidden {kind} marker count={len(matches)}"
                )
    return errors


def parse_asm_branches(asm_text: str) -> list[str]:
    found: list[str] = []
    for raw in asm_text.splitlines():
        line = raw.split("//", 1)[0].split("@", 1)[0].strip()
        if not line or line.startswith((".", "#")) or line.endswith(":"):
            continue
        token = line.split(None, 1)[0].lower()
        operand = line[len(token):].strip().lower()
        if token == "bx" and operand in {"lr", "r14"}:
            continue
        if token in ASM_RETURN:
            continue
        if token in ASM_EXACT_BRANCH or token in ARM_COND or token.startswith("b."):
            found.append(line)
    return found


def compile_probe(cfg: dict) -> tuple[list[dict], list[str]]:
    clang = shutil.which("clang")
    if not clang:
        return [], ["clang not found: assembly/vector probe NOT_RUN"]
    source = ROOT / cfg["assembly_probe"]["source"]
    results: list[dict] = []
    errors: list[str] = []
    with tempfile.TemporaryDirectory(prefix="raf-pure-core-") as td:
        outdir = Path(td)

        vector_name = cfg["assembly_probe"].get("host_vector_source")
        if vector_name:
            vector_source = ROOT / vector_name
            vector_bin = outdir / "pure-q16-vectors"
            vector_compile = subprocess.run(
                [clang, "-std=c11", "-O2", str(vector_source), "-o", str(vector_bin)],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                check=False,
            )
            vector_item = {
                "target": "host-native-q16-vectors",
                "compile_exit": vector_compile.returncode,
                "run_exit": None,
                "branches": [],
            }
            if vector_compile.returncode != 0:
                errors.append(
                    "host q16 vector compile failed: "
                    + vector_compile.stdout.strip()[:1200]
                )
            else:
                vector_run = subprocess.run(
                    [str(vector_bin)],
                    cwd=ROOT,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    check=False,
                )
                vector_item["run_exit"] = vector_run.returncode
                if vector_run.returncode != 0:
                    errors.append(
                        f"host q16 vectors failed count={vector_run.returncode}"
                    )
            results.append(vector_item)

        for target in cfg["assembly_probe"]["targets"]:
            out = outdir / (target.replace("/", "_") + ".s")
            cmd = [
                clang,
                f"--target={target}",
                "-std=c11",
                "-O2",
                "-ffreestanding",
                "-nostdlib",
                "-nostdinc",
                "-fno-builtin",
                "-fno-stack-protector",
                "-fno-unwind-tables",
                "-fno-asynchronous-unwind-tables",
                "-S",
                str(source),
                "-o",
                str(out),
            ]
            cp = subprocess.run(
                cmd,
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                check=False,
            )
            item = {
                "target": target,
                "compile_exit": cp.returncode,
                "branches": [],
            }
            if cp.returncode != 0:
                errors.append(
                    f"{target}: compile failed: {cp.stdout.strip()[:1200]}"
                )
            else:
                asm = out.read_text(encoding="utf-8", errors="replace")
                branches = parse_asm_branches(asm)
                item["branches"] = branches
                if branches:
                    errors.append(
                        f"{target}: non-terminal control transfer(s): "
                        + " | ".join(branches[:12])
                    )
            results.append(item)
    return results, errors


def build_report(cfg: dict, assembly: list[dict], pure_errors: list[str]) -> dict:
    all_sources = source_files()
    focus = focus_files(cfg)
    focus_records = []
    for path in focus:
        rec = lexical_record(path)
        rec["class"] = classify(rec)
        focus_records.append(rec)

    digest_paths: dict[str, list[str]] = defaultdict(list)
    for path in all_sources:
        digest_paths[sha256(path)].append(rel(path))
    duplicate_groups = [
        {"sha256": digest, "paths": paths, "count": len(paths)}
        for digest, paths in digest_paths.items()
        if len(paths) > 1
    ]
    duplicate_groups.sort(key=lambda x: (-x["count"], x["paths"][0]))

    classes = Counter(rec["class"] for rec in focus_records)
    return {
        "schema": "rafaelia.freestanding-boundary-inventory/v1",
        "baseline_commit": cfg["baseline_commit"],
        "source_files_total": len(all_sources),
        "source_unique_blobs": len(digest_paths),
        "exact_duplicate_groups": len(duplicate_groups),
        "exact_duplicate_excess": sum(g["count"] - 1 for g in duplicate_groups),
        "focus_files": len(focus_records),
        "focus_class_counts": dict(sorted(classes.items())),
        "focus_heap_call_files": sum(1 for r in focus_records if r["heap_calls"]),
        "focus_syscall_marker_files": sum(
            1 for r in focus_records if r["syscall_markers"]
        ),
        "focus_jni_marker_files": sum(
            1 for r in focus_records if r["jni_markers"]
        ),
        "pure_core_source_policy": "PASS" if not pure_errors else "FAIL",
        "pure_core_assembly_probe": assembly,
        "claim_allowed": False,
        "physical_android": "TOKEN_VAZIO",
        "duplicates": duplicate_groups,
        "focus": focus_records,
    }


def write_reports(report: dict) -> None:
    reports = ROOT / "reports"
    reports.mkdir(exist_ok=True)
    json_path = reports / "freestanding-boundary-inventory.json"
    md_path = reports / "freestanding-boundary-inventory.md"
    json_path.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    lines = [
        "# Freestanding Boundary Inventory",
        "",
        f"- baseline: `{report['baseline_commit']}`",
        f"- all C/C++/ASM source files: **{report['source_files_total']}**",
        f"- unique source blobs: **{report['source_unique_blobs']}**",
        f"- exact duplicate excess: **{report['exact_duplicate_excess']}**",
        f"- focused low-level files: **{report['focus_files']}**",
        f"- focused files with explicit heap calls: **{report['focus_heap_call_files']}**",
        f"- focused files with syscall markers: **{report['focus_syscall_marker_files']}**",
        f"- focused files with JNI markers: **{report['focus_jni_marker_files']}**",
        f"- pure-core source policy: **{report['pure_core_source_policy']}**",
        "",
        "## Focus classes",
        "",
        "| class | files |",
        "|---|---:|",
    ]
    for name, count in report["focus_class_counts"].items():
        lines.append(f"| {name} | {count} |")
    lines.extend([
        "",
        "## Claim boundary",
        "",
        "Inventory and assembly probes are build evidence only. "
        "They are not physical Android execution and do not promote a release claim.",
        "",
    ])
    md_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--write-report", action="store_true")
    parser.add_argument("--compile-probe", action="store_true")
    args = parser.parse_args()

    cfg = json.loads(CONFIG.read_text(encoding="utf-8"))
    pure_errors = audit_pure_source(cfg)
    assembly: list[dict] = []
    assembly_errors: list[str] = []
    if args.compile_probe:
        assembly, assembly_errors = compile_probe(cfg)

    report = build_report(cfg, assembly, pure_errors)
    if args.write_report:
        write_reports(report)

    print(json.dumps({
        "source_files_total": report["source_files_total"],
        "source_unique_blobs": report["source_unique_blobs"],
        "exact_duplicate_excess": report["exact_duplicate_excess"],
        "focus_files": report["focus_files"],
        "focus_class_counts": report["focus_class_counts"],
        "focus_heap_call_files": report["focus_heap_call_files"],
        "pure_core_source_policy": report["pure_core_source_policy"],
        "assembly_probe": assembly,
        "claim_allowed": False,
    }, indent=2, sort_keys=True))

    errors = pure_errors + assembly_errors
    for err in errors:
        print(f"ERROR: {err}", file=sys.stderr)
    if args.strict and errors:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
