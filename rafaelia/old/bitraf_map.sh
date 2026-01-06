#!/data/data/com.termux/files/usr/bin/bash

# BITRAF_MAP — mapa de commits BitRAF em Markdown
#  - Le RAFAELIA_LOGS/contextos/bitraf_commits.log
#  - Extrai timestamp e checksum de cada commit
#  - Gera RAFAELIA_LOGS/maps/bitraf_map.md

ROOT="$HOME/RAFAELIA_LOGS"
LOGDIR="$ROOT/contextos"
MAPDIR="$ROOT/maps"

BITRAF_LOG="$LOGDIR/bitraf_commits.log"
MAP_FILE="$MAPDIR/bitraf_map.md"

mkdir -p "$LOGDIR" "$MAPDIR"

if [ ! -f "$BITRAF_LOG" ]; then
  echo "# BitRAF Map — Linha do Tempo" > "$MAP_FILE"
  echo "" >> "$MAP_FILE"
  echo "Nenhum commit BitRAF encontrado." >> "$MAP_FILE"
  echo "Arquivo de log nao existe:" >> "$MAP_FILE"
  echo "$BITRAF_LOG" >> "$MAP_FILE"
  echo "[BITRAF_MAP] Nenhum log encontrado. Mapa vazio gerado em: $MAP_FILE"
  exit 0
fi

NOW="$(date)"

awk -v map_file="$MAP_FILE" -v now="$NOW" '
  BEGIN {
    commit = 0;
  }
  # Marca inicio de um commit
  /================ BITRAF COMMIT ================/ {
    commit++;
    ts = "";
    cs = "";
    next;
  }
  # Linha de timestamp
  /^timestamp:/ {
    sub(/^timestamp:[ ]*/, "", $0);
    ts = $0;
    timestamps[commit] = ts;
    next;
  }
  # Linha com checksum (gerada pelo binario rafaelia_bitraf_hash)
  /checksum[ ]*:/ {
    # Exemplo de linha: "  checksum : 123456 (soma de bytes mod 2^32)"
    cs = $0;
    # Pegar o numero logo apos ":"
    # split pela ":" e depois pegar o comeco da segunda parte
    n = split($0, parts, ":");
    if (n >= 2) {
      sub(/^[ \t]*/, "", parts[2]);
      # Pega primeiro token numerico da segunda parte
      split(parts[2], vals, " ");
      cs = vals[1];
    }
    checksums[commit] = cs;
    next;
  }
  END {
    # Cabecalho do mapa
    print "# BitRAF Map — Linha do Tempo" > map_file;
    print "" >> map_file;
    print "Atualizado em: " now >> map_file;
    print "" >> map_file;

    if (commit == 0) {
      print "Nenhum commit BitRAF encontrado em:" >> map_file;
      print "  " map_file >> map_file;
      next;
    }

    print "Total de commits BitRAF: " commit >> map_file;
    print "" >> map_file;
    print "| # | Timestamp | Checksum |" >> map_file;
    print "|---|-----------|----------|" >> map_file;

    for (i = 1; i <= commit; i++) {
      ts = (i in timestamps) ? timestamps[i] : "-";
      cs = (i in checksums)  ? checksums[i]  : "-";
      print "| " i " | " ts " | " cs " |" >> map_file;
    }
  }
' "$BITRAF_LOG"

echo "[BITRAF_MAP] Mapa BitRAF gerado em: $MAP_FILE"
