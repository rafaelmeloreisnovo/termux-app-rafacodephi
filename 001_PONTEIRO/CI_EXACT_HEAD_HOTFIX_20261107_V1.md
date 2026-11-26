# RAFAELIA Ω — CI exact-HEAD causal hotfix (Termux App, 2026-11-07)
μID=MU-TERMUX-APP-EXACT-HEAD-20261107-V1
source=rafaelmeloreisnovo/termux-app-rafacodephi/pull/526 | parent=c54677b65ac0b0af16cf9de41c604a521234b24c | kind=CAUSAL_CI_HOTFIX+APPEND_ONLY_RECEIPT | routes=P,C,R,I,E
facts=At prior HEAD c54677b65ac0b0af16cf9de41c604a521234b24c, inline ASM run 42426806307 failed exit127 ('llvm-objdump: not found'); Action Reference Audit 42426806380 failed 2 checkout@v5 dated-policy violations; Native Safety 42426806303 failed due github_action_references condition. MicroASM v2 run 42426806364 SUCCESS_SCOPED. Provider Protection 42426806317 FAIL: NO_ACTIVE_DEFAULT_BRANCH_RULESET (independent admin gate, NOT waived).
delta=Exact PR HEAD checkout v7 with no persisted credentials; bounded timeouts L0/V2; resolve versioned LLVM objdump or install llvm-18 on Ubuntu 24.04 CI test runner; fail closed if unavailable; disassemble actual compiled ARM objects and keep negative vectors; do not waive provider or safety constraints.
gate=NEW_HEAD_CI:TOKEN_VAZIO_NOT_RUN; PROVIDER_RULESET:FAIL_NOT_FIXED; PHYSICAL_ARM32/64:TOKEN_VAZIO; BOOTSTRAP_APT_PKG:NOT_RUN; CLAIM_ALLOWED=false; RELEASE_ALLOWED=false
evidence=prior log runs 42426806307/42426806380/42426806303 and 42426806317; exact new HEAD results still required; file blob readback after mutation required.
rollback=revert single scoped PR-branch commit after review, preserve prior receipts, no force push or master mutation.
next=exact-head CI; retain provider gate blocked until authorized ruleset creation/readback; no release until producer/consumer physical evidence.
R3=<scoped tool/checkout/timeouts, new CI/provider/device gaps, inspect exact-head results and independent governance>
