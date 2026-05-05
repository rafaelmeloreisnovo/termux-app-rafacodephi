# RMR Pure Core Report

## Q16 sqrt(3)/2 constant decision (2026-05-05)

Objective selftest compares absolute error against `sqrt(3)/2` quantized to Q16.16:

- `0x0000DDB3`
- `0x0000DDB4`

Criterion: choose the value with minimum absolute error in Q16 units.

Result:

- Reference Q16 (`round((sqrt(3)/2)*65536)`) = `0x0000DDB4`.
- `abs_err(0x0000DDB3) = 1`
- `abs_err(0x0000DDB4) = 0`

Decision:

- Official single macro is `RMR_Q16_SQRT3_2` in `rmr/include/rmr_hex_const.h`.
- Value fixed to `0x0000DDB4`.
- Silent coexistence of both values is disallowed.

Validation entry point: `tools/rmr_pure_core_selftest.c`.
