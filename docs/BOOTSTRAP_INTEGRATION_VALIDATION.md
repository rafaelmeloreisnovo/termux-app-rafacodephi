# Bootstrap Integration Validation Guide

**Objective:** Comprehensive verification that Verbovivo bootstrap integration is working correctly in the APK and on device.

## Integration Checklist

### Source Code Components

- [x] `app/src/main/java/com/termux/rafaelia/VerbativoCore.java` (172 lines)
  - JNI wrapper for Verbovivo native functions
  - Static initializer with fail-closed behavior
  - Public API: `isInitialized()`, `executeConvergence()`, `computePhiMetrics()`, `generateConvergenceReceipt()`, `validateConvergenceReceipt()`, `recallAttractor()`

- [x] `app/src/main/java/com/termux/app/bootstrap/VerbativoBootstrapManager.java` (227 lines)
  - Bootstrap lifecycle manager
  - Main entry: `executeBootstrap()` returns boolean
  - Stores receipt in appFilesDir/verbovivo/convergence_receipt.bin
  - Provides metrics extraction and validation

- [x] `rafaelia/src/main/cpp/verbovivo_jni.c` (232 lines)
  - JNI bridge to native Verbovivo library
  - JNI_OnLoad() initializes T^7 toroid graph
  - Six native functions implemented with proper exception handling

- [x] `rafaelia/src/main/cpp/Android.mk`
  - Builds libverbovivo_graph static library from three source files
  - Builds termux-rafaelia-verbovivo JNI bridge
  - Main termux-rafaelia library links both
  - 16KB page alignment for Android 15+

- [x] `app/src/main/java/com/termux/app/TermuxActivity.java`
  - Bootstrap integrated in onCreate() (line ~240)
  - Fail-closed: blocks app startup if validation fails
  - Logs metrics and status via Logger

### Native Source Files

- [x] `rafaelia/verbovivo_graph.c` — T^7 toroid lattice implementation
- [x] `rafaelia/t7_toroid_builder.c` — Toroid state initialization
- [x] `rafaelia/verbovivo_bootstrap_gate.c` — Convergence validation gate
- [x] `rafaelia/verbovivo_graph.h` — Public interface header

### Testing & Documentation

- [x] `VERBOVIVO_DEVICE_TEST_PLAN.md` — 6-phase device testing plan
- [x] `VERBOVIVO_COMPLETE_TESTING_GUIDE.md` — End-to-end testing procedures
- [x] `scripts/validate_verbovivo_device.sh` — Automated ADB validation
- [x] This document — Bootstrap integration validation

## Pre-Build Verification (Static Analysis)

### 1. File Presence Verification

```bash
#!/bin/bash
# Verify all required files exist
files=(
  "app/src/main/java/com/termux/rafaelia/VerbativoCore.java"
  "app/src/main/java/com/termux/app/bootstrap/VerbativoBootstrapManager.java"
  "rafaelia/src/main/cpp/verbovivo_jni.c"
  "rafaelia/src/main/cpp/Android.mk"
  "rafaelia/verbovivo_graph.c"
  "rafaelia/t7_toroid_builder.c"
  "rafaelia/verbovivo_bootstrap_gate.c"
  "rafaelia/verbovivo_graph.h"
)

for f in "${files[@]}"; do
  [ -f "$f" ] && echo "✓ $f" || echo "✗ MISSING: $f"
done
```

### 2. Code Review Verification

**VerbativoCore.java:**
```java
// Line 18: JNI type conversion
initialized = isInitializedNative() != 0;  // ✓ Correct boolean conversion
```

**VerbativoBootstrapManager.java:**
```java
// executeBootstrap() must:
// 1. Check Verbovivo initialized
if (!VerbativoCore.isInitialized()) return false;  // ✓
// 2. Generate receipt
byte[] receipt = VerbativoCore.generateConvergenceReceipt();
// 3. Validate receipt
if (VerbativoCore.validateConvergenceReceipt(receipt) != 0) return false;  // ✓
// 4. Store receipt
storeReceipt(receipt);  // ✓
// 5. Return true only if all steps pass
return true;  // ✓
```

