# Verbovivo Complete Testing & Validation Guide

## Overview

This guide covers the complete testing workflow for Verbovivo bootstrap integration from build through device validation, including detailed troubleshooting and data collection procedures.

---

## Part 1: Build Verification

### 1.1 Host Build Prerequisites

**Required Software:**
- Java Development Kit (JDK) 11+ (test with `javac -version`)
- Android SDK (API 31+)
- Android NDK (r26d recommended)
- Git 2.40+
- Bash 5.0+

**Environment Setup:**

```bash
# Verify Android SDK/NDK paths
export ANDROID_HOME=${ANDROID_HOME:-~/Android/sdk}
export ANDROID_NDK_HOME=${ANDROID_NDK_HOME:-~/Android/ndk/26.1.10909125}

# Verify tools
javac -version        # Expected: 11.0.X or higher
$ANDROID_HOME/ndk-bundle/clang++ --version  # Expected: clang 16.0+
adb version          # Expected: Android Debug Bridge version 1.0.X
```

### 1.2 APK Build Process

**Debug Build:**

```bash
cd /home/user/termux-app-rafacodephi

# Clean rebuild (full compilation)
./gradlew clean

# Verbose build for troubleshooting
./gradlew :app:assembleDebug --info --no-daemon

# Expected output
# > Task :app:compileDebugJavaWithJavac OK
# > Task :app:buildNdkBuildDebug[arm64-v8a] OK
# > Task :app:buildNdkBuildDebug[armeabi-v7a] OK
# > Task :app:assembleDebug OK
```

**Build Artifacts Locations:**

```
build/outputs/apk/debug/
├── termux-rafcodephi-debug-arm64-v8a.apk      (ARM64, ~120MB)
├── termux-rafcodephi-debug-armeabi-v7a.apk    (ARMv7, ~115MB)
├── termux-rafcodephi-debug-x86.apk            (x86, ~120MB)
├── termux-rafcodephi-debug-x86_64.apk         (x86_64, ~125MB)
└── termux-rafcodephi-debug-universal.apk      (all ABIs, ~480MB)
```

### 1.3 Build Validation Checklist

- [ ] No compilation errors (exit code 0)
- [ ] `compileDebugJavaWithJavac` succeeds (no type errors)
- [ ] `buildNdkBuildDebug` completes for all ABIs
- [ ] `assembleDebug` produces APK files
- [ ] APK files present and non-zero size
- [ ] No warnings about deprecated APIs (Java 8 compatibility)

---

## Part 2: Device Preparation

### 2.1 Device Selection

**Recommended Test Devices:**

| Device | Architecture | Android | Reason |
|--------|-------------|---------|--------|
| Pixel 5a | ARM64 | 13+ | Modern, official support |
| Pixel 4 | ARM64 | 12 | Mid-range, stable |
| OnePlus 8T | ARM64 | 12+ | High performance |
| Galaxy A52 | ARM64 | 12+ | Mid-range, common |
| Samsung Galaxy Tab S7 | ARM64 | 12+ | Larger screen for testing |
| Android Emulator (arm64) | ARM64 | 12+ | No physical device needed |

**Minimum Requirements:**
- Android 7.0+ (API 24)
- 100MB free storage
- USB debugging enabled
- USB Type-C or Micro-USB cable (for physical device)

### 2.2 Device Setup

**Physical Device:**

```bash
# Enable Developer Options
Settings > About Phone > Build Number (tap 7 times)

# Enable USB Debugging
Settings > Developer Options > USB Debugging (ON)

# Connect device
adb devices
# Should output: [device-id] device

# Trust computer (if prompted on device)
# Tap "Always allow" on the device prompt
```

**Android Emulator:**

```bash
# List available AVDs
$ANDROID_HOME/emulator/emulator -list-avds

# Start ARM64 emulator
$ANDROID_HOME/emulator/emulator -avd Pixel_5_API_33 &

# Wait for boot (3-5 min)
adb wait-for-device
```

