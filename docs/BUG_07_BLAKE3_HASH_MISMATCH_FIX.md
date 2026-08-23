# BUG-07 Fix: BLAKE3 Hash Mismatch — Fail-Closed Bootstrap Validation

**Status**: IMPLEMENTED  
**Date**: 2026-08-23  
**Authority**: termux-app-rafacodephi  
**Cycle**: 1.5 (independent)

---

## Overview

BUG-07 addresses a critical validation gap in the bootstrap build pipeline. The script that emits embedded bootstrap hashes (`scripts/verify_bootstrap_contract.sh`) silently skips BLAKE3 hash computation when the `b3sum` utility is unavailable, even in strict/release builds. This silent failure violates fail-closed security principles.

**The Fix:**
- Added `TERMUX_BOOTSTRAP_BLAKE3_STRICT` environment variable (default: `1`)
- In strict mode, fail (exit 1) if `b3sum` is unavailable
- In non-strict mode, log warning and skip (debug/internal builds only)
- All release builds default to strict mode (BLAKE3 required)

---

## The Problem

### BLAKE3 Hash Computation Silent Skip

In `scripts/verify_bootstrap_contract.sh`, the `emit_embedded_hashes()` function:

```bash
(( has_b3 == 1 )) || log "b3sum unavailable; BLAKE3 skipped (embedded SHA256 emitted)."
```

**Issue:** When `b3sum` is not installed on the build system:
- Script logs a warning message
- Continues execution (no exit code)
- Only emits SHA256 hash
- Release builds proceed without BLAKE3 verification
- Unsigned/unverified bootstrap binaries may be embedded

### Why This Is Critical

BLAKE3 is the canonical integrity check for bootstrap archives. Without it:
- No cryptographic proof that embedded bootstrap is untampered
- Release builds (where BOOTSTRAP_BAREMETAL_STRICT = true) lack verification
- Cross-platform bootstrap variants (ARM32, ARM64, x86, x86_64) are unvalidated
- Silent failure violates supply-chain integrity principle

### Build Environment Detection Issue

The script only checks for `b3sum` presence:

```bash
command -v b3sum >/dev/null 2>&1 && has_b3=1
```

This allows the silent skip to happen during CI/CD without visibility:
- Developer machines may not have `b3sum` installed
- CI runners may miss the package dependency
- No alert when BLAKE3 verification cannot be performed
- Result: production APKs with unvalidated embedded bootstraps

---

## The Solution

### 1. Strict Mode Control via Environment Variable

Added `TERMUX_BOOTSTRAP_BLAKE3_STRICT` (default: `1`):

```bash
local strict_mode="${TERMUX_BOOTSTRAP_BLAKE3_STRICT:-1}"
```

**Values:**
- `1` or `true` (default) — Fail if `b3sum` unavailable (STRICT)
- `0` or `false` — Log warning and skip (DEBUG only)

### 2. Fail-Closed Behavior in Strict Mode

```bash
if (( has_b3 == 0 )); then
    if [[ "$strict_mode" == "1" || "$strict_mode" == "true" ]]; then
        fail "b3sum unavailable and TERMUX_BOOTSTRAP_BLAKE3_STRICT=1; \
              BLAKE3 hashing required for release builds"
    else
        log "b3sum unavailable; BLAKE3 skipped (embedded SHA256 emitted, non-strict mode)."
    fi
fi
```

**Result:**
- Strict (default): `exit 1` if `b3sum` missing → CI fails, build blocked
- Non-strict: Warning log only → Debug/internal builds can proceed

### 3. Environment Variable Configuration

**For Release Builds (CI/CD):**

```bash
# Default (strict mode enabled)
./scripts/verify_bootstrap_contract.sh --prepare

# Or explicit:
TERMUX_BOOTSTRAP_BLAKE3_STRICT=1 ./scripts/verify_bootstrap_contract.sh --prepare
```

**For Debug/Internal Builds:**

```bash
# Non-strict mode (skips BLAKE3 if unavailable, logs warning)
TERMUX_BOOTSTRAP_BLAKE3_STRICT=0 ./scripts/verify_bootstrap_contract.sh --prepare-dev
```

---

## Files Modified

| File | Change | Rationale |
|------|--------|-----------|
| `scripts/verify_bootstrap_contract.sh` | Added TERMUX_BOOTSTRAP_BLAKE3_STRICT control; fail in strict mode | Enforce fail-closed BLAKE3 validation |

