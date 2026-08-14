# OMEGA GAP DELTA — Cross-Domain Execution 01 — 2026-08-14

- parent: `docs/audit/OMEGA_GAP_DELTA_20260814_CROSS_DOMAIN_ANDROID.md`
- mode: `APPEND_ONLY | EVIDENCE_FIRST | FAIL_CLOSED`
- claim_allowed: `false`
- release: `false`

## Materialized after the parent delta

1. `tests/fixtures/cross_domain_dependency_graph.valid.json`
   - canonical governed fixture for the current RMX3834 C.74 anchor and F.94 declared target.
   - physical F.94 provenance remains `TOKEN_VAZIO`.

2. `tests/fixtures/cross_domain_dependency_graph.invalid_promotion.json`
   - adversarial fixture that attempts `promote_allowed=true` while `runtime_evidence=TOKEN_VAZIO` and declares an `EXECUTED` lifecycle without `OBSERVED_RUNTIME` provenance.
   - expected result: fail-closed rejection.

3. `tools/validate_cross_domain_dependency_graph.py`
   - checks exact top-level contract shape, typed node/edge/gap enums, provenance, uniqueness, endpoint integrity, lifecycle/runtime evidence coupling, CLOSED-gap evidence, and promotion gates.
   - runtime-like lifecycle states require `OBSERVED_RUNTIME` provenance.
   - `promote_allowed=true` requires every promotion gate to be `PASS`.
   - optional full JSON Schema validation runs when `jsonschema` is available; mandatory manual fail-closed checks require only Python stdlib.

4. `.github/workflows/cross-domain-contract-gate.yml`
   - adds a PR/workflow-dispatch gate to run self-test and validate the canonical fixture.
   - emits machine-readable reports when steps execute.

## CI observation

Workflow `Cross-Domain Contract Gate`, run `31768839501`, was created for branch head `f74b5d9a2e3609869c05c8c2624b0d1ec582c0dd`.

### Attempt 1
- job `94670296247`
- conclusion: `failure`
- observed steps: `0`
- log fetch: `HTTP_404_BLOB_NOT_FOUND`

### Attempt 2 — explicit rerun
- rerun request: accepted
- job `94670370584`
- conclusion: `failure`
- observed steps: `0`

Classification:

`REPEATED_CI_STARTUP_OR_RUNNER_FAILURE_BEFORE_OBSERVABLE_STEPS`

Therefore:

`CI_FAILURE_WITH_ZERO_STEPS != VALIDATOR_LOGIC_FAILURE`

and also:

`VALIDATOR_REMOTE_EXECUTION = TOKEN_VAZIO`

The negative result is preserved rather than hidden. It does not promote either PASS or content failure.

Machine-readable observations:
- `data/evidence/github/cross-domain-contract-ci-observation-20260814.v1.json`
- `data/evidence/github/cross-domain-contract-ci-observation-20260814.v2.json`

## Gap-state delta

- `GAP-XDOM-002`: `PARTIAL_SCHEMA -> PARTIAL_SCHEMA_FIXTURES_VALIDATOR_CI_DEFINED`.
- `GAP-XDOM-004`: `OPEN -> PARTIAL_GATE_MATERIALIZED`; a fail-closed promotion rule is now executable in code, but its remote execution is not yet proven.
- `TOKEN_VAZIO_RUNTIME`: preserved because both GitHub CI attempts produced zero observable steps.
- `GAP-RMX3834-001`: unchanged; F.94 physical device receipt remains absent from the audited evidence.

## Next verifiable gate

Run the exact branch validator in an observable execution environment and preserve:

- immutable branch/head SHA;
- validator file hash;
- schema + fixture hashes;
- exit codes;
- stdout/stderr hashes;
- generated JSON report hashes.

Only after an actual validator step executes may `GAP-XDOM-002` move beyond PARTIAL execution state.
