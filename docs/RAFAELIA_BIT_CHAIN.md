# RAFAELIA Bit Chain

## BitRAF
- Oficial: `app/src/main/cpp/lowlevel/raf_bitraf.h/.c`.
- Funções: encode/decode/validate/to_string (status IMPLEMENTADO/PARCIAL conforme cobertura de testes).

## BitStack
- Origem: `rmr/Rrr/rafaelia_core.c` e correlatos.
- Parâmetros citados: `N_STACKS=1000`, `N_EXTRA=8`, `N_TOTAL=1008`, XOR/paridade/CRC.
- Status: SOLTO/Rrr (não integrado ao build oficial principal).

## ZipRAF
- Busca no código oficial: sem implementação operacional confirmada.
- Status: AUSENTE/HIPÓTESE.

## BitOmega
- Busca no código oficial: sem implementação operacional confirmada.
- Status: AUSENTE/HIPÓTESE.

## CRC chain / commit gate / seal
- Caminho oficial: `rafaelia_commit_gate_ll.c` + CRCs em low-level.
- Artefatos benchmark/device ainda necessários para comprovar claims de performance/robustez.