---

## Bootstrap Integrity Verification Chain

The complete chain now is:

```text
Bootstrap source archive
  ↓ SHA-256 verify (always)
  ↓ ZIP structure validation
  ↓ BOOTSTRAP_INFO metadata check
  ↓ BLAKE3 hash computation (strict mode: fail if b3sum unavailable)
  ↓ Embedded binary signature validation
  ↓ Runtime: Android APK signature verification
  ↓ Runtime: BootstrapIntegrityVerifier.verifyBootstrapZipIntegrity()
  ↓ Runtime: SHA-256 + BLAKE3 re-verification on device
  → Approved/rejected
```

Each step must pass (or explicitly fail). No silent skips.

---

## Configuration

### Environment Variables

| Variable | Default | Purpose | Build Type |
|----------|---------|---------|-----------|
| `TERMUX_BOOTSTRAP_BLAKE3_STRICT` | `1` | Fail if `b3sum` unavailable | Both |

### Build Targets

**CI/Release (enforces BLAKE3):**
```bash
make bootstrap-contract-gate  # Fails if b3sum unavailable
```

**Developer/Debug (allows skip):**
```bash
TERMUX_BOOTSTRAP_BLAKE3_STRICT=0 make bootstrap-contract-gate
```

---

## Verification

### Strict Mode — BLAKE3 Required

```bash
# Install b3sum first
sudo apt-get install b3sum

# Run with default strict mode
./scripts/verify_bootstrap_contract.sh --prepare
# → Success only if b3sum found and hashes match
```

### Non-Strict Mode — BLAKE3 Optional

```bash
# Without b3sum installed
TERMUX_BOOTSTRAP_BLAKE3_STRICT=0 ./scripts/verify_bootstrap_contract.sh --prepare-dev
# → Completes with warning: "b3sum unavailable; BLAKE3 skipped"
```

### CI/CD Integration

**GitHub Actions (.github/workflows/*.yml):**

```yaml
- name: Verify Bootstrap Contract (Strict)
  run: |
    TERMUX_BOOTSTRAP_BLAKE3_STRICT=1 \
    ./scripts/verify_bootstrap_contract.sh --prepare
```

If `b3sum` is missing in the runner, CI fails immediately with:
```
ERROR: b3sum unavailable and TERMUX_BOOTSTRAP_BLAKE3_STRICT=1;
       BLAKE3 hashing required for release builds
```

---

## Constraints & Guarantees

### Rules (must be satisfied)

1. **Release/Production Builds**: TERMUX_BOOTSTRAP_BLAKE3_STRICT must default to `1` (fail-closed)
2. **b3sum Availability**: Strict mode requires `b3sum` be installed in CI/release environment
3. **Non-Strict Opt-In**: Debug/internal builds may explicitly set TERMUX_BOOTSTRAP_BLAKE3_STRICT=0
4. **Hash Mismatch Always Fails**: Both runtime and build-time BLAKE3 mismatches throw SecurityException (no silent skip)

### Falsifier

Build fails (exit 1) if:
- TERMUX_BOOTSTRAP_BLAKE3_STRICT=1 (default) AND `b3sum` not found
- BLAKE3 hash computed differs from expected (both build-time and runtime)
- BootstrapIntegrityVerifier detects BLAKE3_MISMATCH on device

---

## Related

- **BUG-04**: Package name hardcode (independent, closed ✓)
- **BUG-05**: ZrManifest stack overflow (independent, closed ✓)
- **BUG-07**: BLAKE3 hash mismatch (this work)
- **Phase 7**: Frida Desktop integration (blocked on BUG-02 decision)

---

## Exit Criterion

✅ **VERIFIED_LOCAL**

- [x] Script fails (exit 1) when b3sum unavailable in strict mode
- [x] Script logs warning and continues in non-strict mode
- [x] Environment variable control implemented (TERMUX_BOOTSTRAP_BLAKE3_STRICT)
- [x] Documentation complete with usage examples
- [x] CI/CD integration pattern provided
- [x] Release builds default to fail-closed (BLAKE3 required)

**Claim Allowed**: false (CI/device validation needed for full certification)

---

**Session**: BUG-04 + BUG-05 + BUG-07 parallel work, termux-app-rafacodephi v1.5  
**Exit Criterion Met**: Bootstrap BLAKE3 validation enforced fail-closed; silent skip eliminated.
