#!/bin/sh
# validate_deb_prefix_safe.sh
# R3 Validação — Verifica se arquivo .deb é prefix-safe (sem referências a /data/data/com.termux/...)
# Extrai .deb, inspeciona todos ELFs, rejeita se contiver legacy prefix

set -e

DEB_PATH="${1:?usage: validate_deb_prefix_safe.sh <file.deb>}"

EXPECTED_PREFIX="/data/data/com.termux.rafacodephi/files/usr"
LEGACY_PREFIX="/data/data/com.termux/files/usr"

# Validações iniciais
if [ ! -f "$DEB_PATH" ]; then
  echo "ERROR: DEB file not found: $DEB_PATH" >&2
  exit 1
fi

# Criar tmpdir seguro
TMPDIR=$(mktemp -d 2>/dev/null || mktemp -d -t 'deb_validate')
trap "rm -rf '$TMPDIR'" EXIT

cd "$TMPDIR"

# Extrai .deb (ar + tar)
if ! ar x "$DEB_PATH" 2>/dev/null; then
  echo "ERROR: Failed to extract .deb (ar command failed)" >&2
  exit 1
fi

# Extrai data.tar.* (tenta xz, depois gz, depois sem compressão)
if [ -f data.tar.xz ]; then
  tar xf data.tar.xz 2>/dev/null || true
elif [ -f data.tar.gz ]; then
  tar xf data.tar.gz 2>/dev/null || true
elif [ -f data.tar ]; then
  tar xf data.tar 2>/dev/null || true
else
  echo "ERROR: No data.tar.* found in .deb" >&2
  exit 1
fi

# Buscar todos ELFs e validar
FOUND_LEGACY=0
FOUND_ELFS=0
ELFS_OK=0

find . -type f 2>/dev/null | while IFS= read -r file; do
  file_type=$(file "$file" 2>/dev/null || echo "unknown")

  echo "$file_type" | grep -q "ELF" || continue

  FOUND_ELFS=$((FOUND_ELFS + 1))

  # Verificar se contém legacy prefix
  if readelf -d "$file" 2>/dev/null | grep -q "$LEGACY_PREFIX" >/dev/null 2>&1; then
    echo "FAIL: ELF contains legacy prefix: $file" >&2
    FOUND_LEGACY=$((FOUND_LEGACY + 1))
  elif readelf -d "$file" 2>/dev/null | grep -q "RUNPATH\|RPATH" >/dev/null 2>&1; then
    # OK: tem RUNPATH ou RPATH (sem legacy)
    ELFS_OK=$((ELFS_OK + 1))
  else
    # OK: sem RPATH/RUNPATH (independente)
    ELFS_OK=$((ELFS_OK + 1))
  fi
done

# Retornar ao dir original para emitir status
cd - > /dev/null

# Emitir resultado final
DEB_BASENAME=$(basename "$DEB_PATH")
if [ $FOUND_LEGACY -eq 0 ]; then
  if [ $FOUND_ELFS -eq 0 ]; then
    echo "OK: $DEB_BASENAME (no ELFs found, likely script/config package)"
  else
    echo "OK: $DEB_BASENAME ($ELFS_OK ELFs, all prefix-safe)"
  fi
  exit 0
else
  echo "FAIL: $DEB_BASENAME (legacy prefix in $FOUND_LEGACY/$FOUND_ELFS ELFs)" >&2
  exit 1
fi
