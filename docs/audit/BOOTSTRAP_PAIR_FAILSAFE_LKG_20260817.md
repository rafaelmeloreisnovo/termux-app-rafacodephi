# Bootstrap Pair Fail-safe / Rollback / LKG — 2026-08-17

## Invariant

ARM32 and AArch64 are one logical bootstrap transaction. Downstream APK build may consume only:

1. `PRIMARY_COMMITTED`: both primary archives strictly validated, installed, hash-bound and post-validated; or
2. `FAILOVER_LKG`: the primary transaction failed, the pre-state was rolled back, and an independent LKG ARM32+AArch64+manifest set was strictly revalidated and installed.

Any partial, mixed, unvalidated or rollback-failed state is `BLOCKED`.

## Fail-safe behavior

- Both primary archives are validated before the first destination mutation.
- Existing destination pair is snapshotted before commit.
- A failure after ARM32 replacement restores the complete pre-transaction pair.
- LKG failover is allowed only as a complete ARM32+AArch64+manifest set.
- LKG is subjected to the same strict real-pkg importer contract as primary.
- Bridge and legacy-prefix payloads are never eligible as LKG.
- If LKG is absent, the valid state is `TOKEN_VAZIO`; rollback still executes and CI blocks rather than inventing a fallback.
- Transaction state is persisted in `build/reports/bootstrap-pair-transaction.json`.

## CI resilience

- `scripts/test_bootstrap_pair_transaction.py` deterministically proves primary commit, partial-primary rollback, and LKG failover.
- A strictly validated primary pair is staged as the next LKG and cached for subsequent runs.
- The beta artifact now includes the two bootstrap ZIPs plus the provenance manifest, not only reports/logs.
- Bootstrap resolution is explicit: `PRIMARY -> FAILOVER_LKG -> BLOCKED`.
- Final CI distinguishes `GREEN_PRIMARY` from `GREEN_DEGRADED`; failover is never mislabeled as primary success.

## Evidence boundary

`claim_allowed_device_runtime=false` and `device_runtime_proof=TOKEN_VAZIO` remain unchanged until the exact APK is exercised on physical Android with retained runtime provenance.