**TermuxActivity.java Integration:**
```java
// Line ~240 (onCreate after preferences)
VerbativoBootstrapManager verbativoBootstrap = new VerbativoBootstrapManager(this);
if (!verbativoBootstrap.executeBootstrap()) {
    Logger.logError(LOG_TAG, "Verbovivo bootstrap validation FAILED - app initialization blocked");
    mIsInvalidState = true;
    return;  // ✓ Fail-closed: blocks app startup
}
```

### 3. Build Configuration Verification

**Android.mk Paths:**
```bash
cd rafaelia/src/main/cpp
# Verify paths from cpp/ resolve to verbovivo files in rafaelia/
[ -f ../../../verbovivo_graph.c ] && echo "✓ Path correct" || echo "✗ Path error"
```

Expected structure:
```
rafaelia/
├── verbovivo_graph.c ← ../../../ from cpp/
├── t7_toroid_builder.c
├── verbovivo_bootstrap_gate.c
├── verbovivo_graph.h
└── src/main/cpp/
    ├── Android.mk
    └── verbovivo_jni.c
```

## Build Verification (Gradle)

### 1. Syntax Check
```bash
# Verify no compilation errors
./gradlew :app:compileDebugJava --no-daemon

# Expected output: BUILD SUCCESSFUL
```

### 2. Native Build
```bash
# Verify NDK builds Verbovivo libraries
./gradlew :app:assembleDebug -Pandroid.injected.build.abi=arm64-v8a --no-daemon

# Check build outputs:
find build/ -name "libverbovivo_graph.a" -o -name "libterm*-rafaelia.so"
```

### 3. APK Packaging
```bash
# Verify APK contains JNI libraries
unzip -l build/outputs/apk/debug/termux-*arm64*.apk | grep "\.so$"

# Expected files:
# lib/arm64-v8a/libverbovivo_graph.a (in static library, not in APK directly)
# lib/arm64-v8a/libtermux-rafaelia.so (contains Verbovivo JNI)
```

## Runtime Verification (Logcat)

### 1. Bootstrap Startup
```bash
# Start app and capture logs
adb logcat -c
adb shell am start -n com.termux.rafacodephi/com.termux.app.TermuxActivity
sleep 2
adb logcat | grep -E "(VerbativoCore|VerbativoBootstrap)"
```

**Expected Log Sequence:**
```
I/VerbativoCore: Verbovivo graph loaded and initialized
I/VerbativoBootstrap: Starting Verbovivo bootstrap validation
I/VerbativoBootstrap: ✓ Verbovivo core initialized
I/VerbativoBootstrap: ✓ Convergence receipt generated (26 bytes)
I/VerbativoBootstrap: ✓ Receipt validation passed
I/VerbativoBootstrap: ✓ Bootstrap φ validation: H=XX.XX% C=YY.YY% φ=ZZ.ZZ% attractor=N status=M
I/VerbativoBootstrap: ✓ Receipt stored for audit trail
I/VerbativoBootstrap: ✅ Verbovivo bootstrap validation PASSED
```

### 2. Metric Extraction
```bash
# Extract metrics from logcat
adb logcat -d | grep "Bootstrap φ validation:" | tail -1
```

**Parse and validate:**
- H ∈ [0.0, 100.0]
- C ∈ [0.0, 100.0]
- φ ∈ [0.0, 100.0]
- φ ≈ (1 - H/100) × (C/100) × 100 (within 5% tolerance)
- attractor ∈ [0, 41] or 255
- status ∈ [0, 3]

### 3. Consistency Checks
```bash
# Attractor-status consistency
if [ attractor < 42 ]; then
  assert status == 0  # Converged
else
  assert status != 0  # Not converged
fi

# Status codes:
# 0 = Attractor (converged to fixed point)
# 1 = Stable (in stable manifold)
# 2 = No edges (isolated point)
# 3 = Timeout (convergence iteration limit)
```

## Receipt Verification

### 1. File Existence
```bash
# Check receipt file location
adb shell run-as com.termux.rafacodephi test -f files/verbovivo/convergence_receipt.bin
echo "Exit code: $?"  # Should be 0
```

