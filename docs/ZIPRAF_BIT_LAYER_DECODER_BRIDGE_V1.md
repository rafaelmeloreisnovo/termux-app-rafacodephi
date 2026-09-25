# ZIPRAF Bit Layer Decoder Bridge V1

State: BRIDGE_SPEC / PHASE_A_RUNTIME_IMPLEMENTED_UNTESTED  
claim_allowed=false

## Existing producer evidence

- `docs/RAFAELIA_10X10X10_BIT_MATRIX.md`: bit_plane, bit_volume, bit_route, Q16, CRC.
- `rmr/Rrr/zipraf_index.c`: logical projection/index layer over unchanged physical ZIP bytes.

## Binding

```text
I_M = R(B,M,G(M))
```

The decoder must classify blocks as structural or non-structural before applying:

```text
Delta M => Delta G
```

A missing block, corrupt block and structural block are distinct states.

## Download semantics

Out-of-order arrival may change intermediate I(W,q,M), but a complete identical block set must converge to the same final state independent of arrival order.

F_next: derive the exact historical G(M) rule from authoritative decoder code; do not recreate it from imagery.


## Phase A runtime successor — 2026-09-25

Producer reference authority:
- `RafPolimata@d52afbc38acf6d9580b32cbf9f7f259fa4afdf4b`;
- vector Git blob `4c3ba2202afb6f62bb465d435273031ebb16c3b3`.

Termux now carries an independent geometry-free implementation in
`rmr/Rrr/zipraf_bit_layer_phase_a_v1.h`, plus a selftest that covers:
- producer witness bytes BLV-00/01/55/80/A5/FF;
- exhaustive bytes 0..255;
- q=1..8;
- complete out-of-order layer convergence;
- duplicate-conflict and incomplete fail-closed behavior.

The existing freestanding runtime workflow cross-builds ARMv7/AArch64 static ELFs and executes the selftest under QEMU user-mode.

This does not define `M` or `G(M)`. Emulated runtime evidence is not physical Android evidence.

F_next: exact-head dual-ABI gate; then implement the independent Vectra Phase-A adapter and compare both receipts before any T-BL-010 promotion.
