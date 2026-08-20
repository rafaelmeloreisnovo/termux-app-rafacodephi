# Android Compatibility Claim Boundary — 2026-08-20

Estado: `KNOWN_DEBT / NO_REGRESSION / claim_allowed=false`

## Observation

At commit `b6d516cda1c0780c0a06f1891aee7225726cceb3`, the root README contains four classes of wording stronger than the evidence boundary should permit without a device-matrix receipt:

- `fully compatible`;
- `will NOT crash`;
- `production-ready`;
- `Zero Collisions`.

They are recorded as **known debt**, not as validated production claims.

## Immediate invariant

```text
known debt may decrease
known debt must not increase
new absolute guarantees => FAIL
claim_allowed = false
```

The CI gate therefore prevents amplification while preserving the existing text for an explicit reviewed correction rather than silently rewriting history.

## Safe target language

Prefer bounded statements such as:

- Android 15/16 and 16 KiB page-size support is **configured/implemented**, with runtime compatibility limited to the tested device/kernel/build matrix.
- The patch is intended to prevent known alignment-related startup failures; untested combinations remain `TOKEN_VAZIO`.
- Side-by-side identity is structurally configured with a distinct package/authorities; collision-free behavior requires installation receipts.
- Production readiness requires a signed release, install/launch/logcat evidence, dependency/bootstrap gates and the declared release checklist.

## Next verifiable step

Patch the four legacy phrases in `README.md`, execute the claim-boundary gate, then reduce the corresponding `max_count` values to zero in the debt ledger. Only that two-step change closes the debt.
