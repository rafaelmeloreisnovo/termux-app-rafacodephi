# RMR Module

Módulo low-level em C/JNI para utilitários determinísticos usados pelo app.

## Escopo real no código

Implementação atual confirmada em:
- `rmr/src/main/java/com/termux/rmr/RmrCore.java`
- `rmr/src/main/cpp/rmr.c`
- `rmr/src/main/cpp/Android.mk`

## API exposta (RmrCore)

- `nativeNormalizeTag(String)`
- `nativeClamp(int, int, int)`
- `nativeStableHash(String)`
- `nativeTransmuteU32(int)`
- `nativeFlipInPlace(float[])`
- `normalizeTag(String)` (fallback Java)

## Observações de compatibilidade

- A biblioteca nativa carregada é `rmr` (`System.loadLibrary("rmr")`).
- O módulo é incluído no build via `settings.gradle` e `app/build.gradle` (`implementation project(":rmr")`).

## Auditoria desta estrutura

Consulte [AUDITORIA.md](./AUDITORIA.md) para achados de documentação e compatibilidade do módulo RMR.
