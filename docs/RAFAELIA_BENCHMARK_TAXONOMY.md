# RAFAELIA Benchmark Taxonomy

- `storage_io`: MB/s, random, fsync, IOPS físicos.
- `memory_compute`: memcpy/memset/CRC/EMA/bandwidth/cache-resident.
- `logical_instruction`: BitRAF ops/s, vCPU step/s, macro-op throughput.
- `scheduler`: target_hz, actual_hz_q16, jitter_ns, missed_ticks.
- `binary_system`: APK/.so/page_size/cacheline/ABI/RSS.

## Status por métrica
DOC_ONLY | SCRIPT_ONLY | MEASURED_LOCAL | CI_ARTIFACT | DEVICE_ARTIFACT | NEEDS_DEVICE | NEEDS_BENCHMARK | INVALIDATED.

Regra: bilhões de ops/s sem I/O físico **não** são storage IOPS; classificar como logical_instruction/memory_compute.
