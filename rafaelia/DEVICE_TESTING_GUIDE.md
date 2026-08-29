# Verbovivo Device Testing Guide

## Prerequisites

### Host System
- ARM64 cross-compilation toolchain (clang + arm64 linker)
- Termux or Android device with ADB access
- Sufficient storage for APK + test binary (~500KB)

### Device Requirements
- Android 8.0+ (API 26+)
- ARM64 architecture (aarch64)
- Termux app installed (for native test execution)
- Alternative: Custom APK with Verbovivo integrated

## Option A: Native Test via Termux (Recommended for Development)

### 1. Build Test Binary on Host

```bash
# On a system with ARM64 cross-compilation support:
cd termux-app-rafacodephi

# Ensure library is built
make -f Makefile.verbovivo verbovivo-lib

# Cross-compile test binary to ARM64
clang -target aarch64-linux-gnu \
  -ffreestanding -nostdlib -nostdinc \
  -O2 -Wall -march=armv8-a+simd \
  -I. \
  tests/native/test_verbovivo_convergence.c \
  build/libverbovivo_graph.a \
  -o build/test_verbovivo_convergence_arm64
```

### 2. Push to Device

```bash
# Verify ADB connection
adb devices

# Push binary to device
adb push build/test_verbovivo_convergence_arm64 /data/local/tmp/

# Make executable
adb shell chmod +x /data/local/tmp/test_verbovivo_convergence_arm64
```

### 3. Execute Test on Device

```bash
# Run the test
adb shell /data/local/tmp/test_verbovivo_convergence_arm64

# Expected output:
# === Verbovivo Convergence Test ===
# ✓ T^7 toroid built with 42 attractors
# ✓ Graph coherence validated
# ✓ Starting at attractor 0
# ✓ Convergence walk completed
#   Status: ATTRACTOR
# ✓ φ within bounds [0, 0x10000]
# ✓ Attractor ID consistent
# === All tests passed ===
```

### 4. Check Exit Code

```bash
adb shell /data/local/tmp/test_verbovivo_convergence_arm64 && echo "✅ Test PASSED" || echo "❌ Test FAILED"
```

## Option B: Standalone Termux Build (No Host Cross-Compiler)

### 1. On Device (Termux Terminal)

```bash
# Install build tools in Termux
pkg install clang make

# Clone or sync repository
cd ~/termux-app-rafacodephi

# Build library
make -f Makefile.verbovivo verbovivo-lib

# Build test
clang -ffreestanding -nostdlib -nostdinc \
  -O2 -Wall -march=armv8-a+simd \
  -I. \
  tests/native/test_verbovivo_convergence.c \
  build/libverbovivo_graph.a \
  -o test_verbovivo_convergence
```

### 2. Run Directly

```bash
./test_verbovivo_convergence
```

## Option C: Integration into Termux APK Build

### 1. Add Verbovivo to APK Compilation

Edit `.github/workflows/ci.yml`:

```yaml
# In the ρ ARM64 Debug job
- name: Build Verbovivo Library
  run: |
    make -f Makefile.verbovivo verbovivo-lib
    
- name: Link Verbovivo into APK
  run: |
    # Copy library to app's native build directory
    mkdir -p app/src/main/cpp/verbovivo
    cp build/libverbovivo_graph.a app/src/main/cpp/verbovivo/
```

### 2. Update Android.mk

```makefile
# app/src/main/cpp/Android.mk
LOCAL_PATH := $(call my-dir)

# Verbovivo graph library
include $(CLEAR_VARS)
LOCAL_MODULE := libverbovivo_graph
LOCAL_SRC_FILES := verbovivo/libverbovivo_graph.a
include $(PREBUILT_STATIC_LIBRARY)

# Link into main native lib
# ... existing rules ...
LOCAL_STATIC_LIBRARIES += libverbovivo_graph
```

### 3. Build APK

```bash
./gradlew :app:assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## Validation Checklist

Run through each check after executing the test:

- [ ] Binary executes without segfault
- [ ] T^7 toroid constructs with 42 attractors
- [ ] Graph coherence validation passes
- [ ] Convergence walk completes (any status 0-3)
- [ ] φ value is within Q16 bounds [0, 0x10000]
- [ ] Attractor ID is consistent (0-41 for convergence, 255 otherwise)
- [ ] Exit code is 0 (all tests passed)

## Interpreting Test Results

### Success (Exit Code 0)
```
✓ T^7 toroid built with 42 attractors
✓ Graph coherence validated
✓ Starting at attractor 0
✓ Convergence walk completed
  Status: ATTRACTOR  (or STABLE, TIMEOUT, etc.)
