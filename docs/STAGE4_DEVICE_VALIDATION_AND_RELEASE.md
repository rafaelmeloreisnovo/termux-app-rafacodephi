# Stage 4: Device Validation & Release — Implementation Summary

**Status:** IN_PROGRESS  
**Phase:** Days 11-14 (Bootstrap Gap Implementation — Final Stage)  
**Objective:** Execute physical device validation and determine release readiness

---

## Overview

Stage 4 represents the final phase of the bootstrap gap implementation roadmap. It validates the complete bootstrap pipeline on actual Android devices across ARM32/ARM64 architectures, generates runtime receipts, and establishes release readiness criteria.

---

## Deliverables (Days 11-14)

### Day 11: Device Lab Setup

#### 1. Device Validation Contract (`configs/device-validation-contract.json`)

**Purpose:** Defines complete device testing matrix, scenarios, and release criteria

**Key Sections:**

**Testing Matrix:**
```json
{
  "platforms": [
    {
      "arch": "aarch64",
      "abi": "arm64-v8a",
      "min_api": 24,
      "devices": ["Pixel 6+", "Galaxy S21+", "OnePlus 9"]
    },
    {
      "arch": "armv7l",
      "abi": "armeabi-v7a",
      "min_api": 21,
      "devices": ["Pixel 4", "Galaxy S10", "OnePlus 6T"]
    }
  ]
}
```

**Bootstrap Probe APK:**
- Minimal ~500KB APK for runtime validation
- Native bootstrap validator components
- Receipt logger for runtime proof
- Coherence validator for phi_fst at runtime
- Supports both ARM64 and ARMv7

**Test Scenarios (10 total):**

| Scenario | Platform | Purpose | Timeout |
|----------|----------|---------|---------|
| boot-001 | Both | PREFIX_EMPTY initialization | 5s |
| boot-002 | Both | PROOT_INITIALIZED | 10s |
| boot-003 | Both | PAYLOAD_EXTRACTED | 15s |
| boot-004 | Both | DPKG_INSTALLED | 10s |
| boot-005 | Both | APT_CONFIGURED | 10s |
| boot-006 | Both | USER_PACKAGES_READY | 10s |
| cohere-001 | Both | phi_fst validation | 5s |
| determ-001 | Both | Multi-boot (3 reboots) | 120s |
| sig-001 | Both | Package signatures | 30s |
| perf-001 | Both | Performance < 60s | 90s |

**Validation Gates (4 total):**
- G-DEV-001: ARM64 bootstrap gate
- G-DEV-002: ARMv7 bootstrap gate
- G-DEV-003: Coherence runtime gate
- G-DEV-004: Determinism multi-boot gate

### Days 12-13: Device Testing

#### 2. Bootstrap Probe Controller (`scripts/device_bootstrap_probe.py`)

**Purpose:** Manage bootstrap probe execution on physical devices via ADB

**Key Classes:**
- `DeviceBootstrapProbe` — Main controller for device interaction

**Key Methods:**
- `is_connected()` — Check ADB device connectivity
- `get_device_info()` — Retrieve device metadata (API, arch, version)
- `push_probe_apk(path)` — Deploy APK to device
- `install_apk(name)` — Install APK with permissions
- `start_bootstrap_validator()` — Launch validator activity
- `fetch_device_logs(tag)` — Retrieve logcat output
- `fetch_receipt(path)` — Pull bootstrap receipt from device
- `execute_bootstrap_scenario(scenario)` — Run test scenario
- `generate_device_receipt(results)` — Create runtime receipt

**Usage:**
```bash
# List connected devices
python3 scripts/device_bootstrap_probe.py --list-devices

# Probe specific device
python3 scripts/device_bootstrap_probe.py --probe <SERIAL>

# Execute scenario on device
python3 scripts/device_bootstrap_probe.py --execute-scenario boot-001 <SERIAL>
```

**Output:** JSON results per scenario with:
- Scenario ID and name
- Pass/fail/inconclusive status
- Duration in seconds
- Log evidence (last 20 lines)

### Day 14: Receipt Generation & Release Readiness

#### 3. Runtime Receipt Schema

**File:** `configs/device-validation-contract.json` (sample receipt)

**Schema:** `raf.device-bootstrap-receipt.v1`

