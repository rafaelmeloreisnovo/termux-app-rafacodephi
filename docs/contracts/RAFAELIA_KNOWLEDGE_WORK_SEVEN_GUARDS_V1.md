# RAFAELIA — Seven Knowledge/Work Guards — Termux App V1.1

Repository role: **Android runtime provider**.

Canonical source: `rafaelmeloreisnovo/Mapa@3a2821d44d555c28cd041ba029cfa940fe3d4c0a`,
`docs/canonical/2026-09-13/CASA_CONHECIMENTO_TRABALHO_V1.md`.

Sequence:

`provenance -> context -> evidence -> contradiction -> uncertainty -> reproduction -> rollback`.

Reconstructibility remains transverse through the exact canonical `reconstruction_pointer`.

## V1.1 logic exam / semantic hardening

The V1 structural gate was valid but accepted several semantically malformed values because presence was checked more strongly than domain membership. V1.1 closes that gap without changing repository authority.

Fail-closed additions:
- GitHub provenance uses an exact 40-hex commit, with a digest-shaped object hash;
- observation time must be ISO-like;
- evidence types use a closed vocabulary;
- contradiction, uncertainty, reproduction and rollback states use closed vocabularies;
- `reproduction=PASS` requires evidence of TEST/RUN/RECEIPT/TEST_FIXTURE/CI class;
- the reconstruction pointer must equal the pinned Mapa contract;
- unknown states are errors, not silent readiness;
- material mutation still requires rollback `READY` or `EXECUTED`.

## Termux-specific boundary

CI/build evidence is not physical Android runtime evidence. APK presence is not installation or launch. Provider dispatch is not terminal execution. Discovery is not QEMU/guest execution. `armeabi-v7a` remains supported. `TOKEN_VAZIO != 0 != PASS`. Structural readiness never promotes a domain claim; `claim_allowed=false`.

The validator returns only `BLOCKED` or `READY_FOR_DOMAIN_REVIEW`.
A material mutation requires rollback state `READY` or `EXECUTED`.

## Structural validity is not readiness

`structural_status=PASS` means the envelope is complete enough to evaluate. It does not mean that its domain, integration, or physical-runtime claim passed.

The synthetic positive fixture remains a validator self-test:

```sh
python3 scripts/validate_knowledge_work_seven_guards.py \
  examples/knowledge-work-seven-guards.example.json
```

The versioned handoff snapshot is expected to remain blocked while its named contradictions and uncertainties are open:

```sh
python3 scripts/validate_knowledge_work_seven_guards.py \
  examples/knowledge-work-seven-guards.handoff-20260913.json \
  --expect BLOCKED
```

That command exits zero only when the structure is valid **and** the fail-closed decision is exactly `BLOCKED`. Structural errors still exit nonzero. Changing the expected decision never suppresses a validator error and never changes `claim_allowed=false`.

## Current bounded handoff

`TERMUX-PACKAGES-TO-APP-20260913-001` binds this consumer adapter to exact GitHub commits, blobs, CI runs, open contradictions, uncertainty probes, reproduction scope, and a non-history-rewriting rollback procedure. Its current status is intentionally `BLOCKED`; package consumption and physical Android runtime remain `TOKEN_VAZIO`.
