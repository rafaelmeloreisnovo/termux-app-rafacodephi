# BITRAF42 Format Authority V1

## Runtime authority

`BITRAF42-LOWLEVEL-V1` is the executable 42-bit layout used by
`app/src/main/cpp/lowlevel/raf_bitraf.c`, which is included in
`app/src/main/cpp/Android.mk` for `termux_rafaelia_direct`.

```text
41                    36 35   33 32              23 22          11 10       0
+-----------------------+-------+------------------+--------------+-----------+
| opcode:6              | dir:3 | layer:10         | imm:12       | flags:11  |
+-----------------------+-------+------------------+--------------+-----------+
```

Pure successor:
`rafaelia/src/main/cpp/zero/include/rafz_pure_bitraf42.h`.

## Separate experimental format

`rmr/Rrr/generated/rafaelia_isa_spec.md` and the 10×10×10+8 staging family
describe another 42-bit word as six fields of seven bits:

```text
freq7 | weight7 | phase7 | crc7 | load7 | op7
```

That is named here `BITRAF42-6X7-EXPERIMENTAL`.

The two encodings are **not binary-compatible**. Shared width `42` is not
evidence that fields, opcodes, CRC semantics, or execution contracts are the
same.

## Evidence status

- lowlevel encode/decode/validate exists in the Android build;
- `tools/rmr_pure_core_selftest.c` already has a hosted smoke roundtrip;
- the new ZERO leaf adds source-level no-control-flow constraints,
  equivalence vectors against the lowlevel implementation, and ARM assembly
  probing;
- the six-by-seven format remains separate and must receive its own producer,
  vectors and version before promotion.

Original contradiction:
`TOKEN_VAZIO_CONTRADICTION`.

Resolution for routing:
**name/version separation**, not silent conversion and not deletion.
