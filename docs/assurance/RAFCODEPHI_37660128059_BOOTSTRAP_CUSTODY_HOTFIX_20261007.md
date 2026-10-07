# RAFCODEΦ — job 112925303754 fail-closed bootstrap custody hotfix

- **Source of observation:** [workflow run 37660128059, job 112925303754](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions/runs/37660128059/job/112925303754)
- **Observation timestamp:** 2026-10-07. This is a historical failed run, not a new successful execution.
- **Parent app revision:** `f8d3bd78b63dfc8f8084bf3f0fc4e251bdbb750d`.
- **Producer previously pinned:** `1e63d81ddc71d5b3ff7730f201a8cd70979ca218` (merged [termux-packages #119](https://github.com/rafaelmeloreisnovo/termux-packages/pull/119)).
- **Proposed exact producer candidate:** `97cd0fd25b194b3d857b3f6a2ac330ff0efb003c`; 60 commits ahead of previous pin, 0 behind at source compare. The candidate is *not* runtime-promoted.
- **Merged producer fix lineage:** [#121](https://github.com/rafaelmeloreisnovo/termux-packages/pull/121) normalizes `./bin/sh` and `./libexec/termux-api`; [#135](https://github.com/rafaelmeloreisnovo/termux-packages/pull/135) addresses a separate APT-guard false positive.

## Two independently observed failures

1. **Step 7**: `cannot seal profile; missing installed entries: bin/sh,libexec/termux-api`, followed by `RAFCODEPHI_BOOTSTRAP_DOCKER=BLOCKED build_exit=1 copy_exit=0`. Historical producer #121 directly identifies a representation mismatch: `./path` versus `path`. Do not fabricate installed binaries or skip verification. The newer producer has an evidence-bound symlink repair path.
2. **Step 15**: `upload-artifact` rejects colon `:` inside `ca-certificates-java_1:2026.05.14_all.deb`. Broad selection of `artifacts/rafcodephi-bootstrap/` unintentionally included the entire original Debian package tree. This is a separate **custody upload** failure, not proof that compilation failed for the same reason.

## Scoped consumer changes

- Pin the **candidate** channel to the exact merged producer SHA above, preserving previous pin and `claim_allowed=false`.
- Upload the parent manifest, real bootstrap ZIP pair, symlink-repair receipts, per-ABI `SHA256SUMS`, and build reports. Avoid uploading unsanitized `*.deb` filenames with Debian epoch colons. Original `.deb` bytes are **not** contained in this bounded upload; checksum lists are evidence pointers, not independent binary-custody proof.
- Include a stdlib-only fast contract test for upload paths. The heavy workflow still depends on `contract-fast`; no `continue-on-error` or failure masking.

## Evidence boundary and rollback

**Known:** former job `contract-fast=PASS`; source build `FAIL`; downstream APK steps `SKIPPED`; original failed upload produced **zero** GitHub artifacts. The merged producer fixes and this consumer proposal are not equivalent to a successful combined execution.

**Required next receipt:** exact-head `contract-fast` PASS, then producer-pinned ARM and AArch64 ZIPs and immutable manifest hashes, validated embedded APK pair, usable-beta receipts; physical Android install/runtime and enforcement remain `TOKEN_VAZIO`.

**Rollback:** revert this consumer hotfix PR. The old candidate SHA is retained as `previous_candidate_commit` in the pin contract; no silent overwrite of the canonical channel.

`SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`
`IMPLEMENTED_UNTESTED != PASS`
