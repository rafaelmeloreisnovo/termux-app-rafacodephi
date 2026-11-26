# Termux App — Signed APT bootstrap handoff to real pkg/apt

## What is implemented as source candidate

`scripts/import_rafcodephi_real_bootstrap.py` retains the historical **blocked** APT bootstrap; default `BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED` still requires disabled source + pre-invoke deny and does not pretend `pkg update` is functional.

For signed mode declared in both ZIP and manifest by the *separately verified producer*, the importer also accepts `SIGNED_REPOSITORY_CONFIGURED`, only when all agree:
1. Exact canonical package/prefix and ARM/AArch64 ELF checks, per-architecture ZIP SHA256 and API receiver contracts (existing guards, unchanged).
2. HTTPS APT URI with no credentials, query, fragment, injection or custom port; archive `termux.sources` has `Enabled: yes`, exactly expected stable/main/arm+aarch64 `Signed-By` key path and no `Trusted: yes`.
3. Legacy APT blocking pre-invoke directive is not active.
4. Public archive key exists inside ZIP and hashes to **independent** build-time `RAF_APT_TRUSTED_KEY_SHA256` (64 hex characters). This value must be supplied by an explicitly verified human/provider key custodian, never inferred from the same untrusted ZIP/manifest.
5. `BOOTSTRAP_INFO`, JSON profile and release manifest consistently declare repository URL, signed trust and inactive guard. Cross-architecture transaction rejects mixed modes. The receipts use measured values, not a hardcoded blocked state.

The owner-operated source-only CI `.github/workflows/rafcodephi-signed-apt-import-contract.yml` invokes `rafaelmeloreisnovo/actions/rafcode/verify-source@7c03495...` and positive/negative host fixtures; no automatic publication, device APT command, release, firewall or network permissions.

## CI/device authority

`pkg`, `apt`, `dpkg` binaries inside ZIP ≠ a working APT repository. The package source currently requires canonical prefix `/data/data/com.termux.rafacodephi/files/usr`; never point it at binaries built for `com.termux`. The producer's signed APT channel and hash-bound archives have not yet been published/installed under observed evidence. Until signed content is produced and the independent trust pin is supplied, the import stays **BLOCKED**, as intended.

After an authorized and published signed repository, test `apt update`, `apt-cache policy`, `pkg install nano/python/git`, `dpkg -V` and dependency resolution on actual ARM32/ARM64 Android devices. Capture ABI, SDK, package identity, APK/ZIP SHA, Release signature, exit codes and logs (sanitize device private data). Do **not** automatically run privileged package installation or alter the user's phone from hosted CI.

Licenses, authorship and third-party attributions preserved without modifications; no claim of third-party runtime originality, accreditation, certification or complete bare metal.

## R3

F_ok = source integration for signed mode, independent pinned key check and dual-ABI transactional receipt; F_gap = publisher absent, NDK toolchain pending, no real apt on device; F_next = exact-head tests and signed repository build/publication under explicit authority, then hardware test. `claim_allowed=false`, `promotion=HOLD`. Rollback = close unmerged DRAFT branch/revert delta only, preserve predecessor SHA.
