# RAFCODEΦ: causal hotfix receipt — GitHub Actions run 37660128059

- Date: 2026-10-07
- Producer: [termux-packages](https://github.com/rafaelmeloreisnovo/termux-packages)
- Consumer: [termux-app-rafacodephi](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi)
- Observed job: [112925303754](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions/runs/37660128059/job/112925303754)
- Observed consumer SHA: `afcb6167cae754cfbd25ff845a4dd2f7e26ffc2e`
- Observed producer candidate SHA: `1e63d81ddc71d5b3ff7730f201a8cd70979ca218`
- Next exact producer candidate SHA: `97cd0fd25b194b3d857b3f6a2ac330ff0efb003c`
- Release: BLOCKED; claim_allowed=false; physical_android=TOKEN_VAZIO.

## Evidence: two distinct failures

1. Source-build step 7 stopped on `cannot seal profile; missing installed entries: bin/sh,libexec/termux-api`. The pinned producer code compared raw `SYMLINKS.txt` destination strings without normalizing `./`; it did not contain the later source-evidence-backed repair step. This is a concrete candidate cause, not physical execution evidence.
2. Step 15 failed independently in `actions/upload-artifact` because a preserved DEB path contained `ca-certificates-java_1:2026.05.14_all.deb`: the colon is disallowed in uploaded **path names**, not inside a TAR payload. This is a custody transport error, not a package compiler error.

Step 14 fail-closed evidence was emitted; later APK/receipt stages were skipped. Missing downstream outputs are consequences, not positive tests.

## Bounded intervention

- Candidate pin now resolves to the exact current merged producer source SHA, **without promoting canonical** and without claiming device execution. Historical pin stays recoverable in this receipt.
- Cheap producer symlink-repair regression fixture runs before the expensive source build, and source-gate requires the repair file to exist and be referenced by the producer.
- Per-architecture DEB directories are verified against their `SHA256SUMS` and wrapped as portable deterministic TAR files, retaining original `.deb` filenames and metadata; SHA-256 of each TAR accompanies the artifact.
- Upload includes ZIPs, manifest TXT and repair JSON at the top level; the unsafe raw `debs/` tree is not supplied directly to `upload-artifact`.
- No package content is renamed, no signature or license is modified, and no release gate is bypassed.

## Evidence still required

- App branch CI, including preflight repair test: NOT_RUN pending workflow.
- Real ARM32 and ARM64 source-build of **new exact pin**: NOT_RUN pending workflow.
- APK matrix, device install/repair, runtime, `apt/pkg/dpkg` and physical Android: TOKEN_VAZIO.
- Artifact readback: verify TAR SHA-256 and embedded `SHA256SUMS` in CI output before custodian promotion.
- Existing `canonical` pin remains unchanged.

Route: `SOURCE (producer SHA) → ARTIFACT (per-arch ZIP/DEB TAR) → EXECUTION (CI) → EVIDENCE (receipts + hashes) → CLAIM (blocked until gates)`.

R3 = <F_ok: exact failure fingerprint + reversible source/transport changes, F_gap: new CI and device proof, F_next: validate PR, then run only failing expensive candidate gate and inspect custody readback>.