**Structure:**
```json
{
  "device_id": "SHA256(model+serial)",
  "platform": "arm64-v8a",
  "android_version": 13,
  "api_level": 33,
  "timestamp": "2026-08-29T12:34:56Z",
  "bootstrap_stages": [
    {
      "stage": 0,
      "stage_name": "PREFIX_EMPTY",
      "duration_ms": 100,
      "crc32c": "0xdeadbeef",
      "status": "PASS",
      "phi_fst": 0.4567,
      "attractor": 12
    }
  ],
  "total_duration_seconds": 45,
  "overall_coherence": 0.4523,
  "reproducibility_score": 0.98,
  "exit_code": 0,
  "errors": []
}
```

#### 4. Release Readiness Criteria (10 total)

**Blocking Criteria (RC-001 to RC-004):**

| Criterion | Status | Blocker |
|-----------|--------|---------|
| RC-001: ARM64 bootstrap passes | BLOCKED | Device validation not executed |
| RC-002: ARMv7 bootstrap passes | BLOCKED | Device validation not executed |
| RC-003: Coherence ✓ both archs | BLOCKED | phi_fst runtime not executed |
| RC-004: Reproducibility ≥ 0.95 | BLOCKED | Multi-boot testing not executed |

**Planned Criteria (RC-005 to RC-009):**

| Criterion | Evidence Required |
|-----------|------------------|
| RC-005: Signature validation | Device receipt: signature_validation: PASS |
| RC-006: Performance < 60s | Device receipt: total_duration_seconds < 60 |
| RC-007: No timeout triggers | Device receipt: retry_count < 2 per stage |
| RC-008: No security rollbacks | Device receipt: rollback_count = 0 |
| RC-009: APK signature chain | Device receipt: signature_validation: PASS |

**Manual Gate (RC-010):**
- Code review and security audit complete

---

## Testing Execution Plan

### Day 11: Lab Preparation
```bash
# Setup device connectivity
adb devices

# Flash bootstrap probe APK
adb push termux-bootstrap-probe.apk /data/local/tmp/
adb install -r -g termux-bootstrap-probe.apk

# Verify device capabilities
python3 scripts/device_bootstrap_probe.py --probe <SERIAL>
```

### Days 12-13: Scenario Execution (2 architectures × 10 scenarios)

**ARM64 (Target API 24+):**
```bash
for scenario in boot-001 boot-002 boot-003 boot-004 boot-005 boot-006 cohere-001 determ-001 sig-001 perf-001; do
    python3 scripts/device_bootstrap_probe.py --execute-scenario $scenario <ARM64_SERIAL>
done
```

**ARMv7 (Target API 21+):**
```bash
for scenario in boot-001 boot-002 boot-003 boot-004 boot-005 boot-006 cohere-001 determ-001 sig-001 perf-001; do
    python3 scripts/device_bootstrap_probe.py --execute-scenario $scenario <ARMv7_SERIAL>
done
```

**Multi-Boot Determinism (determ-001):**
```bash
# Execute on each architecture 3 times
# Device reboots between runs
# Compare receipts for identical CRC32C/state hashes
```

### Day 14: Receipt Generation & Decision
```bash
# Aggregate results from all devices
# Generate unified device receipt
# Verify all release criteria met
# Make release readiness decision
```

---

## Rollback Conditions

Any of the following triggers immediate rollback to Stage 3 (no release):

1. Any bootstrap stage timeout (watchdog > 30 seconds)
2. Any stage retry count exceeds 2
3. Coherence phi_fst outside [0, 1]
4. Reproducibility score < 0.90
5. Package signature validation failure
6. glibc dependency detected at runtime
7. Global /data/data/com.termux reference detected
8. APT non-deterministic mirror selection observed

---

## Success Metrics

| Metric | Target | Threshold |
|--------|--------|-----------|
| Pass Rate | 100% | All scenarios on all architectures |
| Coherence | phi_fst ≥ 0.3 | Convergence to T^7 attractor |
| Determinism | reproducibility_index ≥ 0.95 | Cross-reboot consistency |
| Performance | Bootstrap < 60 seconds | Total execution time |
| Timeouts | Zero | No watchdog triggers |

---

## Device Lab Requirements

