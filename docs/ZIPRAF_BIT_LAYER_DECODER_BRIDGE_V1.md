# ZIPRAF Bit Layer Decoder Bridge V1

State: BRIDGE_SPEC / IMPLEMENTATION_UNTESTED  
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
