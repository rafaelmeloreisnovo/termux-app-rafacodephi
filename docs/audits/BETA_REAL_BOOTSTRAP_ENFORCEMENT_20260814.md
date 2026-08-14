# RAFCODEΦ Beta — Real Bootstrap Enforcement — 2026-08-14

Status: `IMPLEMENTED_BRANCH / BUILD_EVIDENCE_PENDING`

## Invariant

`BRIDGE_BOOTSTRAP != REAL_PKG_BOOTSTRAP != INSTALLED_RUNTIME != PHYSICAL_RUNTIME_PROOF != RELEASE_CLAIM`

`claim_allowed=false` until a newly produced beta artifact is inspected and then exercised on physical Android.

## Observed negative baseline supplied from the device workflow

The supplied beta artifact bundle was frozen as a negative baseline, not promoted:

- bundle SHA-256: `917832ff345d4f8b621451d8db0442b6fcf410e769dcbb5952db8287c4dd37cb`
- beta evidence JSON SHA-256: `44e14b5c2c8de5f893c78f9a78f77068be0b1925576c986bac54f3fb2f23d499`
- screenshot SHA-256: `86b0938cb822c2f814b7b1d41811ece575f778579b3f48faf71f7c4787156e0e`
- signed ARM32 APK SHA-256: `07d7643820d00c03d04663a051a3d595a073e8dd92c13c86255fcc57a4bb957e`
- embedded ARM bootstrap blob SHA-256: `7c808bd77bb163baa4f6618340878651bd3d0560d328c4f34f86870222f1cf29`

Direct artifact inspection found:

- APK matrix diagnostic: `bootstrap_source: local`
- embedded bootstrap profile: `bridge`
- `package_layer: bridge`
- `bin/pkg`: script
- `bin/apt`: script
- `bin/apt-get`: script
- `bin/busybox`: script
- `bin/proot`: script
- `bin/dpkg`: absent
- `bin/bash`: absent

Therefore this artifact is a valid negative fixture for the new policy and is **not** the desired beta release state.

## Device-side blocker observed

The device had a canonical private runtime and multiple executable targets, but the bootstrap readiness receipt remained blocked by the profile contract. The concrete profile violation was `missing_required_entry_1`.

Root cause in source:

1. `tools/raf_bootstrap_profile.py` declared `SYMLINKS.txt` in `required_entries`.
2. `TermuxInstaller` consumes `SYMLINKS.txt` to materialize symlinks and intentionally does not copy that source manifest into the installed prefix.
3. The old `BootstrapReadinessGate` treated every archive `required_entry` as an installed-runtime file.
4. The installed runtime could therefore be blocked by a source-archive instruction that should not exist after installation.

The correction does **not** weaken readiness. `SYMLINKS.txt` is now classified as source-archive provenance while runtime requirements become stronger.

## Implemented beta contract

### Build path

`.github/workflows/beta-build.yml` now requires a source-built real pair from `rafaelmeloreisnovo/termux-packages`:

- ARM32: `arm`
- ARM64: `aarch64`
- `RAF_BOOTSTRAP_SOURCE=source-built-real`
- manifest schema `rafcodephi.real-bootstrap-sourcebuild/v1`
- `bridge_allowed=false`
- `legacy_prefix_allowed=false`

The pipeline fails closed if the pair cannot be built or imported.

Before APK generation, both embedded bootstrap profiles must be:

- `profile=real-pkg`
- `package_layer=real-pkg`

and `bin/apt` / `bin/apt-get` must classify as ELF.

### APK matrix

`scripts/build_apk_matrix.sh` accepts `source-built-real` as a first-class bootstrap source and no longer forces that source back to the local bridge route.

### Runtime readiness

`BootstrapReadinessGate` requires:

- profile and package layer `real-pkg`;
- package / prefix / architecture coherence;
- `runtime_materialized=true`;
- claim and release gates closed;
- `apt`, `apt-get`, `dpkg`, `bash`, `busybox`, `proot` executable ELF;
- non-empty `var/lib/dpkg/status`;
- an APT source definition;
- all true runtime required entries.

`SYMLINKS.txt` remains evidence of the source archive and is not incorrectly required in the installed prefix.

### Existing-beta migration

`BetaRealBootstrapRepair` provides a governed repair route for an already installed bridge beta:

1. validate the candidate archive as `real-pkg` **before** touching the old prefix;
2. move the old `$PREFIX` to an app-private sibling backup;
3. preserve `$HOME`;
4. call the existing `TermuxInstaller` for installation;
5. require the shared real-pkg readiness gate to pass;
6. delete the backup only after PASS;
7. if the installer returns but the strong gate remains blocked, quarantine the rejected prefix and restore the previous prefix.

The beta wizard now invokes this route with `Install / Repair Real Bootstrap`.

## Anti-regression CI

`.github/workflows/beta-real-bootstrap-contract.yml` runs a fast structural contract gate.

Observed on branch head family:

- workflow `beta-real-bootstrap-contract`
- run `31790767729`
- conclusion `SUCCESS`
- Python policy tests: PASS
- Python syntax: PASS
- shell syntax: PASS
- explicit local-bridge regression check: PASS

The heavyweight beta build is intentionally separate because it must source-build real ARM32 + ARM64 bootstraps.

## Open evidence / TOKEN_VAZIO

- `TV-BETA-REAL-BUILD-ARTIFACT`: inspect a newly generated signed beta and prove embedded ARM32/ARM64 profiles are `real-pkg` with real package binaries.
- `TV-BETA-REAL-BUILD-RECEIPTS`: freeze manifest, bootstrap hashes, APK hashes, signing evidence and build source commit from the completed real build.
- `TV-BETA-PHYSICAL-MIGRATION`: upgrade the currently observed bridge beta and execute `Install / Repair Real Bootstrap` on physical Android.
- `TV-BETA-PHYSICAL-PKG`: prove `dpkg`, `apt`, `pkg` behavior on-device rather than inferring from ELF presence.
- `TV-BETA-REPOSITORY-CONNECTIVITY`: prove configured repositories can actually update/install packages.
- `TV-BETA-ARM64-PHYSICAL`: physical ARM64 install/boot/runtime receipt.

## Current gate

`SOURCE-BUILT REAL BOOTSTRAP -> EMBEDDED PROFILE/ELF GATE -> APK BUILD/SIGN -> ARTIFACT INSPECTION -> GOVERNED MIGRATION -> PHYSICAL RUNTIME RECEIPT -> CLAIM GATE`

No bridge artifact may be renamed or reclassified as a real beta.
