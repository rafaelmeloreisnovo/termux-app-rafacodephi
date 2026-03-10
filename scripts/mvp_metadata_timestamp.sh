#!/usr/bin/env bash
set -euo pipefail

FILE="${2:-mvp/rafaelia_opcodes.hex}"
MODE="${1:-}"

if [[ -z "${MODE}" ]]; then
  echo "Uso: $0 <update|validate> [arquivo_hex]" >&2
  exit 2
fi

if [[ ! -f "${FILE}" ]]; then
  echo "Arquivo não encontrado: ${FILE}" >&2
  exit 2
fi

update_timestamp() {
  local epoch iso b0 b1 b2 b3 line
  epoch="$(date -u +%s)"
  iso="$(date -u -d "@${epoch}" +%Y-%m-%dT%H:%M:%SZ)"

  b0=$(printf '%02X' $(( epoch        & 0xFF )))
  b1=$(printf '%02X' $(( (epoch >> 8)  & 0xFF )))
  b2=$(printf '%02X' $(( (epoch >> 16) & 0xFF )))
  b3=$(printf '%02X' $(( (epoch >> 24) & 0xFF )))

  line="    db ${b0}h, ${b1}h, ${b2}h, ${b3}h  ; Timestamp Unix = ${epoch} (${iso}, build UTC)"

  python3 - "$FILE" "$line" <<'PY'
import pathlib
import re
import sys

path = pathlib.Path(sys.argv[1])
new_line = sys.argv[2]
text = path.read_text(encoding='utf-8')

pattern = re.compile(r'(^\s*; Timestamp Unix[^\n]*\n)\s*db[^\n]*$', re.MULTILINE)
if not pattern.search(text):
    raise SystemExit('Linha de Timestamp Unix não encontrada para atualização.')
text = pattern.sub(r'\1' + new_line, text, count=1)
path.write_text(text, encoding='utf-8')
PY

  echo "Timestamp atualizado: ${epoch} (${iso})"
}

validate_timestamp() {
  python3 - "$FILE" <<'PY'
import pathlib
import re
import sys

path = pathlib.Path(sys.argv[1])
text = path.read_text(encoding='utf-8')

m = re.search(r'METADATA:\n(?P<body>[\s\S]*?)\n\s*; Autor hash', text)
if not m:
    raise SystemExit('Seção METADATA não encontrada para validação.')

body = m.group('body')
if re.search(r';\s*Timestamp Unix[^\n]*\n\s*db\s+00h,\s*00h,\s*00h,\s*00h\b', body):
    raise SystemExit('ERRO: Timestamp Unix permanece zerado (00000000) em METADATA.')

if not re.search(r';\s*Timestamp Unix', body):
    raise SystemExit('ERRO: Campo Timestamp Unix não encontrado em METADATA.')

print('OK: Timestamp Unix em METADATA está preenchido.')
PY
}

case "${MODE}" in
  update)
    update_timestamp
    ;;
  validate)
    validate_timestamp
    ;;
  *)
    echo "Modo inválido: ${MODE}" >&2
    echo "Use: update | validate" >&2
    exit 2
    ;;
esac
