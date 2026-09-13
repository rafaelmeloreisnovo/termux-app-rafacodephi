# Seven Guards Logic/Structure Exam — Termux App — 2026-09-13

state: SOURCE_HARDENING_CANDIDATE
claim_allowed: false

## Exam
V1 was tested against malformed-but-complete inputs.

Observed bypass classes before hardening:
1. unknown contradiction state accepted;
2. unknown uncertainty state accepted;
3. unknown rollback state accepted when mutation=false;
4. arbitrary evidence type accepted;
5. malformed provenance object hash accepted;
6. GitHub branch-like ref accepted instead of exact commit;
7. arbitrary reconstruction pointer accepted.

## Development response
V1.1 adds closed vocabularies, exact GitHub commit validation, digest shape validation, canonical reconstruction pointer validation, and a minimum coupling between reproduction PASS and reproduction-class evidence.

## Boundary
This is a structural/semantic validator hardening. It does not prove APK build/install/device runtime and does not promote scientific or runtime claims.

## R3
F_ok: bypasses identified with falsifying inputs; hardening encoded in validator/tests.
F_gap: remote CI for this candidate and physical Android evidence.
F_next: run existing Safety Gates CI; fix only candidate-caused regressions; keep inherited lowlevel include-policy gap separate.
