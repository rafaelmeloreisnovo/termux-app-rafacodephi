# RAFAELIA HZ AS MEMORY

## Hz físico
Frequência de CPU e cpufreq descrevem clock de hardware; não são equivalentes ao ritmo lógico do motor.

## Hz lógico
Campos operacionais: `target_hz`, `period_ns`, `actual_delta_ns`, `actual_hz_q16`, `jitter_ns`, `missed_ticks`.

## Hz como memória
Cada tick preserva estado anterior e acumula `phase`, `step`, `crc`, `coherence`, `entropy`. `jitter_ns` mede ruído teórico/prático. `missed_ticks` indica falha de agendamento/capacidade.

## Relação L1/L2/RAM
- L1: estado vCPU (`phase`,`step`,`crc`,`s[7]`).
- L2: buffers JNI/DirectByteBuffer/arenas/telemetria.
- RAM: logs, artifacts de benchmark, arquivos Termux.

## Ruído vs erro
- Ruído: diferença mensurável entre `target_hz` e `actual_hz_q16`.
- Erro: violação de contrato (overflow, corrida, sem lock, claim sem prova).
