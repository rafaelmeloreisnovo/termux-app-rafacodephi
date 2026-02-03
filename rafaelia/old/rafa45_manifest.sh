#!/usr/bin/env bash
set -euo pipefail

DAYS="${1:-45}"
OUTDIR="${2:-out_rafa45}"
ROOT="${3:-.}"

mkdir -p "$OUTDIR"
python3 - <<'PY' "$DAYS" "$OUTDIR" "$ROOT"
import os, sys, re, subprocess, time, pathlib, math
from datetime import datetime, timedelta

DAYS = int(sys.argv[1])
OUTDIR = pathlib.Path(sys.argv[2]).resolve()
ROOT = pathlib.Path(sys.argv[3]).resolve()

OUTDIR.mkdir(parents=True, exist_ok=True)

# Extensões alvo
EXTS = {
  "C": {".c",".h"},
  "RUST": {".rs"},
  "PY": {".py"},
  "ASM": {".s",".S",".asm"},
}
ALL_EXTS = set().union(*EXTS.values())

# Heurísticas simples (sem “pseudo”, só sinais)
PATTERNS = {
  "TODO_FIXME": re.compile(r"\b(TODO|FIXME|HACK|XXX)\b"),
  "UNSAFE_RUST": re.compile(r"\bunsafe\b"),
  "FFI": re.compile(r"\bextern\s+\"C\"|\bffi\b|\bdlopen\b|\bdlsym\b"),
  "SYSCALLS": re.compile(r"\b(syscall|ioctl|ptrace|mmap|mprotect|seccomp)\b"),
  "CRYPTO": re.compile(r"\b(AES|SHA|BLAKE|RSA|EC(DH|DSA)|HMAC|PBKDF|scrypt|argon2)\b", re.I),
  "NETWORK": re.compile(r"\b(socket|connect|send|recv|curl|wget|http|tls|ssl)\b", re.I),
  "ASM_HINT": re.compile(r"\b(__asm__|asm\b|\.global|\.text)\b"),
  "DANGEROUS_C": re.compile(r"\b(strcpy|strcat|gets|sprintf)\b"),
}

def run(cmd, cwd=None):
  try:
    return subprocess.check_output(cmd, cwd=cwd, stderr=subprocess.STDOUT, text=True)
  except Exception:
    return ""

now = datetime.now()
cutoff = now - timedelta(days=DAYS)

def file_mtime(p: pathlib.Path):
  try:
    return datetime.fromtimestamp(p.stat().st_mtime)
  except Exception:
    return datetime.fromtimestamp(0)

# Se for repo git, lista arquivos mudados nos últimos DAYS com maior precisão
is_git = (ROOT / ".git").exists()
git_files = set()
if is_git:
  # pega commits desde cutoff e lista paths tocados
  since = cutoff.strftime("%Y-%m-%d")
  log = run(["git","log",f"--since={since}","--name-only","--pretty=format:"], cwd=str(ROOT))
  for line in log.splitlines():
    line=line.strip()
    if not line: 
      continue
    p = (ROOT / line).resolve()
    if p.exists() and p.is_file():
      if p.suffix in ALL_EXTS:
        git_files.add(p)

def scan_file(p: pathlib.Path):
  try:
    data = p.read_text(errors="ignore")
  except Exception:
    data = ""
  lines = data.splitlines()
  size = p.stat().st_size if p.exists() else 0
  loc = len(lines)
  hits = {}
  for k, rgx in PATTERNS.items():
    hits[k] = len(rgx.findall(data))
  # complexidade superficial: conta keywords (C/Rust/Py)
  kw = {
    "if": len(re.findall(r"\bif\b", data)),
    "for": len(re.findall(r"\bfor\b", data)),
    "while": len(re.findall(r"\bwhile\b", data)),
    "match": len(re.findall(r"\bmatch\b", data)),
    "unsafe": hits["UNSAFE_RUST"],
  }
  # score objetivo: maior = “mais crítico/arriscado/complexo”
  # (não é “juízo moral”; é só triagem)
  score = 0.0
  score += min(size/1024/50, 10)                 # tamanho (até 10)
  score += min(loc/300, 10)                      # LOC (até 10)
  score += hits["TODO_FIXME"] * 0.8
  score += hits["UNSAFE_RUST"] * 2.0
  score += hits["DANGEROUS_C"] * 1.2
  score += hits["SYSCALLS"] * 1.5
  score += hits["NETWORK"] * 1.0
  score += hits["FFI"] * 1.3
  score += hits["ASM_HINT"] * 1.4
  score += hits["CRYPTO"] * 0.6
  # um “boost” se for asm puro
  if p.suffix in {".s",".S",".asm"}:
    score += 2.0
  return {
    "path": str(p.relative_to(ROOT)),
    "mtime": file_mtime(p),
    "size": size,
    "loc": loc,
    "hits": hits,
    "kw": kw,
    "score": score,
    "head": "\n".join(lines[:60]),
    "tail": "\n".join(lines[-40:]) if loc>40 else "",
  }

