#!/usr/bin/env python3
"""Generate a reproducible source-code inventory for the repository."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from collections import Counter, defaultdict

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "INVENTARIO_CODIGO_FONTE.md"

SOURCE_EXTS = {
    ".java": "Java",
    ".kt": "Kotlin",
    ".c": "C",
    ".cc": "C++",
    ".cpp": "C++",
    ".h": "C/C++ Header",
    ".hpp": "C/C++ Header",
    ".S": "Assembly",
    ".s": "Assembly",
    ".mk": "Make",
    ".py": "Python",
    ".sh": "Shell",
}

EXCLUDE_PARTS = {".git", ".gradle", "build", ".idea"}

CRITICAL_FILES = [
    "app/build.gradle",
    "app/src/main/cpp/Android.mk",
    "app/src/main/java/com/termux/lowlevel/BareMetal.java",
    "app/src/main/cpp/lowlevel/baremetal.c",
    "app/src/main/cpp/lowlevel/baremetal_jni.c",
    "app/src/main/cpp/lowlevel/baremetal_asm.S",
    "rafaelia/src/main/java/com/termux/rafaelia/AnovaResult.java",
]

EXPECTED_HELPERS = [
    "scripts/rafaelia_ci_smoke.sh",
    "rafaelia_env/tools/doctor.sh",
    "scripts/rafaelia_upstream_parallel_check.sh",
]

@dataclass
class FileStat:
    path: str
    lang: str
    size: int
    lines: int


def should_exclude(path: Path) -> bool:
    return any(part in EXCLUDE_PARTS for part in path.parts)


def line_count(path: Path) -> int:
    with path.open("rb") as f:
        return sum(chunk.count(b"\n") for chunk in iter(lambda: f.read(1024 * 1024), b""))


def main() -> None:
    stats: list[FileStat] = []
    by_lang_size = Counter()
    by_lang_files = Counter()
    by_lang_lines = Counter()
    modules = defaultdict(int)

    for p in ROOT.rglob("*"):
        if not p.is_file() or should_exclude(p):
            continue
        ext = p.suffix
        if ext not in SOURCE_EXTS:
            continue
        rel = p.relative_to(ROOT)
        lang = SOURCE_EXTS[ext]
        size = p.stat().st_size
        lines = line_count(p)
        stats.append(FileStat(str(rel), lang, size, lines))
        by_lang_size[lang] += size
        by_lang_files[lang] += 1
        by_lang_lines[lang] += lines
        top = rel.parts[0] if rel.parts else "(root)"
        modules[top] += 1

    total_size = sum(by_lang_size.values()) or 1
    active = [s for s in stats if not s.path.startswith("rafaelia/old/")]
    legacy = [s for s in stats if s.path.startswith("rafaelia/old/")]

    critical_rows = []
    for rel in CRITICAL_FILES:
        p = ROOT / rel
        exists = p.exists()
        size = p.stat().st_size if exists else 0
        critical_rows.append((rel, size, "✅" if exists else "❌"))

    missing_helpers = [p for p in EXPECTED_HELPERS if not (ROOT / p).exists()]

    lines = []
    lines.append("# 📂 INVENTÁRIO COMPLETO DE ARQUIVOS DE CÓDIGO FONTE\n")
    lines.append("_Gerado automaticamente por `scripts/generate_code_inventory.py`._\n")

    lines.append("## 🏗️ Estrutura Geral (por diretório de topo)\n")
    for mod, count in sorted(modules.items(), key=lambda x: (-x[1], x[0])):
        lines.append(f"- `{mod}/`: {count} arquivos de código")
    lines.append("")

    lines.append("## 📊 Resumo por linguagem\n")
    lines.append("| Linguagem | Arquivos | Linhas | Tamanho (bytes) | % tamanho |")
    lines.append("|---|---:|---:|---:|---:|")
    for lang, sz in sorted(by_lang_size.items(), key=lambda x: -x[1]):
        pct = 100 * sz / total_size
        lines.append(f"| {lang} | {by_lang_files[lang]} | {by_lang_lines[lang]} | {sz} | {pct:.1f}% |")
    lines.append("")

    lines.append("## 🔵 Código ativo vs legado\n")
    lines.append(f"- Ativo (fora de `rafaelia/old`): **{len(active)} arquivos**, **{sum(s.lines for s in active)} linhas**, **{sum(s.size for s in active)} bytes**")
    lines.append(f"- Legado (`rafaelia/old`): **{len(legacy)} arquivos**, **{sum(s.lines for s in legacy)} linhas**, **{sum(s.size for s in legacy)} bytes**")
    lines.append("")

    lines.append("## 🎯 Arquivos críticos\n")
    lines.append("| Arquivo | Tamanho (bytes) | Status |")
    lines.append("|---|---:|---:|")
    for rel, size, status in critical_rows:
        lines.append(f"| `{rel}` | {size} | {status} |")
    lines.append("")

    lines.append("## ⚠️ Itens de consistência\n")
    if missing_helpers:
        lines.append("### Scripts esperados e ausentes")
        for p in missing_helpers:
            lines.append(f"- ❌ `{p}`")
    else:
        lines.append("- ✅ Todos os scripts esperados existem.")

    empty_legacy = ROOT / "rafaelia/old/rafaelia_kernel.c"
    if empty_legacy.exists() and empty_legacy.stat().st_size <= 8:
        lines.append(f"- ⚠️ `{empty_legacy.relative_to(ROOT)}` está praticamente vazio ({empty_legacy.stat().st_size} bytes).")

    lines.append("")
    OUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
