# RAFAELIA Benchmark Taxonomy

## 1) storage_io
- read/write MB/s, random 4K IOPS, fsync latency.
- status típico: SCRIPT_ONLY/NEEDS_DEVICE.

## 2) memory_compute
- memcpy, memset, CRC, EMA, bandwidth, cache-resident ops.

## 3) logical_instruction
- BitRAF ops/sec, vCPU step/sec, macro-op throughput.

## 4) scheduler
- target_hz, actual_hz_q16, jitter_ns, missed_ticks.

## 5) binary/system
- APK size, .so size, page size, cache line, ABI, RSS.

## Status permitidos
DOC_ONLY, SCRIPT_ONLY, MEASURED_LOCAL, CI_ARTIFACT, DEVICE_ARTIFACT, NEEDS_DEVICE, NEEDS_BENCHMARK, INVALIDATED.
