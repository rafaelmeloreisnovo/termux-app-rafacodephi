# Verbovivo Bootstrap Device Testing Plan

## Prerequisites

- Physical Android device (ARM64 or ARMv7) running Android 7.0+
- Android SDK Platform Tools (`adb`) installed on host
- USB debugging enabled on device
- Device connected via USB cable
- OR: Android Emulator (arm64-v8a or armeabi-v7a) running

## Build Options

### Option 1: Build APK on Host (Recommended)

```bash
cd /home/user/termux-app-rafacodephi

# Set up Android SDK path (if not in PATH)
export ANDROID_HOME=/path/to/Android/sdk
export ANDROID_SDK_ROOT=$ANDROID_HOME

# Clean build
./gradlew clean :app:assembleDebug --no-daemon

# Output APK location
# build/outputs/apk/debug/termux-rafcodephi-debug-*.apk
```

### Option 2: Build on Device via Termux

If host doesn't have Android SDK:

```bash
# On device (via Termux terminal)
pkg install clang git
cd ~/termux-app-rafacodephi
./gradlew :app:assembleDebug --no-daemon
```

## Device Installation

### Method A: ADB Push (Recommended)

```bash
# Identify device
adb devices

# Build output paths (adapt ABI as needed)
DEBUG_APK="build/outputs/apk/debug/termux-rafcodephi-debug-arm64-v8a.apk"

# Install APK
adb install -r "$DEBUG_APK"

# Verify installation
adb shell pm list packages | grep "com.termux.rafacodephi"
```

### Method B: Manual Installation

```bash
# Copy APK to device manually via file manager
# Or use: adb push build/outputs/apk/debug/termux-rafcodephi-debug-universal.apk /sdcard/
# Then install from device's file manager
```

## Testing Checklist

### Phase 1: App Startup (5 min)

```bash
adb logcat -c
adb shell am start -n com.termux.rafacodephi/com.termux.app.TermuxActivity
sleep 2
adb logcat | grep -E "(VerbativoBootstrap|VerbativoCore)" | head -20
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

**Validation:**
- [ ] App launches without crashing
- [ ] Bootstrap message appears in logcat
- [ ] H%, C%, φ% values are printed (0-100%)
- [ ] Attractor ID is 0-41
- [ ] Status code is 0-3
- [ ] φ computation: φ = (1-H)·C verified (rough check)

### Phase 2: Receipt Verification (5 min)

```bash
# Extract stored receipt
adb shell run-as com.termux.rafacodephi cat files/verbovivo/convergence_receipt.bin > /tmp/receipt.bin

# Check receipt file size (should be 26 bytes)
hexdump -C /tmp/receipt.bin | head -2
```

**Expected Output:**
```
00000000: HHHH HHHH CCCC CCCC PPPP PPPP AAAA SSSS ...
         ↑────────────────────────────────────────↑
         8 bytes H  8 bytes C  8 bytes φ  1 A  1 S
```

**Validation:**
- [ ] Receipt file exists
- [ ] File size is exactly 26 bytes
- [ ] Hex values appear (not all zeros)
- [ ] All bytes are valid (0x00-0xFF)

### Phase 3: Convergence Metrics (5 min)

```bash
# Extract metrics from receipt programmatically (pseudo-code):
# Bytes 0-7: H_norm (uint64 big-endian) → divide by 0x10000 for percentage
# Bytes 8-15: C_norm (uint64 big-endian) → divide by 0x10000 for percentage
# Bytes 16-23: φ_fst (uint64 big-endian) → divide by 0x10000 for percentage

# Manual check using hexdump
adb shell run-as com.termux.rafacodephi hexdump -C files/verbovivo/convergence_receipt.bin
```

**Validation Formulas:**
- [ ] H_norm ∈ [0x0000, 0x10000]
- [ ] C_norm ∈ [0x0000, 0x10000]
- [ ] φ_fst ∈ [0x0000, 0x10000]
- [ ] φ_fst = (0x10000 - H_norm) × C_norm ÷ 0x10000 (with ±5% tolerance)

### Phase 4: Attractor Consistency (3 min)

```bash
adb logcat | grep "attractor="
```

**Validation:**
- [ ] attractor_id value (0-41 for converged, 255 if not found)
- [ ] status value (0=attractor, 1=stable, 2=no-edges, 3=timeout)
- [ ] If attractor < 42, then status MUST be 0
- [ ] If attractor = 255, then status ≠ 0

### Phase 5: App Functionality (10 min)

```bash
# Once bootstrap passes, verify normal Termux functionality
adb shell am start -n com.termux.rafacodephi/com.termux.app.TermuxActivity

