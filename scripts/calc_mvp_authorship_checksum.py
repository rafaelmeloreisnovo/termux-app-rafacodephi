#!/usr/bin/env python3
"""Calcula checksum XOR-32 do payload de mvp/rafaelia_opcodes.hex.

Regra:
- Escopo: bytes definidos em diretivas `db` do início do arquivo até ANTES
  do rótulo `AUTHORSHIP_CHECKSUM:`.
- Comentários (`; ...`) e tudo que não é `db` são ignorados.
- Agregação em grupos de 4 bytes (word little-endian), com zero-padding no
  último grupo incompleto.
- Checksum final = XOR de todas as words de 32 bits.
"""

from __future__ import annotations

import pathlib
import re
import sys


def parse_hex_byte(token: str) -> int:
    t = token.strip().lower()
    if not t:
        raise ValueError("token vazio")
    if t.endswith("h"):
        t = t[:-1]
    value = int(t, 16)
    if value < 0 or value > 0xFF:
        raise ValueError(f"byte fora do intervalo: {token}")
    return value


def collect_payload_bytes(text: str) -> list[int]:
    out: list[int] = []
    for line in text.splitlines():
        if re.match(r"\s*AUTHORSHIP_CHECKSUM\s*:", line):
            break

        code_only = line.split(";", 1)[0]
        match = re.search(r"\bdb\b(.*)", code_only, flags=re.IGNORECASE)
        if not match:
            continue

        for token in match.group(1).split(","):
            token = token.strip()
            if token:
                out.append(parse_hex_byte(token))

    return out


def xor32_groups_of_4(byte_values: list[int]) -> int:
    acc = 0
    for idx in range(0, len(byte_values), 4):
        b = byte_values[idx : idx + 4]
        while len(b) < 4:
            b.append(0)
        word = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)
        acc ^= word
    return acc


def main() -> int:
    target = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path("mvp/rafaelia_opcodes.hex")
    text = target.read_text(encoding="utf-8")
    payload = collect_payload_bytes(text)
    checksum = xor32_groups_of_4(payload)

    print(f"FILE={target}")
    print(f"PAYLOAD_BYTES={len(payload)}")
    print(f"CHECKSUM_XOR32_GROUP4=0x{checksum:08X}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
