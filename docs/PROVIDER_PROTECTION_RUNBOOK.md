# Provider Protection — Enterprise Runbook V3

## Purpose

Close the boundary between repository policy-as-code and the live GitHub ruleset
protecting `master`.

```text
PROVIDER_RULESET_TARGET.v3.json
        ↓ desired policy
provider_protection_contract.py
        ↓ live provider observation + version-bound witness
provider-protection-gate.yml
        ↓ receipt published before fail-closed exit
00_START_HERE.yml
        ↓
10_PROVIDER or 09_ENTERPRISE
```

`TARGET != LIVE_PROVIDER_STATE != WITNESS != RECEIPT != CLAIM`.

## Authority

| Layer | Authority |
|---|---|
| desired provider policy | `governance/provider/PROVIDER_RULESET_TARGET.v3.json` |
| V2 historical policy | `governance/provider/PROVIDER_RULESET_TARGET.v2.json` |
| current external bypass witness | `governance/provider/PROVIDER_RULESET_EXTERNAL_WITNESS_20260926.v2.json` |
| administrative mutation plan | `governance/provider/PROVIDER_RULESET_ADMIN_DELTA_20260926.v1.json` |
| evaluator | `scripts/ci/provider_protection_contract.py` |
| regression vectors | `tests/test_provider_protection_contract.py` |
| live workflow | `.github/workflows/provider-protection-gate.yml` |
| human router | `.github/workflows/00_START_HERE.yml` |

Historical targets and witnesses remain append-only evidence and are not silently
rewritten into the current policy.

## V3 change: bypass identity is typed

V2 modeled only Integration IDs. The current provider read exposes an additional
always-bypass actor:

- `RepositoryRole:5`;
- `Integration:20150`;
- `Integration:29110`;
- `Integration:73253`;
- `Integration:1144995`.

The live observation also reports `current_user_can_bypass=always`.

V3 therefore defines bypass identity as `(actor_type, actor_id)`. An always-bypass
actor of any type must be explicitly identified and justified or removed. Unknown
or stale visibility is never promoted to PASS.

## Version-bound witness

The current witness is bound to:

- ruleset ID `21908888`;
- ruleset updated instant `2026-09-26T20:05:43.907-03:00`;
- default branch condition `~DEFAULT_BRANCH`.

The evaluator compares ISO-8601 instants rather than timestamp strings. If the
ruleset ID or update instant changes, the witness becomes stale and the gate emits
`BYPASS_VISIBILITY_UNPROVEN` / `TOKEN_VAZIO_STALE_OR_UNMATCHED_WITNESS`.

This prevents both false absence and permanent stale evidence.

## Current provider gaps

At the bound provider version, ruleset `21908888` still has:

- no `required_status_checks` rule;
- pull-request policy with `require_code_owner_review=true`;
- merge methods `merge,squash,rebase` rather than target `squash,rebase`;
- five unreviewed always-bypass actors listed above.

V3 requires:

- strict required status checks;
- required context `provider-protection`;
- pull-request parameters aligned with the V3 target;
- every always-bypass actor justified or removed.

Provider state therefore remains **FAIL** until administration is reconciled.

## Administrative delta

The evaluator is intentionally read-only. The machine-readable administrative
plan is fail-stale: it may be applied only if the provider still reports the exact
ruleset ID and `updated_at` recorded in its precondition.

Minimum plan:

1. add strict `required_status_checks`;
2. require `provider-protection`;
3. align pull-request policy and merge methods;
4. identify and justify or remove `RepositoryRole:5`;
5. identify and justify or remove each Integration bypass actor;
6. execute `00 START HERE → 10_PROVIDER`;
7. accept closure only when the exact receipt has `gate=PASS`.

If the precondition no longer matches, abort and generate a successor observation
and plan. Do not force an old plan onto a changed provider state.

## Receipt semantics

A failed provider gate still publishes JSON and Markdown evidence before the final
non-zero exit. V3 receipts include:

- target path/schema/SHA-256;
- witness path/schema/SHA-256/binding;
- live ruleset IDs and canonical digest;
- observed rule types;
- every typed always-bypass actor;
- integration-ID compatibility projection;
- `current_user_can_bypass`;
- structured differences and remediation operations;
- workflow SHA/run/attempt/event;
- `claim_allowed=false`.

## Boundary

Provider protection PASS proves only the configured GitHub provider scope. It does
not prove Android runtime, APK behavior, physical benchmarking, scientific claims,
legal identity, or release fitness.

## R3

`F_ok`: V3 closes the actor-class blind spot and keeps desired policy, observation,
witness, mutation plan and receipt separate.

`F_gap`: live GitHub administration remains outside the current repository-write
connector and the provider is still nonconformant.

`F_next`: authorized provider administration applies a non-stale delta, then route
`10_PROVIDER` must produce an exact PASS receipt.