### 2.3 Pre-Installation Checks

```bash
# Verify device connectivity
adb devices -l
# Expected: device is "device" state, not "unauthorized" or "offline"

# Check available space
adb shell df /data | awk '{print $4}' | head -2
# Expected: >100000 (blocks, ~100MB free)

# Check Android version
adb shell getprop ro.build.version.release
# Expected: 7.0+

# Check device architecture
adb shell getprop ro.product.cpu.abi
# Expected: arm64-v8a, armeabi-v7a, x86, or x86_64
```

---

## Part 3: APK Installation

### 3.1 Install via ADB

**Select Correct APK:**

```bash
DEVICE_ABI=$(adb shell getprop ro.product.cpu.abi)
echo "Device ABI: $DEVICE_ABI"

# Match APK to device
case "$DEVICE_ABI" in
  arm64-v8a) APK="build/outputs/apk/debug/termux-rafcodephi-debug-arm64-v8a.apk" ;;
  armeabi-v7a) APK="build/outputs/apk/debug/termux-rafcodephi-debug-armeabi-v7a.apk" ;;
  x86) APK="build/outputs/apk/debug/termux-rafcodephi-debug-x86.apk" ;;
  x86_64) APK="build/outputs/apk/debug/termux-rafcodephi-debug-x86_64.apk" ;;
  *) APK="build/outputs/apk/debug/termux-rafcodephi-debug-universal.apk" ;;
esac

echo "Selected APK: $APK"
```

**Install:**

```bash
# Remove old version (if installed)
adb uninstall com.termux.rafacodephi 2>/dev/null || true

# Install new APK
adb install "$APK"
# Expected: "Success"

# Verify installation
adb shell pm list packages | grep "com.termux.rafacodephi"
# Expected: "package:com.termux.rafacodephi"
```

### 3.2 Installation Troubleshooting

**Issue: "Failure [INSTALL_FAILED_INSUFFICIENT_STORAGE]"**

```bash
# Free space
adb shell rm -rf /sdcard/Android/data/com.termux.rafacodephi
adb shell am clear-package-state com.termux.rafacodephi 2>/dev/null || true

# Check space again
adb shell df /data
# Need >150MB free

# Retry install
adb install "$APK"
```

**Issue: "Failure [INSTALL_FAILED_INVALID_APK]"**

```bash
# Verify APK integrity
$ zipcmp -r build/outputs/apk/debug/termux-rafcodephi-debug-*.apk | head -20

# Check if APK is corrupted
$ file "$APK"
# Expected: "Zip archive data"

# Rebuild APK
./gradlew clean :app:assembleDebug
```

**Issue: "Failure [INSTALL_FAILED_MISSING_SHARED_LIBRARY]"**

```bash
# JNI library failed to load
# Check device logs
adb logcat | grep -i "UnsatisfiedLinkError"

# May indicate NDK incompatibility - rebuild with different NDK
export NDK_VERSION=r26d
./gradlew :app:assembleDebug
```

---

## Part 4: Verbovivo Bootstrap Testing

### 4.1 Automated Testing (Recommended)

```bash
# Run comprehensive test
./scripts/validate_verbovivo_device.sh --verbose

# Expected output includes:
# [✓] ADB device found
# [✓] App installed: com.termux.rafacodephi
# [✓] App started
# [✓] Bootstrap validation PASSED
#   H (entropy):   XX.XX%
#   C (coherence): YY.YY%
#   φ (convergence): ZZ.ZZ%
#   Attractor ID: N
#   Status: M
# [✓] Receipt file size: 26 bytes (correct)
# [✓] Metric bounds verified
# [✓] Attractor consistency verified
# [✓] Second launch metrics stable
# ✅ Status: PASS
```

### 4.2 Manual Testing (If Script Unavailable)

**Step 1: Clear Logs**

```bash
adb logcat -c
```

**Step 2: Start App**

