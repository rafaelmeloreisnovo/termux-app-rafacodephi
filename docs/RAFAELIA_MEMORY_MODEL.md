# RAFAELIA MEMORY MODEL

## L1 lógico
`STATE_CAP=64`, `raf_state_t`, vCPU alinhado, `phase`, `step`, `crc`, `coherence`, `entropy`, `s[7]`.

## L2 lógico
`IN_BUF=64KB`, `OUT_BUF=64KB`, arena JNI, arena baremetal nomalloc, BitRAF, GP MVP, buffers benchmark.

## Buffer
`DirectByteBuffer` é ponte Java/C; não usar `ByteBuffer.wrap` no hot path. Todo acesso com tamanho externo deve validar capacidade real.

## RAM
Heap Java, APK, arquivos Termux, bootstrap, logs e artifacts de benchmark.

## Hardware
`page_size`, `cache_line`, ABI e recursos SIMD (NEON/SSE/AVX) quando disponíveis.