✓ φ within bounds [0, 0x10000]
✓ Attractor ID consistent
=== All tests passed ===
```

### Failure: φ Out of Bounds (Exit Code 1)
```
FAIL: φ out of Q16 bounds
```
**Diagnosis:** φ computation error; check entropy/coherence metrics

### Failure: Attractor Inconsistency (Exit Code 1)
```
FAIL: Attractor ID invalid for convergence
```
**Diagnosis:** Convergence status doesn't match attractor ID; check convergence logic

### Failure: Coherence Check (Exit Code 1)
```
FAIL: Coherence check failed
```
**Diagnosis:** Graph structure or attractor seeding issue; verify T^7 coordinate mapping

## Performance Metrics (Expected on Device)

| Metric | Expected Value | Notes |
|--------|---|---|
| Graph construction time | <10ms | Depends on device CPU |
| Convergence iterations | 10-100 | Typical walk to attractor |
| φ computation | <1ms | 128-lane popcount |
| Total test runtime | <100ms | All checks included |

## Debugging on Device

### Enable Verbose Logging

Modify `tests/native/test_verbovivo_convergence.c` to add detailed output:

```c
/* Add after convergence walk */
write_str("  φ value (hex): 0x");
for (int i = 15; i >= 0; i--) {
    char nibble = "0123456789abcdef"[(phi_result >> (i * 4)) & 0xF];
    write_syscall(2, &nibble, 1);
}
write_str("\n");
```

### Use logcat for System Logs

```bash
# Capture system logs during test
adb logcat -c  # Clear log buffer
adb shell /data/local/tmp/test_verbovivo_convergence_arm64
adb logcat | grep -i "verbovivo\|convergence\|φ"
```

## Integration with RAFAELIA Bootstrap

Once device testing passes, integrate into bootstrap:

1. **Convergence Receipt Generation**
   ```c
   ConvergenceReceipt receipt;
   int status = vv_bootstrap_convergence_gate(&graph, &bootstrap_receipt, &receipt);
   if (status == 0) {
       // φ validated; proceed with bootstrap
       log_receipt_text(&receipt);  // Logs via SVC #0
   } else {
       // Validation failed; reject bootstrap (fail-closed)
       return -1;
   }
   ```

2. **Receipt Signing**
   - Serialize receipt to binary
   - Sign with EdDSA key
   - Attach to APK manifest or runtime config

3. **Device Attestation**
   - Verify receipt on physical device
   - Cross-check φ values against expected ranges
   - Log receipt to logcat for forensics

## Troubleshooting

### Binary Not Found After Push
```bash
# Verify push succeeded
adb shell ls -lh /data/local/tmp/test_verbovivo_convergence_arm64

# If missing, try alternative path
adb push build/test_verbovivo_convergence_arm64 /data/data/com.termux/files/tmp/
```

### Permission Denied on Execution
```bash
# Ensure executable bit is set
adb shell chmod 755 /data/local/tmp/test_verbovivo_convergence_arm64
```

### Segmentation Fault (SIGSEGV)
- Graph allocation exceeded (max 256 nodes × ~1KB per node)
- Stack overflow (stack size in Termux may be smaller than expected)
- Unaligned memory access (ensure Q16 math is correct)

**Workaround:** Reduce graph size or intermediate node count

### φ Value Incorrect
- Verify H_norm and C_norm computation
- Check entropy metric (bit-counting via popcount)
- Validate coherence seed (0x4050302010080402)
- Ensure Q16 shifts are correct: `((1 - H) * C) >> 16`

## Related Documentation

- `VERBOVIVO_README.md` — Architecture and design
- `Makefile.verbovivo` — Build system
- `tests/native/test_verbovivo_convergence.c` — Test source
- `docs/00_BUG_MASTER_INDEX.md` — Known issues and constraints

## Success Criteria

✅ Test runs without crashing  
✅ All 42 attractors created  
✅ Graph coherence verified  
✅ Convergence walk completes  
✅ φ bounds validated [0, 0x10000]  
✅ Attractor consistency checked  
✅ Exit code 0 (all checks passed)  

Once all criteria are met, Verbovivo is production-ready for RAFAELIA bootstrap receipt generation.
