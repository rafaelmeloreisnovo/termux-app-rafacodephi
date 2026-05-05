# RMR Lowlevel Purity Audit

## Hex constant normalization (2026-05-05)

Normalization performed to remove divergent lowlevel literals for Q16 constants.

- Created single-source header: `rmr/include/rmr_hex_const.h`.
- Updated `rmr/Rrr/rafaelia_types.h` to consume only the official macro set.
- `Q16_SPIRAL` now aliases `RMR_Q16_SQRT3_2` (`0x0000DDB4`) exclusively.

Control:

- Objective decision test in `tools/rmr_pure_core_selftest.c` asserts the minimum absolute error criterion against Q16 reference of `sqrt(3)/2`.
- Any reintroduction of `0x0000DDB3` as alternative active constant violates this audit decision.
