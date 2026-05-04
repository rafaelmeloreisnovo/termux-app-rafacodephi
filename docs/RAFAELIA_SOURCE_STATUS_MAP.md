# RAFAELIA Source Status Map

## OFICIAL_BUILD
- `app/src/main/cpp/Android.mk` + fontes referenciadas (`termux-bootstrap-zip.S`, `termux-bootstrap.c`, `lowlevel/*.c`, `lowlevel/baremetal_asm.S`, `raf_*`).
- `rmr/src/main/cpp/rmr.c`.
- Pontos S: S1=OFICIAL/IMPLEMENTADO; S3=SYNCED parcial; S11=código+build.

## OFICIAL_JAVA_API
- `rmr/src/main/java/com/termux/rmr/RmrCore.java`.
- `rafaelia/src/main/java/com/termux/rafaelia/RafaeliaCore.java`.
- `rafaelia/src/main/java/com/termux/rafaelia/runtime/*`.
- Pontos S: S1=OFICIAL/PARCIAL; S3=CODE_AHEAD_DOC em partes.

## SOLTO_Rrr
- `rmr/Rrr/rafaelia_types.h`
- `rmr/Rrr/rafaelia_arena.h`
- `rmr/Rrr/rafaelia_core.c`
- `rmr/Rrr/rafaelia_b1.S`
- Pontos S: S1=SOLTO/Rrr+EXPERIMENTAL; S3=DOC_AHEAD_CODE (núcleo oficial não usa direto).

## MVP
- `mvp/rafaelia_mvp_puro.s`
- `mvp/rafaelia_opcodes.hex`
- Pontos S: S1=MVP; S3=DOC_AHEAD_CODE; S12=HIPÓTESE/EXPERIMENTAL.

## OLD_LEGACY
- `rafaelia/old/*`
- Pontos S: S1=LEGADO; S3=CODE_AHEAD_DOC local; S9=não usar em trilha oficial.

## DOC_ONLY
- ZipRAF/BitOmega (quando não localizados em código operacional oficial).
- MissHit como taxonomia formal (sem módulo executável dedicado atual).

## BENCH_ONLY
- Documentos/scripts de benchmark fora da integração CI artifact obrigatória (ex.: partes de `BENCHMARKS_COMPARISON.md`, `docs/RAFAELIA_TOP42_BENCHMARKS.md` sem artefato vinculado).
