# RAFAELIA MEMORY MODEL

## L1 / hot path
- `raf_state_t`: s[7], coherence, entropy, phase, step, crc.
- `STATE_CAP=64` no Java.
- `raf_vcpu_t` alinhado em 64 bytes para reduzir false sharing entre vCPU.

## L2 / warm path
- `IN_BUF` 64KB e `OUT_BUF` 64KB (DirectByteBuffer).
- `JNI_ARENA_SZ` 256KB.
- `BM_ARENA_SZ` 512KB no caminho nomalloc.
- matrizes/vetores/catálogo em baremetal.

## RAM / cold path
- Java heap do app, APK, bootstrap Termux, logs e arquivos de pacotes.

## Observações de cache
- cache line alvo: 64 bytes.
- stride por vCPU via struct alinhada.
- working set do estado lógico é pequeno e separado de buffers grandes.
- estado lógico não representa toda memória física do processo.
