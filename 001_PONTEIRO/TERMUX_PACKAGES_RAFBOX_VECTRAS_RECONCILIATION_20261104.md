# RAFAELIA Ω — Termux App / Packages / RAFBOX / Vectras reconciliation (2026-11-04)

State: DRAFT_HOLD, claim_allowed=false, release_allowed=false.
Authority: this repository owns Android host/bootstrap-import and freestanding RAFCODEPHI code; termux-packages owns source-built DEB/APT/bootstrap; Vectras-VM-Android owns consumer IPC/guest operation. No cross-repo auto-merge.

## Current verified sources (immutable refs)

- App master after merged PR519: `5fa40c2ab4e82142267b8d8d6a21168488fcb7c8`.
- Packages main after merged PR174 (Ninja) and PR179 (epoch transport): `b5499741aada8a779099b4fd7aa0fd5067169bb8`.
- Packages PR183 DRAFT candidate: `bcb1dfec1ecc72cd3acee80b5bf3706cead4849c` at source reconciliation; exact SHA, never resolve a moving branch implicitly.
- Packages run41764398128 built both ARM32/ARM64 sources successfully, but downstream matrix custody FAIL: `ARCHIVE_MISSING_OR_DUPLICATE=arm` (job126385293719). TAR and SHA missing from *upload paths*, not proof of bad ARM code.
- PR183 branch hotfix adds sibling `rafcodephi-bootstrap-evidence.tar.gz` and `.sha256` to the success upload and negative regression. New exact-head fan-in result: TOKEN_VAZIO until independently witnessed.
- App PR520 RAFBOX C11/no-libc authorial subset and Vectras PR1171 APT→PRoot→Ninja→QEMU are DRAFT/HOLD. Their runtime is not proven by source presence.

## Component and dependency boundary

| Node | Owner | Runtime mode | Status/gate |
|---|---|---|---|
| RAFBOX bounded name/path parser | Termux App PR520 | original C11 pure core, no libc/heap/OS calls | source in draft; no installation proof |
| RAFBOX ARMv7/AArch64 syscall adapter | Termux App PR520 | OS syscall PLATFORM_GATE, not pure core | cross-QEMU-user gate previously scoped PASS; physical Android TOKEN_VAZIO |
| Existing freestanding prefix/ELF probes | App and Packages producers | C/ASM with compiler/linker at build time, no general libc at runtime where built freestanding | scoped source gates, not full stack |
| APT, dpkg, Ninja, PRoot, QEMU | Packages and upstream components | external projects with original licenses and platform dependencies | do not relabel as original or as dependency-free |
| Android App UI/IPC/ART/DEX/permissions | Termux App and Vectras | mandatory Android platform APIs | permission and device transaction evidence TOKEN_VAZIO |
| Vectras guest boot | Vectras consumer | PRoot/QEMU userspace emulation; hardware hypervisor is not implied | TOKEN_VAZIO |

## Release-state gate (fail closed)

`SOURCE ≠ ARTIFACT ≠ EXECUTION ≠ EVIDENCE ≠ CLAIM`

1. Source pin: contract `data/contracts/termux-packages-rafcodephi-pin.v1.json` must resolve canonical and candidate to distinct exact 40-hex SHAs; merged PR174 cannot remain active candidate; past pins retained in `pin_history`.
2. Producer: source build for both ARM ABIs must pass, with explicit SHA-256 per TAR and invariant manifest; the *downloaded* TAR and its digest must pass matrix reconciliation. A successful build job alone is not enough.
3. Import: app verifies exact bootstrap ZIP contents, Ninja ELF architecture, full custom prefix, dpkg inventory and producer manifest before embedding APK.
4. Device: install exact APK; get permission and persisted IPC transaction receipt before executing APT, PRoot, Ninja and QEMU in that order. `apt-get --version` is a smoke, not signed-repository verification.
5. Guest: independent boot marker plus runtime provenance and rollback observation. Missing evidence = TOKEN_VAZIO, not PASS.

## Short rollback and next action

Rollback: close this unmerged App draft PR; close packages PR183 if its source correction regresses; preserve receipts; avoid force pushes, secret changes, provider-protection bypasses and automatic release.

Next: verify terminal exact-head packages PR183 matrix custody and first failure; only then update App candidate SHA again if producer moves and attempt the paired APK source build. Merge/release requires independent review and physical Android run. C11 pure core may remain authorial; Android/OS/third-party tools remain deliberately isolated.

R3=<App source pins reconciled in review branch; producer failure localized and patched in review branch,exact-head custody and device proof TOKEN_VAZIO,verify producer matrix then import APK and physically attest transaction+rollback>.
