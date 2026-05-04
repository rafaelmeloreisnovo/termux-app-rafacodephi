# RAFAELIA VCPU CLOCK

- vCPU count: 8 (`RAF_VCPU`).
- Clock lógico configurável por `target_hz`.
- 10Hz => 100,000,000 ns.
- 10kHz => 100,000 ns.
- jitter e delta são medidos por CLOCK_MONOTONIC.
- sem claim de computação quântica real; apenas mecanismo clássico quantum-inspired.


## Threading contract (vNext)
Opção A adotada: `raf_vcpu.c` permanece single-thread interno; todo acesso externo é serializado pelo `g_sched_mutex` no JNI (`stepVcpuNative`, `stepAllVcpusNative`, `getClockProfileNative`, `getVcpuTelemetryNative`).
