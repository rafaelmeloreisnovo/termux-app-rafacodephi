# RAFAELIA Branchless Map

- `rmr/src/main/cpp/rmr.c`: **ARCH_SELECTED** (asm `rev` por arquitetura + fallback C).
- `app/src/main/cpp/lowlevel/baremetal_asm.S`: **BRANCH_REDUCED** (vetorização/ISA, não comprovado branchless total).
- `rmr/Rrr/rafaelia_core.c`: **HAS_BRANCH** (ifs/loops explícitos apesar de trechos aritméticos por máscara).
- `mvp/rafaelia_mvp_puro.s`: **DOC_ONLY/EXPERIMENTAL** sem integração para comprovar classificação operacional.
- Termo “branchless” literal: **ABSENT** na maior parte do código.

## Pontos S
S0 arquivos acima; S1 misto OFICIAL/SOLTO/MVP; S3 SYNCED parcial; S12 sem prova => PRECISA_MEDIÇÃO.
