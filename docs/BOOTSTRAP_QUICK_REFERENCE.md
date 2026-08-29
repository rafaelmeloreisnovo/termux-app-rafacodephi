# Verbovivo Bootstrap — Quick Reference

**Purpose:** One-page reference for understanding and working with the Verbovivo bootstrap integration.

## What is Verbovivo Bootstrap?

Convergence engine that computes three metrics at Termux app startup:
- **H** (Entropy): System disorder [0-100%]
- **C** (Coherence): KAM-7 alignment [0-100%]  
- **φ** (Convergence): Combined metric = (1-H)·C

If validation fails → **app doesn't start** (fail-closed).

## File Map

```
VerbativoBootstrapManager.java
    ↓ calls
VerbativoCore.java (JNI wrapper)
    ↓ calls
verbovivo_jni.c (C bridge)
    ↓ calls
libverbovivo_graph (native library)
    ├── verbovivo_graph.c (T^7 toroid)
    ├── t7_toroid_builder.c (init)
    └── verbovivo_bootstrap_gate.c (validation)

Integration point:
TermuxActivity.java onCreate()
    → VerbativoBootstrapManager.executeBootstrap()
    → if fails → return (block startup)
```

## Key Classes & Methods

### VerbativoCore (JNI wrapper)

```java
// Check if initialized
VerbativoCore.isInitialized() → boolean

// Execute convergence walk
VerbativoCore.executeConvergence() → long (φ_fst in Q16)

// Compute all three metrics
VerbativoCore.computePhiMetrics(long[] out) → 0 (success) or -1
  // out[0] = H_norm, out[1] = C_norm, out[2] = φ_fst

// Generate receipt
VerbativoCore.generateConvergenceReceipt() → byte[26] or null

// Validate receipt
VerbativoCore.validateConvergenceReceipt(byte[] receipt) → 0 (valid) or -1
```

### VerbativoBootstrapManager (lifecycle)

```java
// Main entry point
VerbativoBootstrapManager bootstrap = new VerbativoBootstrapManager(context);
boolean passed = bootstrap.executeBootstrap();

// Inside executeBootstrap():
// 1. Check initialized
// 2. Generate receipt
// 3. Validate receipt  
// 4. Store receipt
// 5. Return true only if all pass
```

## Receipt Format (26 bytes)

```
[0-7]   H_norm (uint64, big-endian, 0 = 0%, 0x10000 = 100%)
[8-15]  C_norm (uint64, big-endian)
[16-23] φ_fst (uint64, big-endian)
[24]    attractor_id (0-41=converged, 255=not found)
[25]    status (0=attractor, 1=stable, 2=no-edges, 3=timeout)
```

**Storage:** `appFilesDir/verbovivo/convergence_receipt.bin`

## Metrics Validation

**All three must pass:**
1. H ∈ [0%, 100%] (entropy in bounds)
2. C ∈ [0%, 100%] (coherence in bounds)
3. φ ∈ [0%, 100%] (convergence in bounds)
4. φ ≈ (1-H)·C (formula satisfied within ±5%)

**Attractor consistency:**
- If attractor < 42 → status MUST be 0
- If attractor == 255 → status MUST NOT be 0

## Troubleshooting

### App Won't Start
```
Error: "Verbovivo bootstrap validation FAILED"
→ Check: logcat for "VerbativoBootstrap" or "VerbativoCore"
→ Check: φ values in bounds
→ Check: φ formula satisfied
```

### JNI Load Error
```
Error: "UnsatisfiedLinkError"
→ Check: libverbovivo_graph compiled with correct ABI
→ Check: Android.mk includes all source files
→ Check: Linker flags correct (16KB alignment)
```

### Receipt Not Generated
```
Error: Receipt file missing
→ Check: executeConvergence() returns valid φ
→ Check: computePhiMetrics() succeeds
→ Check: appFilesDir/verbovivo/ directory writable
```

### CI Build Failure
```
Error: "cannot find source file"
→ Check: Android.mk paths (should be ../../../ from cpp/)
→ Fix: Verify relative paths resolve correctly
```

## Common Tasks

### Check Bootstrap on Device
```bash
adb logcat | grep -E "(VerbativoBootstrap|VerbativoCore)"
```

### Extract Receipt
```bash
adb shell run-as com.termux.rafacodephi \
  hexdump -C files/verbovivo/convergence_receipt.bin
```

### Parse Receipt Metrics
```python
import struct
with open('receipt.bin', 'rb') as f:
    data = f.read(26)
H = struct.unpack('>Q', data[0:8])[0] / 0x10000 * 100
C = struct.unpack('>Q', data[8:16])[0] / 0x10000 * 100
phi = struct.unpack('>Q', data[16:24])[0] / 0x10000 * 100
print(f"H={H:.1f}% C={C:.1f}% φ={phi:.1f}%")
```

### Build APK for Testing
```bash
./gradlew :app:assembleDebug -Pandroid.injected.build.abi=arm64-v8a --no-daemon
```

### Run Automated Validation
```bash
./scripts/validate_verbovivo_device.sh --verbose
```

## Performance Notes

- **Bootstrap time:** ~50-100ms on typical device
- **Memory footprint:** ~2-5MB (T^7 graph)
- **Receipt computation:** ~10-20ms per execution
- **Convergence iterations:** Typically 5-20 steps

## Security Notes

1. **Fail-closed:** Bootstrap failure blocks app startup (no fallback)
2. **Receipt validation:** All 26 bytes checked, no partial acceptance
3. **Metric bounds:** Out-of-range values cause validation failure
4. **No secrets:** Receipt contains no keys/tokens/PII
5. **Immutable:** Receipt generated at startup, never re-generated during app lifetime

## Dependencies

- Android API 24+ (Android 7.0)
- ARM64 or ARM32 architecture
- JNI available
- Native library compiled with NDK
- 16KB page alignment support (Android 15+)

## Related Documentation

- `VERBOVIVO_DEVICE_TEST_PLAN.md` — 6-phase testing plan
- `VERBOVIVO_COMPLETE_TESTING_GUIDE.md` — Full procedures
- `BOOTSTRAP_INTEGRATION_VALIDATION.md` — Validation checklist
- `STATUS.md` — Project status tracking
- `CLAUDE.md` — Project instructions

---

**Version:** 1.0  
**Last Updated:** 2026-08-29
