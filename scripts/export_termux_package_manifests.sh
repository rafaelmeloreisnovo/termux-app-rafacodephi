#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$ROOT_DIR/rafaelia/termux-packages-manifests"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

UPSTREAM_URL="${1:-https://github.com/termux/termux-packages.git}"
LIMIT="${2:-700}"

printf '[export] clone %s\n' "$UPSTREAM_URL"
git clone --depth 1 "$UPSTREAM_URL" "$TMP_DIR/termux-packages" >/dev/null 2>&1

printf '[export] gerar manifests deterministicos (limit=%s)\n' "$LIMIT"
python - "$TMP_DIR/termux-packages" "$OUT_DIR" "$LIMIT" "$UPSTREAM_URL" <<'PY'
import sys
from pathlib import Path

repo = Path(sys.argv[1])
out = Path(sys.argv[2])
limit = int(sys.argv[3])
upstream_url = sys.argv[4]

pkg_dir = repo / "packages"
pkgs = sorted([p for p in pkg_dir.iterdir() if p.is_dir() and (p / "build.sh").exists()], key=lambda p: p.name)
if limit <= 0 or limit > len(pkgs):
    limit = len(pkgs)
sel = pkgs[:limit]

def fnv1a32(data: bytes) -> int:
    h = 2166136261
    for b in data:
        h ^= b
        h = (h * 16777619) & 0xFFFFFFFF
    return h

out.mkdir(parents=True, exist_ok=True)
for f in out.glob('*.rafpkg'):
    f.unlink()

for p in sel:
    name = p.name
    sh = p / 'build.sh'
    raw = sh.read_text(encoding='utf-8', errors='ignore').splitlines()
    version = ''
    homepage = ''
    desc = ''
    for ln in raw:
        t = ln.strip()
        if t.startswith('TERMUX_PKG_VERSION=') and not version:
            version = t.split('=',1)[1].strip().strip('"\'')
        elif t.startswith('TERMUX_PKG_HOMEPAGE=') and not homepage:
            homepage = t.split('=',1)[1].strip().strip('"\'')
        elif t.startswith('TERMUX_PKG_DESCRIPTION=') and not desc:
            desc = t.split('=',1)[1].strip().strip('"\'')
    hid = fnv1a32(name.encode('utf-8'))
    lines = [
        'seal=RAFPKG',
        f'name={name}',
        f'id=0x{hid:08x}',
        f'name_len={len(name)}',
        f'version={version}',
        f'homepage={homepage}',
        f'description={desc}',
        'source=termux/termux-packages',
        f'path=packages/{name}/build.sh',
    ]
    (out / f'{name}.rafpkg').write_text('\n'.join(lines) + '\n', encoding='utf-8')

index = [
    'seal=RAFINDEX',
    f'upstream={upstream_url}',
    f'total_upstream_packages={len(pkgs)}',
    f'exported_packages={len(sel)}',
    'format=.rafpkg',
]
(out / 'INDEX.rafidx').write_text('\n'.join(index) + '\n', encoding='utf-8')
print(f'[export] upstream={len(pkgs)} exported={len(sel)} out={out}')
PY

printf '[export] pronto: %s\n' "$OUT_DIR"