# Escolha do conjunto de arquivos
candidates = set()
if git_files:
  candidates |= git_files
else:
  # fallback: mtime
  for p in ROOT.rglob("*"):
    if not p.is_file():
      continue
    if p.suffix in ALL_EXTS:
      if file_mtime(p) >= cutoff:
        candidates.add(p.resolve())

items = [scan_file(p) for p in sorted(candidates)]

# Agrupa por linguagem
def lang_of(path):
  suf = pathlib.Path(path).suffix
  for k, s in EXTS.items():
    if suf in s:
      return k
  return "OTHER"

for it in items:
  it["lang"] = lang_of(it["path"])

# Ordena “da pior → melhor” = score desc, depois mtime desc
items.sort(key=lambda x: (x["score"], x["mtime"]), reverse=True)

# Escreve manifesto master
master = OUTDIR / "MANIFESTO_45D.txt"
with master.open("w", encoding="utf-8") as f:
  f.write("RAFA45 :: MANIFESTO (últimos %d dias)\n" % DAYS)
  f.write("Root: %s\n" % ROOT)
  f.write("Generated: %s\n" % now.strftime("%Y-%m-%d %H:%M:%S"))
  f.write("Files: %d\n" % len(items))
  f.write("="*78 + "\n\n")

  # índice rápido
  f.write("[ÍNDICE - ordenado por SCORE desc]\n")
  for i,it in enumerate(items,1):
    f.write("%3d) %-5s score=%6.2f loc=%6d size=%8dB mtime=%s  %s\n" % (
      i, it["lang"], it["score"], it["loc"], it["size"],
      it["mtime"].strftime("%Y-%m-%d"), it["path"]
    ))
  f.write("\n" + "="*78 + "\n")

  # seção detalhada por arquivo com cabeçalho
  for i,it in enumerate(items,1):
    f.write("\n" + "#"*78 + "\n")
    f.write("FILE %03d | %s\n" % (i, it["path"]))
    f.write("LANG=%s | SCORE=%.2f | LOC=%d | SIZE=%dB | MTIME=%s\n" % (
      it["lang"], it["score"], it["loc"], it["size"],
      it["mtime"].strftime("%Y-%m-%d %H:%M:%S")
    ))
    f.write("HITS=" + " ".join([f"{k}:{v}" for k,v in it["hits"].items() if v]) + "\n")
    f.write("-"*78 + "\n")
    f.write("[HEAD 60]\n")
    f.write(it["head"] + "\n")
    f.write("-"*78 + "\n")
    f.write("[TAIL 40]\n")
    f.write(it["tail"] + "\n")
    f.write("#"*78 + "\n")

# Também gera manifestos separados por linguagem (melhor pra anexar em partes)
bylang = {}
for it in items:
  bylang.setdefault(it["lang"], []).append(it)

for lg, arr in bylang.items():
  p = OUTDIR / f"MANIFESTO_{lg}.txt"
  with p.open("w", encoding="utf-8") as f:
    f.write(f"RAFA45 :: {lg}\nFiles: {len(arr)}\n\n")
    for i,it in enumerate(arr,1):
      f.write("\n" + "#"*78 + "\n")
      f.write(f"FILE {i:03d} | {it['path']}\n")
      f.write(f"SCORE={it['score']:.2f} LOC={it['loc']} SIZE={it['size']} MTIME={it['mtime']}\n")
      f.write("HITS=" + " ".join([f"{k}:{v}" for k,v in it["hits"].items() if v]) + "\n")
      f.write("-"*78 + "\n")
      f.write(it["head"] + "\n")
      f.write("-"*78 + "\n")
      f.write(it["tail"] + "\n")
      f.write("#"*78 + "\n")

print("OK:", master)
PY

echo "[OK] Gerado em: $OUTDIR"
echo "[DICA] Agora compacte e anexe aqui: zip -r ${OUTDIR}.zip $OUTDIR"