**Hardware:**
- ARM64 devices: Pixel 6+, Galaxy S21+, OnePlus 9 (or equivalents)
- ARMv7 devices: Pixel 4, Galaxy S10, OnePlus 6T (or equivalents)
- Minimum 2 devices per architecture (higher is better)

**Software:**
- Android 8.0+ (API 26+) on test devices
- ADB (Android Debug Bridge) tools
- USB connectivity or network ADB

**Network:**
- Package manager requires network access (for signature verification)
- Termux mirror access for package installation

**Time Allocation:**
- Day 11: 2 hours (setup + verification)
- Day 12: 3 hours (10 scenarios × 2 archs)
- Day 13: 4 hours (determinism multi-boot, 120 seconds × 6 runs)
- Day 14: 2 hours (receipt generation + decision)
- **Total: ~11 hours of device lab time**

---

## Known Limitations

1. **Physical Devices Required** — Cannot be simulated in CI
2. **Device Availability** — Testing depends on lab access
3. **Network Access** — Package manager features require connectivity
4. **Multi-Boot Testing** — Requires extended uninterrupted device access
5. **Timing Sensitivity** — Determinism depends on stable runtime conditions

---

## CI Integration (Simulated)

For CI environments without physical devices:

```yaml
name: Stage 4 - Device Validation (Simulated)

jobs:
  device-validation-simulation:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Validate device contract
        run: |
          python3 -c "
          import json
          with open('configs/device-validation-contract.json') as f:
              contract = json.load(f)
              assert contract['stage'] == 'Stage 4: Device Validation & Release'
              assert len(contract['test_scenarios']) == 10
              assert len(contract['release_criteria']) == 10
          print('Device contract valid')
          "
      - name: Generate simulated receipts
        run: |
          # Generate placeholder receipts for CI validation
          mkdir -p results/device-receipts
          # (Simulation: create sample receipts)
```

---

## Release Decision Matrix

| Condition | ARM64 | ARMv7 | Decision |
|-----------|-------|-------|----------|
| All scenarios PASS | ✓ | ✓ | **READY FOR RELEASE** |
| All scenarios PASS | ✓ | ⚠️ (1 INCONCLUSIVE) | REQUEST REVIEW |
| Any scenario FAIL | ✓ | ✗ | **BLOCKED — ROLLBACK TO STAGE 3** |
| Coherence FAIL | ✓ | ✓ | **BLOCKED — INVESTIGATE phi_fst** |
| Determinism < 0.90 | ✓ | ✓ | **BLOCKED — NON-REPRODUCIBLE** |

---

## Deliverables Checklist

- [ ] Device validation contract created (`configs/device-validation-contract.json`)
- [ ] Bootstrap probe APK built and tested
- [ ] Probe controller deployed (`scripts/device_bootstrap_probe.py`)
- [ ] Device lab setup and connectivity verified
- [ ] 10 scenarios executed on ARM64
- [ ] 10 scenarios executed on ARMv7
- [ ] Device receipts generated and archived
- [ ] All release criteria evaluated
- [ ] Release readiness decision made
- [ ] Final report generated

---

## Timeline

| Date | Phase | Deliverable |
|------|-------|-------------|
| Day 11 | Lab Setup | Contract + Probe ready |
| Day 12 | ARM64 Testing | 10 scenarios on ARM64 |
| Day 13 | Full Testing | ARMv7 + Multi-boot |
| Day 14 | Decision | Release/Rollback decision |

---

## Next Steps (After Release Decision)

**If READY FOR RELEASE:**
1. Tag commit as release candidate
2. Generate release notes
3. Create GitHub Release with device receipts
4. Archive device test logs

**If ROLLBACK TO STAGE 3:**
1. Analyze failure logs
2. Identify root cause
3. Plan corrective action
4. Re-run Stage 3 validation gates
5. Schedule Stage 4 retry

---

## References

- Device Contract: `configs/device-validation-contract.json`
- Probe Controller: `scripts/device_bootstrap_probe.py`
- Bootstrap Contract (Stage 1): `configs/bootstrap-contract.json`
- Package Manager Contract (Stage 2): `configs/package-manager-contract.json`
- Coherence Gate (Stage 3): `scripts/import_coherence_gate.py`
- Determinism Gate (Stage 3): `scripts/import_determinism_gate.py`
