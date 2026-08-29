# Device Integration Plan — P0.1-P0.5 Runtime Validation

**Status**: PLANNING  
**Path**: Orthogonal to CI infrastructure fix  
**Authority**: termux-app-rafacodephi local ops

---

## Overview

Parallel operational track: Deploy P0.1-P0.5 to physical Android device, execute bootstrap stages, capture runtime receipt with φ convergence validation.

**Independence**: Does not require CI APK path fix; uses local gradle build + adb.

---

## Prerequisites

1. Android device (ARM64 preferred, ARM32 supported)
2. adb accessible
3. Local gradle build infrastructure (`./gradlew :app:assembleDebug`)
4. adb logcat monitoring

---

## Phases

### Phase 1: Local APK Build (15 min)
- Build debug APK with P0 modules: `./gradlew :app:assembleDebug --no-daemon`
- Output: `app/build/intermediates/apk/debug/app-debug.apk` (or locate via `find`)
- Verify P0 .so files included: `unzip -l app-debug.apk | grep lib.*\.so`

### Phase 2: Device Deployment (5 min)
- Clear app data: `adb shell pm clear com.termux.rafacodephi || true`
- Install APK: `adb install -r app-debug.apk`
- Verify installation: `adb shell pm list packages | grep termux.rafacodephi`

### Phase 3: Bootstrap Execution (10 min)
- Clear logcat: `adb logcat -c`
- Launch activity: `adb shell am start -n com.termux.rafacodephi/com.termux.app.BootstrapGateActivity`
- Capture log: `adb logcat -v raw > bootstrap_receipt.log &`
- Wait for app to stabilize (30s)
- Stop log capture

### Phase 4: Receipt Validation (5 min)
- Parse receipt from logcat: grep receipt structure (magic, stage, phi_fst, attractor, φ ∈ [0,1])
- Verify P0 stages executed: proot_fork, extract_payload, dpkg_install, bootstrap_orchestrator, validator
- Validate φ = (1-H)·C convergence constraint
- Document evidence: exact logcat line, timestamp, device ID

---

## Success Criteria

- [x] APK builds locally without P0-related errors
- [ ] Device installation succeeds
- [ ] Bootstrap stages execute in order
- [ ] Receipt captured with valid structure
- [ ] φ_fst ∈ [0, 0x10000] in Q16 fixed-point
- [ ] attractor ∈ [0, 41]
- [ ] No restart_count > 2 or skip_count > 0
- [ ] Runtime execution logged to logcat

---

## Known Limitations

- Requires physical device (emulator may not have required APK/framework)
- No root; respects Android sandbox
- TAR extraction requires `/data/data/com.termux.rafacodephi` writable
- BLAKE3 validation depends on bootstrap embedded hash presence

---

## Exit Criterion

**DEVICE_RECEIPT_VALID**: Receipt with timestamp, device signature, and φ validation documented.

**Claim Allowed**: Conditional on device ≥ Android 7.0, ARM64 or ARM32 ARMv7.

---

**Operational Next Step**: Coordinate with CI infrastructure track. Proceed with Phase 1 (local build) now.
