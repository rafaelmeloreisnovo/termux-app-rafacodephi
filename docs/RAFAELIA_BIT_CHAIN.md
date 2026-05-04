# RAFAELIA Bit Chain

## BitRAF
- Oficial: `app/src/main/cpp/lowlevel/raf_bitraf.h/.c`.
- Estado: IMPLEMENTADO/PARCIAL (encode/decode/validate/to_string verificar cobertura por testes).

## BitStack
- Origem: `rmr/Rrr/rafaelia_core.c`.
- Valores: `N_STACKS=1000`, `N_EXTRA=8`, `N_TOTAL=1008`.
- Paridade XOR + CRC presentes no núcleo Rrr.
- Estado: SOLTO/Rrr (não oficial).

## ZipRAF
- Busca sem evidência de módulo executável oficial.
- Estado: AUSENTE/HIPÓTESE.

## BitOmega
- Busca sem evidência de módulo executável oficial.
- Estado: AUSENTE/HIPÓTESE.

## CRC/Commit/Seal
- CRC aparece em low-level e Rrr; commit gate oficial existe (`rafaelia_commit_gate_ll.c`), mas chain unificada ainda parcial.

## Pontos S
S3: BitRAF=SYNCED parcial; BitStack/ZipRAF/BitOmega=DOC_AHEAD_CODE.
S12: marcar ausências como HIPÓTESE sem claim.