# In Termux terminal (via adb shell or device):
echo "Hello from Termux"
ls /data/data/com.termux.rafacodephi
```

**Validation:**
- [ ] Terminal UI loads
- [ ] Commands execute
- [ ] Receipt file visible in `/data/data/com.termux.rafacodephi/files/verbovivo/`

### Phase 6: Multiple Launches (5 min)

```bash
# Force-close app
adb shell am force-stop com.termux.rafacodephi

# Relaunch
adb shell am start -n com.termux.rafacodephi/com.termux.app.TermuxActivity
sleep 2
adb logcat | grep "VerbativoBootstrap" | tail -10
```

**Validation:**
- [ ] Bootstrap runs on each launch
- [ ] φ values consistent across launches (within 5%)
- [ ] Attractor ID consistent or stable

## Data Collection

Create a test report with:

```markdown
# Device Test Report

## Device Info
- Device: [model/name]
- Android Version: [version]
- Architecture: [arm64-v8a/armeabi-v7a/x86/x86_64]
- Build ID: [ID from "adb shell getprop ro.build.version.release"]

## Bootstrap Metrics
- H_norm (entropy): 0x[HEX] ([PERCENT]%)
- C_norm (coherence): 0x[HEX] ([PERCENT]%)
- φ_fst (convergence): 0x[HEX] ([PERCENT]%)
- Attractor ID: [0-41 or 255]
- Status Code: [0-3]
- Receipt Size: 26 bytes ✓

## Validation Results
- φ formula verified: YES/NO
- Bounds check passed: YES/NO
- Attractor consistency: YES/NO
- Receipt stored: YES/NO
- App functionality: YES/NO

## Issues/Observations
[Any crashes, unexpected values, or observations]
```

## Troubleshooting

### APK Won't Install
```bash
# Check device compatibility
adb shell getprop ro.product.cpu.abi
# Should show: arm64-v8a, armeabi-v7a, x86, or x86_64

# Check storage space
adb shell df /data
# Need at least 100MB free

# Uninstall old version
adb uninstall com.termux.rafacodephi
adb install build/outputs/apk/debug/termux-rafcodephi-debug-arm64-v8a.apk
```

### App Crashes on Startup
```bash
# Check logcat for JNI errors
adb logcat | grep -E "(VerbativoCore|VerbativoBootstrap|JNI|UnsatisfiedLinkError)"

# Verify native library loaded
adb shell pm dump com.termux.rafacodephi | grep -A5 native
```

### Receipt Not Found
```bash
# Verify app data directory
adb shell run-as com.termux.rafacodephi ls -la files/

# Check app permissions
adb shell pm dump com.termux.rafacodephi | grep -i permission
```

### φ Values Out of Bounds
```bash
# Bootstrap validation should have blocked this
# If seen, indicates bug in bounds checking
# Collect the exact values from logcat and receipt

adb logcat > /tmp/device_test.log
adb shell run-as com.termux.rafacodephi hexdump -C files/verbovivo/convergence_receipt.bin >> /tmp/device_test.log
```

## Success Criteria

✅ **PASS** if ALL of the following are true:

1. APK installs without errors
2. App launches and executes `executeBootstrap()`
3. Bootstrap log shows all 5 validation steps passed
4. H_norm, C_norm, φ_fst values are in range [0, 0x10000]
5. φ formula verified: φ_fst ≈ (0x10000 - H_norm) × C_norm ÷ 0x10000
6. Attractor ID matches status code (if ID < 42, status = 0)
7. Receipt file exists and contains 26 bytes
8. App continues to normal Termux functionality
9. Metrics consistent on second launch

❌ **FAIL** if:

- App crashes during bootstrap
- Receipt validation returns false
- φ value outside bounds or formula fails
- Attractor inconsistency detected
- Receipt file corrupted or missing

## CI Integration

Once device testing succeeds:

```bash
# Document successful device run
git add VERBOVIVO_DEVICE_TEST_RESULTS.md
git commit -m "device: Verbovivo bootstrap verified on [device model]"
git push origin claude/p0-freestanding-bootstrap

# Update PR with device results link
```

## References

- Receipt format: VerbativoCore.java line 107-134
- Validation logic: verbovivo_jni.c line 160-214
- Bootstrap manager: VerbativoBootstrapManager.java
- Device testing options: DEVICE_TESTING_GUIDE.md
