# RAFAELIA VCPU CLOCK

- vCPU count: 8 (`RAF_VCPU`).
- Clock lógico configurável por `target_hz`.
- 10Hz => 100,000,000 ns.
- 10kHz => 100,000 ns.
- jitter e delta são medidos por CLOCK_MONOTONIC.
- sem claim de computação quântica real; apenas mecanismo clássico quantum-inspired.
