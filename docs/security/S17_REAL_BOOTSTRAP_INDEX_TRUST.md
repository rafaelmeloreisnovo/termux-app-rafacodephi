# S17 — Authority boundary for real Termux apt/package index

This is an **additive, reversible security hotfix**. It protects the optional real-package bootstrap builder, not the C/ASM freestanding runtime.

## Source-first threat model
The original real-package builder verified the SHA-256 of each downloaded `.deb` against hashes read from the mutable remote `Packages` index. An attacker controlling that unsigned/unverified index could change both the package reference and its hash. S17 now **requires a separately authenticated SHA-256 pin for the exact raw `Packages` bytes** before dependency traversal or package downloads; `https` and approved repository host are mandatory.

The human/operator must independently establish each pin from an authenticated `InRelease`/signed repository metadata chain or an equivalently reviewed trust root. **S17 does not itself verify GPG signatures nor establish the operator's authority to supply the pin.** Until that external trust proof exists, `TOKEN_VAZIO_TRUST_ANCHOR`, and real-package bootstrap production remains blocked.

Required environment for an explicitly reviewed real-package build:
- `RAFCODEPHI_INDEX_SHA256_AARCH64` — exactly 64 hex characters, SHA256 of raw arm64 `Packages`.
- `RAFCODEPHI_INDEX_SHA256_ARM` — exactly 64 hex characters, SHA256 of raw ARM `Packages`.
- `RAFCODEPHI_APPROVED_REPO_HOST` defaults to `packages.termux.dev`; changing it needs explicit policy/rights review.
- `RAFCODEPHI_REAL_PKG_BOOTSTRAP=true` and `RAFCODEPHI_REAL_PKG_ARCH=all|aarch64|arm` choose the stage. Missing pin fails **before** generating any embedded ZIP.

A pin mismatch prevents dependency closure. Each individual `.deb` is still independently compared to the pinned index SHA256 list. Existing ZIP path safety and binary legacy-prefix validator remain independent gates.

## What is not proof
- A digest supplied by the same untrusted mirror is **not** independent authentication.
- A debug APK with `--prepare-dev` includes development wrappers; no real apt/pkg/proot runtime.
- C11 L0 no-libc and JNI module compile success do not imply real Android package execution.
- A CI green check is not provider required-check enforcement, rights authorization, release signing or physical-device proof.

## Reproduce without unsafe writes
`python3 -m unittest discover -s tests -p 'test_s17_pinned_packages_index.py' -v` and `python3 -O -m unittest discover -s tests -p 'test_s17_pinned_packages_index.py' -v`; `bash -n scripts/build_rafaelia_bootstraps.sh`. The independent negative shell preflight ensures absent pins produce no bootstraps.

## Acceptance and rollback
1. Human validates trusted source key, signed repository metadata, exact URI/snapshot/index digests and chosen ARM architecture.
2. Provider checks S17, S16 and the exact source HEAD.
3. Stage real `apt/dpkg/proot` payload under a temporary path and test its archive topology, SPDX/upstream rights, ELF/ABI and prefix with independent receipts. Do **not** silently relabel existing bridge-only output.
4. Only with authorized hardware, build a separately signed test variant, verify installed `apt update/pkg install`, terminal keyboard and rollback transaction.
5. Rollback: disable `RAFAELIA_S15_MIDDLEWARE`, keep prior bootstraps/branches, do not delete/overwrite the phone's data.

`SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM` · `TOKEN_VAZIO != 0` · `claim_allowed=false`.
