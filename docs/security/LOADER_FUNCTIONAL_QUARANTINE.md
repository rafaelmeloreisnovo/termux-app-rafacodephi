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

The current `master` loader is intentionally inert:

```text
android:hasCode=false
business Java sources=false
bootstrap payload absent
installer behavior absent
release_allowed=false
state=STUB_NO_BOOTSTRAP_PAYLOAD
```

This is safer than importing a functional branch that accepts an arbitrary
`target_dir`, extracts ZIP content or exposes an unauthenticated Activity.

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
functional_capability=false
release_allowed=false
claim_allowed=false
```

## Supersession rule

A divergent historical PR must not be merged merely because it contains useful
files. Its security-relevant design may be reimplemented over current `master`
only after this quarantine gate accepts the complete state and all independent
artifact/device gates pass.