### 2. Binary Format
```bash
# Extract and verify receipt structure
adb shell run-as com.termux.rafacodephi hexdump -C files/verbovivo/convergence_receipt.bin

# Expected layout (26 bytes):
# 00000000: HHHH HHHH CCCC CCCC PPPP PPPP AAAA SSSS
#          ↑ 8B H  ↑ 8B C  ↑ 8B φ   ↑ 1B A ↑ 1B S
```

### 3. Metric Extraction from Receipt
```python
#!/usr/bin/env python3
import struct
import sys

# Read 26-byte receipt
with open('convergence_receipt.bin', 'rb') as f:
    data = f.read(26)

# Parse big-endian uint64 values (Q16 format)
H_norm = struct.unpack('>Q', data[0:8])[0]
C_norm = struct.unpack('>Q', data[8:16])[0]
phi_fst = struct.unpack('>Q', data[16:24])[0]
attractor_id = data[24]
status = data[25]

# Convert Q16 to percentage (0x10000 = 100%)
H_pct = (H_norm / 0x10000) * 100
C_pct = (C_norm / 0x10000) * 100
phi_pct = (phi_fst / 0x10000) * 100

print(f"H_norm: {H_pct:.2f}%")
print(f"C_norm: {C_pct:.2f}%")
print(f"φ_fst: {phi_pct:.2f}%")
print(f"Attractor ID: {attractor_id}")
print(f"Status: {status}")

# Verify formula: φ = (1-H) × C
expected_phi = ((1.0 - H_norm/0x10000) * (C_norm/0x10000)) * 0x10000
error_pct = abs(phi_fst - expected_phi) / expected_phi * 100
print(f"Formula error: {error_pct:.2f}%")
if error_pct > 5:
    print("⚠ φ formula validation FAILED")
    sys.exit(1)
```

## CI Gate Status

### Expected Build Results

```
✅ Passing:
  - ψ Perception - Contract Gate
  - χ Feedback - Invariant Check
  - Σ Execution - Android Compatibility
  - Δ Validation - Unit Tests
  - 📱 ARM32 v7 (all variants)
  - 📱 ARM64 Debug (all variants)
  - 📱 ARM64 Release (if enabled)

❌ Failing (indicates problem):
  - Ω Alignment - Terminal Gate (fails if any required stage fails)
  - Any ARM64 build failure (indicates path or JNI issue)
  - Unit test failures (indicates logic error)
```

### CI Logs to Inspect

If CI fails, check:

1. **Gradle Compilation**
   ```
   Error line pattern: "error: incompatible types"
   → Check VerbativoCore.java line 18 (JNI type conversion)
   ```

2. **NDK Build**
   ```
   Error pattern: "cannot find -lverbovivo"
   → Check Android.mk link order (static libraries before shared)
   ```

3. **Path Resolution**
   ```
   Error pattern: "cannot open source file"
   → Check Android.mk LOCAL_SRC_FILES paths (should resolve from cpp/ directory)
   ```

4. **Runtime (on device)**
   ```
   Error pattern: "UnsatisfiedLinkError"
   → JNI library loading failed; check native library compilation
   ```

## Final Validation Checklist

Before declaring integration complete:

- [x] All source files committed
- [x] Android.mk paths corrected (b08b1d0a)
- [x] JNI type conversion fixed (672d6fdb)
- [ ] CI passes: ψ χ ρ Δ Σ → Ω GREEN
- [ ] Device test: APK installs
- [ ] Device test: Bootstrap logs appear
- [ ] Device test: Receipt generated (26 bytes)
- [ ] Device test: Metrics extracted and validated
- [ ] Device test: Attractor consistency verified
- [ ] Device test: App functionality preserved

## Next Steps

1. **Monitor CI** — Wait for ARM64 debug builds with corrected paths
2. **Device Test** — Execute `scripts/validate_verbovivo_device.sh --verbose`
3. **Data Collection** — Record device info, metrics, and logs
4. **Documentation** — Update PR with device test results

---

**Version History:**
- 2026-08-29: Initial validation guide created
- 2026-08-29: Android.mk paths corrected (b08b1d0a)
- 2026-08-29: JNI type conversion fixed (672d6fdb)
