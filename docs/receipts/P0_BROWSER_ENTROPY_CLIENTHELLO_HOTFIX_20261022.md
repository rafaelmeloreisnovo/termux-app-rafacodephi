# P0 hotfix receipt — BrowserRaf entropy + ClientHello (2026-10-22)

## Decision boundary
- State: IMPLEMENTED_UNTESTED_CI / DRAFT_ONLY / NO_MERGE.
- Producer: `rafaelmeloreisnovo/termux-app-rafacodephi`; branch: `hotfix/p0-browser-entropy-clienthello-20261022`; base: `master`.
- Incident: PR #503 merged at `cf7f65912d4b31d01f6c5306a0205704f2e33a38` with defensive `getrandom`, ABI pointer width and ClientHello host/cap limits. Subsequent PR #500 merged at `bca1f47b6ec83d307777fb2fb309719b00653dd6` and removed those 165 lines from the materializer. Regression was verified by exact old/current blob readback.
- Restored materializer exact prior blob: `7ce579996878430b739bf2c0203ccc470b233caa`; current regressed blob: `1f4ce8d89125d5ee1b028eb7ad4815856c8909ca`.
- Preservation: original `Browser.sh` untouched, original header literal length corrections Accept=46, Accept-Language=42 untouched, HTTPS->HTTP fail-closed policy untouched. No package, ABI, signing, license or external dependency changes.

## Scope and checks
- Restore source-bound `br_entropy.h` generation using Linux kernel `getrandom` and fail-closed short/error paths; reject historical deterministic `0xDEADBEEF` and `PRNG` from the generated browser.
- Restore bounded ClientHello source transformation, failed entropy / invalid length aborts, and ABI pointer-width guard.
- Extend host-only `validate_browser_fail_closed.py` to require entropy and ClientHello controls; extend `test_browser_materializer_falsifiers.py` to reject source marker drift for seven source-bound anchors and to assert the generated script's safety markers.
- Preflight source/hash readback: PASS_SCOPED (GitHub blob identity and markers); Python hosted CI: TOKEN_VAZIO_NOT_YET_EXECUTED at receipt creation; generated C compiler / platform runtime: TOKEN_VAZIO; physical Android: NOT_RUN; independent security review: TOKEN_VAZIO.
- The raw historical `Browser.sh` keeps its insecure prototype text only as input to the bounded materializer. Direct historical generator execution MUST NOT be promoted as protected TLS.
- `TLS_READY=false`, `claim_allowed=false`, `release_allowed=false`.

## Gate, stop and rollback
- CI: Rafaelia Native Safety workflow must run both the validator and unittest on the PR exact head. All named jobs verified before promotion.
- Negative controls: absent source markers, unsafe/deterministic TLS entropy, malformed ClientHello host or capacity, HTTPS fallback, incorrect header lengths. The current tests are **source/host checks**, not cryptographic or device conformance.
- STOP if provider reports job FAIL/UNOBSERVED, if base SHA drifts, if unsafe generated output is observed, or if a reviewer disputes the transformation.
- Rollback: close this PR (master unchanged while in review); if ever merged, revert exact hotfix commit using governed successor and record readback.
- NEXT: inspect PR CI logs and generated-script output, independent review, then separately verify actual ARM32/ARM64 binary and device behavior.

SOURCE ≠ ARTIFACT ≠ EXECUTION ≠ EVIDENCE ≠ CLAIM; IMPLEMENTED_UNTESTED ≠ PASS; TOKEN_VAZIO ≠ 0.
