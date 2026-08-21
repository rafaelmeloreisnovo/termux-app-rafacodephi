# Security Summary Status V2 — 2026-08-21

Status: `SUPERSEDING_INTERPRETATION / TOKEN_VAZIO_VERIFICATION_REQUIRED`  
Claim gate: `claim_allowed=false`

## Why this successor exists

`SECURITY_SUMMARY.md` is preserved as historical provenance. Its broad conclusions such as `SECURE FOR PRODUCTION USE`, `Approved`, and checklist-wide claims must not be interpreted as current operating-effectiveness, certification, legal compliance, or physical-device security evidence.

This successor does **not** assert that the historical controls are false. It states that their current truth requires exact-head, exact-artifact and, where applicable, exact-device evidence.

```text
historical_documentation != current_execution
source_control_present != operating_effectiveness
static_test != physical_device_security
security_practice_reference != certification
privacy_intent != privacy_conformance
```

## Current high-priority security/privacy surfaces

### P0 — command and IPC authority

- `RUN_COMMAND`/provider execution is a privileged application capability even when protected by a custom permission.
- Discovery or dispatch acceptance does not prove authorization correctness, bounded command construction, exit status, QEMU execution or guest boot.
- Require negative tests for unauthorized caller, permission denial, malformed/replayed request, protected-option bypass, result-digest mismatch and safe-state behavior.

### P0 — execution-time identity

The current provenance work exposes discovery-time material identities. Preserve:

```text
discovery_identity != execution_time_identity
```

Before a runtime/security claim, bind request digest -> package/APK identity -> executable digest at execution -> result/exit -> device/environment receipt.

### P0 — private runtime residue

Treat device identifiers, user paths, environment values, logs, retrieved files, command lines and app/user payload as non-public until classified. Public receipts should prefer hashes, pseudonymous transaction IDs and bounded/redacted status.

### P0/P1 — broad Android permissions

`MANAGE_EXTERNAL_STORAGE`, `SYSTEM_ALERT_WINDOW`, battery-optimization exemptions and any exported component/permission path require exact manifest + runtime-use evidence, least-privilege justification and graceful denial behavior. A declaration alone is neither vulnerability nor proof of safety.

### P1 — native/bootstrap supply chain

Native/bootstrap binaries need pinned source/provenance, artifact hashes, dependency/vulnerability review and execution identity. A Java/Kotlin static-analysis result cannot silently cover opaque native binaries.

## Protected subject + dignity gate

Repository metadata does not establish that a user is a child, an adult, a member of a culture, a guardian or a vulnerable person.

```text
child_status_unknown != adult
age_threshold != cultural_context_resolved
group_label != cultural_meaning
cultural_reference_missing -> TOKEN_VAZIO_CONTEXT
guardian_role != automatically_valid_consent
```

If a feature actually establishes or potentially processes child/vulnerable-person data, elevate to P0 and resolve purpose, minimization, recipients, retention, transparency, authority/consent, alternatives, jurisdiction and best-interest review before promotion. Do not infer protected attributes from names, groups, geography, device data or usage patterns.

## Closure gates

A bounded security/privacy state may be promoted only with the receipts appropriate to its scope:

1. exact repo/ref/commit + manifest/permission inventory;
2. static and negative tests for the changed surface;
3. dependency/native/bootstrap provenance and hashes where relevant;
4. exact APK hash/signing identity;
5. install -> launch -> PID/activity -> bounded logcat/device evidence where runtime is claimed;
6. execution-time command/binary identity for provider/QEMU claims;
7. privacy review of logs/artifacts/retention/access/minimization;
8. independent/security/legal review when the claim requires it;
9. rollback reference for high-impact mutation.

Missing required evidence remains `TOKEN_VAZIO`; it is never converted to `0`, `false`, `PASS` or `NOT_APPLICABLE` without evidence.

## Historical claim handling

The original `SECURITY_SUMMARY.md` remains append-only history. This document supersedes only its **current interpretation**. Future evidence should append scoped receipts rather than rewrite the historical observation.

## Falsifier

A production/security/privacy promotion based only on the historical summary, without current scoped evidence for the claimed boundary, falsifies this successor contract.