```bash
adb shell am start -n com.termux.rafacodephi/com.termux.app.TermuxActivity
sleep 3
```

**Step 3: Check Bootstrap Logs**

```bash
adb logcat | grep -E "Verbovivo(Bootstrap|Core)" | head -20
```

**Expected Output:**

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

### 4.3 Receipt File Extraction

```bash
# Extract receipt
adb shell run-as com.termux.rafacodephi cat files/verbovivo/convergence_receipt.bin > /tmp/convergence_receipt.bin

# Verify size
ls -lh /tmp/convergence_receipt.bin
# Expected: 26 bytes

# View hex content
hexdump -C /tmp/convergence_receipt.bin
# Expected: 26 bytes of data (not all zeros)
```

### 4.4 Metric Validation

**Parse Receipt Programmatically:**

```bash
# Extract and decode receipt metrics
python3 << 'PYTHON'
import struct
import sys

with open('/tmp/convergence_receipt.bin', 'rb') as f:
    data = f.read(26)
    
    if len(data) != 26:
        print(f"ERROR: Receipt size {len(data)}, expected 26")
        sys.exit(1)
    
    # Parse big-endian uint64 values
    h_norm = struct.unpack('>Q', data[0:8])[0]
    c_norm = struct.unpack('>Q', data[8:16])[0]
    phi_fst = struct.unpack('>Q', data[16:24])[0]
    attractor_id = data[24]
    status = data[25]
    
    # Convert Q16 fixed-point to percentages
    h_percent = (h_norm / 0x10000) * 100
    c_percent = (c_norm / 0x10000) * 100
    phi_percent = (phi_fst / 0x10000) * 100
    
    print(f"H_norm:      0x{h_norm:016x} ({h_percent:.2f}%)")
    print(f"C_norm:      0x{c_norm:016x} ({c_percent:.2f}%)")
    print(f"φ_fst:       0x{phi_fst:016x} ({phi_percent:.2f}%)")
    print(f"Attractor:   {attractor_id}")
    print(f"Status:      {status}")
    print()
    
    # Validate bounds
    if not (0 <= h_norm <= 0x10000):
        print(f"ERROR: H_norm out of bounds: 0x{h_norm:x}")
        sys.exit(1)
    if not (0 <= c_norm <= 0x10000):
        print(f"ERROR: C_norm out of bounds: 0x{c_norm:x}")
        sys.exit(1)
    if not (0 <= phi_fst <= 0x10000):
        print(f"ERROR: φ_fst out of bounds: 0x{phi_fst:x}")
        sys.exit(1)
    
    # Validate formula: φ = (1-H)·C
    one_minus_h = 0x10000 - h_norm
    expected_phi = (one_minus_h * c_norm) >> 16
    tolerance = int(0x10000 * 0.05)  # 5% tolerance
    
    if abs(phi_fst - expected_phi) > tolerance:
        print(f"WARNING: φ formula mismatch")
        print(f"  Expected: 0x{expected_phi:x}")
        print(f"  Actual:   0x{phi_fst:x}")
        print(f"  Delta:    0x{abs(phi_fst - expected_phi):x}")
    else:
        print("✓ φ formula verified")
    
    # Validate attractor consistency
    if attractor_id < 42 and status != 0:
        print(f"ERROR: Attractor consistency violation: ID={attractor_id} status={status}")
        sys.exit(1)
    
    if status > 3:
        print(f"ERROR: Status out of range: {status}")
        sys.exit(1)
    
    print("✓ All validations passed")
PYTHON
```

---

## Part 5: Stability Testing

### 5.1 Multiple Launch Test

