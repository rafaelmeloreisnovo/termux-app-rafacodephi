# Loader functional quarantine

## Purpose

The loader may exist in exactly two acceptable states:

```text
STUB_SAFE_BLOCKED
or
FUNCTIONAL_SECURITY_GATED
```

There is no accepted partial state.

## Current state

The current loader implements the complete source-level security boundary:

```text
android:hasCode=true
loader extraction=false
HTTPS same-origin acquisition bounded to 128 MiB
signature-protected entry and read-only provider
host SHA-256+BLAKE3, bounded ZIP custody and atomic install
release_allowed=false
state=FUNCTIONAL_SECURITY_GATED
```

Artifact, matching-certificate and physical Android evidence remain separate
promotion gates; the structural pass does not promote them.

## Forbidden functional patterns

A future implementation fails when it contains or permits:

- `EXTRA_TARGET_DIR` or `target_dir`;
- loader-owned ZIP extraction;
- `ZipInputStream` or `ZipEntry` in the loader service;
- plaintext HTTP;
- implicit redirects;
- unbounded download;
- exported entry point without signature permission;
- writable or exported provider;
- source URL or hashes invented as defaults;
- host/loader signer mismatch;
- external handoff without host SHA-256 and BLAKE3;
- native inbox path without `O_NOFOLLOW`;
- native heap allocation in the bootstrap read path;
- documentation claiming functionality before artifact and device evidence.

## Required functional architecture

```text
host build pins URL + SHA-256 + BLAKE3
→ host verifies loader signer and contract version
→ signature-protected loader Activity
→ HTTPS/443 with same-origin redirects
→ bounded private download
→ loader SHA-256 + fsync
→ temporary read-only URI grant to host
→ host repeats SHA-256 and BLAKE3
→ host owns ZIP budgets and path policy
→ atomic private inbox
→ existing TermuxInstaller owns staging and rollback
```

The loader never receives a host directory and never installs directly into
`$PREFIX`.

## Gate

```bash
python3 tools/validate_loader_functional_security.py --strict --write-report
python3 -m unittest tests/test_loader_functional_security.py -v
```

The validator inspects the actual repository state. If no loader Java code is
present, it proves the inert stub boundary. If Java code appears, it switches
automatically to the complete functional checklist and rejects hybrid states.

## Evidence hierarchy

```text
source validator PASS
!= APK build PASS
!= matching signer receipt
!= unauthorized caller rejection on Android
!= URI grant/revocation observation
!= bootstrap installation and rollback
!= DEVICE_RECEIPT_COMPLETE
!= release approval
```

Current promotion boundary:

```text
functional_capability=true_at_source_boundary
apk_and_device_evidence=TOKEN_VAZIO
release_allowed=false
claim_allowed=false
```

## Supersession rule

A divergent historical PR must not be merged merely because it contains useful
files. Its security-relevant design may be reimplemented over current `master`
only after this quarantine gate accepts the complete state and all independent
artifact/device gates pass.
