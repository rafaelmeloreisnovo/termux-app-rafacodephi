# Provider Protection — Enterprise Runbook V2

## Purpose

This runbook closes the boundary between repository policy-as-code and the live
GitHub ruleset protecting `master`.

The canonical chain is:

```text
PROVIDER_RULESET_TARGET.v2.json
        ↓ desired policy
provider_protection_contract.py
        ↓ evaluate live provider state
provider-protection-gate.yml
        ↓ publish receipt before fail-closed exit
00_START_HERE.yml
        ↓
10_PROVIDER or 09_ENTERPRISE
```

The chain deliberately preserves:

`TARGET_FILE != LIVE_PROVIDER_STATE != OBSERVATION_RECEIPT != CLAIM`.

## Authority

| Layer | Authority |
|---|---|
| desired provider policy | `governance/provider/PROVIDER_RULESET_TARGET.v2.json` |
| historical 2026-08-31 observation | `governance/provider/PROVIDER_RULESET_TARGET_20260831.v1.json` |
| evaluator | `scripts/ci/provider_protection_contract.py` |
| offline regression vectors | `tests/test_provider_protection_contract.py` |
| live execution | `.github/workflows/provider-protection-gate.yml` |
| human router | `.github/workflows/00_START_HERE.yml` |
| evidence | `provider-protection-receipt-<run>-<attempt>` artifact |

V1 is intentionally retained because it contains historical observation data.
It is not the current desired-state source.

## Current live gap observed on 2026-09-26

GitHub ruleset `21908888` is active on the default branch and currently exposes:

- rules: `deletion`, `non_fast_forward`, `required_signatures`,
  `pull_request`, `code_quality`, `required_linear_history`;
- no `required_status_checks` rule;
- pull-request policy with `require_code_owner_review=true`;
- merge methods `merge,squash,rebase`;
- four `always` integration bypass actors:
  `20150`, `29110`, `73253`, `1144995`.

The V2 target requires:

- `required_status_checks`;
- `strict_required_status_checks_policy=true`;
- required context `provider-protection`;
- `require_code_owner_review=false` until an independently governed CODEOWNERS
  path exists;
- review-thread resolution enabled;
- allowed merge methods `squash,rebase`;
- every always-bypass integration explicitly identified and justified, or removed.

Therefore current provider state is expected to remain **FAIL** until the live
ruleset is changed.

## Why bypass is blocking

An always-bypass integration can cross a rule that is otherwise described as
required. An unidentified bypass therefore cannot be represented as PASS.

V2 uses:

`TOKEN_VAZIO_PENDING_IDENTITY_AND_JUSTIFICATION`

for unresolved bypass identity and the provider gate fails closed when such an
actor is observed.

Adding an actor ID to `justified_integration_ids` is not a cosmetic fix. A
future justification must include the integration identity, operational need,
scope, owner, revocation path, and evidence that the bypass is necessary.

## Administrative delta required

The evaluator is read-only. It does **not** mutate GitHub administration state.

The provider administrator must reconcile the live default-branch ruleset with
the V2 target. The minimum current delta is:

1. add `required_status_checks`;
2. require context `provider-protection`;
3. enable strict required-status-check policy;
4. align the pull-request parameters with the V2 target;
5. remove `merge` if the V2 merge-method policy remains authoritative;
6. identify and justify or remove each always-bypass integration;
7. rerun `Actions → 00 START HERE — RAFCODEΦ Enterprise → 10_PROVIDER`;
8. only accept closure when the published V2 receipt has `gate=PASS`.

If the live policy should intentionally differ, update the target through a
reviewable repository delta **before** changing the provider. Do not change the
evaluator merely to fit the live state.

## Receipt contract

The evaluator publishes JSON and Markdown receipts containing:

- target path + SHA-256;
- live ruleset IDs and rule types;
- canonical live-state digest SHA-256;
- pull-request-policy comparison;
- status-check comparison;
- always-bypass comparison;
- structured remediation operations;
- observation timestamp;
- provider observation ID;
- GitHub SHA/run/attempt/event;
- `claim_allowed=false`.

A failed run must still publish its receipt.

## Failure semantics

| State | Meaning |
|---|---|
| `PASS` | all provider structural gates in the current target matched |
| `FAIL` | one or more required provider conditions mismatched |
| `TOKEN_VAZIO` | evidence or identity is absent/unknown; never implicit PASS |
| evaluator/network error | FAIL; observation failure is not provider proof |

Even a V2 provider PASS does not prove Android runtime, APK behavior, physical
benchmarking, or release quality. It proves only the provider-protection scope.

## Rollback

Repository-side evaluator changes are ordinary Git commits and can be reverted.

Provider-side administrative rollback must preserve the previous ruleset
snapshot/receipt and must not remove protection merely to make CI green.

## R3

`F_ok`: target desired state is separated from historical observation; evaluator
is reusable/testable; failure produces evidence; bypass is explicit.

`F_gap`: current GitHub ruleset still requires an administrative change outside
the repository write surface.

`F_next`: apply the provider-side ruleset delta under authorized administration,
then execute route `10_PROVIDER` and retain the exact PASS receipt.