```bash
#!/bin/bash
# Test convergence stability across app restarts

echo "=== Verbovivo Stability Test ==="
echo ""

for i in {1..5}; do
    echo "Launch $i:"
    
    adb shell am force-stop com.termux.rafacodephi 2>/dev/null || true
    sleep 1
    
    adb logcat -c
    adb shell am start -n com.termux.rafacodephi/com.termux.app.TermuxActivity >/dev/null
    sleep 2
    
    METRICS=$(adb logcat -d | grep "Bootstrap φ validation:" | tail -1)
    
    if [ -z "$METRICS" ]; then
        echo "  ❌ Bootstrap did not complete"
        continue
    fi
    
    H=$(echo "$METRICS" | grep -oP 'H=\K[0-9.]+')
    C=$(echo "$METRICS" | grep -oP 'C=\K[0-9.]+')
    PHI=$(echo "$METRICS" | grep -oP 'φ=\K[0-9.]+')
    
    echo "  H=$H% C=$C% φ=$PHI%"
done

echo ""
echo "Stability check complete. Metrics should vary <5% across launches."
```

### 5.2 Concurrent Activity Test

```bash
#!/bin/bash
# Test bootstrap while other services are running

adb shell am start -n com.termux.rafacodephi/com.termux.app.TermuxActivity
sleep 2

# Start background activity
adb shell "am startservice com.android.systemui/.SystemUIService" 2>/dev/null || true

# Spawn logcat reader in background
adb logcat | grep "Verbovivo" &
LOGCAT_PID=$!

# Force restart app while services are running
sleep 3
adb shell am kill com.termux.rafacodephi
sleep 1
adb shell am start -n com.termux.rafacodephi/com.termux.app.TermuxActivity

# Wait and check results
sleep 3
kill $LOGCAT_PID 2>/dev/null || true

# Verify bootstrap still completed
adb logcat -d | grep "PASSED" && echo "✓ Bootstrap succeeded with concurrent services"
```

---

## Part 6: Performance Monitoring

### 6.1 Runtime Metrics Collection

```bash
# Collect performance data during bootstrap

adb shell "dumpsys meminfo com.termux.rafacodephi" > /tmp/memory_baseline.txt
adb shell "dumpsys cpuinfo" > /tmp/cpu_baseline.txt

# Start app and monitor
adb logcat | tee /tmp/bootstrap_run.log &
LOGCAT_PID=$!

adb shell am start -n com.termux.rafacodephi/com.termux.app.TermuxActivity
sleep 5

kill $LOGCAT_PID
adb shell "dumpsys meminfo com.termux.rafacodephi" > /tmp/memory_peak.txt

# Analyze metrics
echo "Memory Usage:"
grep "TOTAL" /tmp/memory_baseline.txt /tmp/memory_peak.txt
```

### 6.2 Bootstrap Timing

```bash
# Extract timing information from logs

START=$(adb logcat -d | grep "Starting Verbovivo bootstrap" | grep -oP '\d{2}:\d{2}:\d{2}\.\d+')
END=$(adb logcat -d | grep "PASSED" | grep -oP '\d{2}:\d{2}:\d{2}\.\d+')

echo "Bootstrap timing:"
echo "  Start: $START"
echo "  End:   $END"
echo "  Duration: <1 second (expected)"
```

---

## Part 7: Failure Diagnosis

### 7.1 Common Failure Scenarios

**Scenario 1: JNI Library Not Found**

Logs:
```
E/VerbativoCore: Failed to load Verbovivo native library
E/VerbativoCore: java.lang.UnsatisfiedLinkError: dlopen failed: cannot locate symbol "vv_build_t7_toroid"
```

**Resolution:**
1. Verify native library built: `adb shell ls -la /data/app/com.termux.rafacodephi-*/lib/arm64-v8a/libtermux-rafaelia.so`
2. Check symbol table: `nm -C build/intermediates/ndkBuild/debug/arm64-v8a/libtermux-rafaelia.so | grep vv_`
3. Rebuild with verbose NDK output: `./gradlew :app:assembleDebug --info`

**Scenario 2: Convergence Receipt Validation Failed**

Logs:
```
E/VerbativoBootstrap: Convergence receipt validation failed
```

