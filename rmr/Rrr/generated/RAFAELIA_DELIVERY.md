# RAFAELIA Delivery Notes (What knowledge was carried)

This repository delivery encodes the RAFAELIA principles as implementable software/firmware artifacts:

1. **Toroidal state model** `T^7` and state vector `(u,v,psi,chi,rho,delta,sigma)` are represented in fixed-point `Q16.16`.
2. **Canonical period 42** drives all loops and phase synchronization.
3. **7-directional evolution** is explicit in the simulator and tied to ISA opcodes.
4. **FORWARD recurrence** uses immutable constants (`SPIRAL_Q16=56755`, `PI_Q16=205887`) with Q16.16 term `3146`.
5. **EMA coherence/entropy update** follows `0.75/0.25`, and computes `phi=(1-H)*C` each cycle.
6. **Integrity path** includes global XOR and **CRC32C Castagnoli** checks during VERIFY gates.
7. **Commit gate** preserves snapshot/rollback semantics.
8. **Toroidal routing** uses stride-7 permutation to cover all oscillators.
9. **Fractal modulation** uses Mandelbrot field to perturb coupling and reduce local trapping.
10. **BitRAF 42-bit ISA** maps frequency/weight/phase/crc/load/opcode fields for hardware-facing control.

The intent is to keep deterministic execution, no heavy dependencies, immutable constants, and direct mapping from mathematical model to runtime behavior.