**Resolution:**
1. Extract receipt and check structure: `adb shell run-as com.termux.rafacodephi hexdump -C files/verbovivo/convergence_receipt.bin`
2. Verify bounds: All bytes should represent values in range [0, 0x10000]
3. Check formula: φ should ≈ (1-H)·C within 5%

**Scenario 3: App Crashes During Bootstrap**

Logs:
```
F/libc: /system/lib64/libc.so (_ZN3art7Runtime11AbortSigAbortEPKcS2_
FATAL EXCEPTION: main
  Process: com.termux.rafacodephi, PID: XXXXX
  java.lang.RuntimeException: Unable to start activity ...
```

**Resolution:**
1. Check logcat for native crash: `adb logcat -d | grep -i "signal\|abort\|segfault"`
2. Get full tombstone: `adb shell cat /data/anr/traces.txt | head -50`
3. Verify native library wasn't stripped: `file build/intermediates/ndkBuild/debug/arm64-v8a/libtermux-rafaelia.so`

---

## Part 8: Test Report Template

Create file: `tests/device/report_[device_model]_[date].md`

```markdown
# Verbovivo Device Test Report

## Device Information
- **Model:** [e.g., Pixel 5a]
- **Android Version:** [e.g., 13]
- **Build ID:** [from: adb shell getprop ro.build.id]
- **Architecture:** [e.g., arm64-v8a]
- **Kernel:** [from: adb shell uname -r]

## Test Environment
- **APK Version:** [from build]
- **Build Date:** [timestamp]
- **Commit:** [commit hash]
- **Tester:** [name/date]

## Bootstrap Results
| Metric | Value | Status |
|--------|-------|--------|
| H (Entropy) | XX.XX% | ✓ Pass |
| C (Coherence) | YY.YY% | ✓ Pass |
| φ (Convergence) | ZZ.ZZ% | ✓ Pass |
| φ Formula | (1-H)·C ± 5% | ✓ Pass |
| Attractor ID | N (0-41) | ✓ Pass |
| Status Code | M (0-3) | ✓ Pass |
| Receipt File | 26 bytes | ✓ Pass |

## Stability Results
- Launch 1: H=XX% C=YY% φ=ZZ%
- Launch 2: H=XX% C=YY% φ=ZZ%
- Launch 3: H=XX% C=YY% φ=ZZ%
- Launch 4: H=XX% C=YY% φ=ZZ%
- Launch 5: H=XX% C=YY% φ=ZZ%

**Variance:** <5% across launches ✓

## App Functionality
- [ ] Terminal UI renders
- [ ] Commands execute
- [ ] Termux functions normally
- [ ] Receipt persists across restarts

## Issues/Observations
[Any crashes, unexpected values, or notes]

## Conclusion
**Status:** PASS / FAIL
**Date:** [timestamp]
**Evidence:** convergence_receipt.bin, logcat.log

---
*Report generated by Verbovivo testing framework*
```

---

## Quick Reference

**All-In-One Test Command:**

```bash
#!/bin/bash
set -e

echo "=== Verbovivo Complete Test ==="

# Build
echo "1. Building APK..."
cd /home/user/termux-app-rafacodephi
./gradlew clean :app:assembleDebug --no-daemon

# Get device ABI
ABI=$(adb shell getprop ro.product.cpu.abi)
APK="build/outputs/apk/debug/termux-rafcodephi-debug-${ABI}.apk"

# Install
echo "2. Installing APK..."
adb install -r "$APK"

# Test
echo "3. Running validation..."
./scripts/validate_verbovivo_device.sh --verbose

echo "✅ All tests completed"
```

---

## Support & Escalation

If tests fail:

1. **Check prerequisites:** OS, SDK, NDK, ADB versions
2. **Review logs:** `adb logcat | grep -i "error\|fail\|verbovivo"`
3. **Rebuild clean:** `./gradlew clean :app:assembleDebug`
4. **Try different device:** May be device-specific issue
5. **Check GitHub issues:** Report reproducible failures

