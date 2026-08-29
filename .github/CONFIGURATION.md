# GitHub Actions CI Configuration

**Last Updated:** 2026-08-29  
**Repository:** `rafaelmeloreisnovo/termux-app-rafacodephi`

---

## Current Setup Status

| Component | Status | Notes |
|-----------|--------|-------|
| **Workflows Configured** | ✅ Yes | `ci.yml` with safety gates |
| **Local Gate Execution** | ✅ All Pass | BUG-01, 03, 06, 08 validated locally |
| **GitHub Actions Runners** | ✅ Available | Ubuntu latest available |
| **Build Tools** | ✅ Partial | clang/gcc available; no full NDK |
| **Device Testing** | 🔴 Blocked | Requires physical Android hardware |

---

## Workflow Files

### 1. `.github/workflows/ci.yml` — Safety Cascade Gates

**Trigger:** Push to master or claude/* branches, PRs to master  
**Job:** `safety-gates` (ubuntu-latest runner)

**Steps:**
1. Checkout code
2. Install build tools (build-essential, clang, gcc-multilib)
3. Syntax check (ARM64 freestanding)
4. Execute all 4 closure gates:
   - BUG-01: Attractor table completeness
   - BUG-03: AArch64 assembly fixes
   - BUG-08: Lyapunov convergence validation
   - BUG-06: CtiScanner race condition (memory barriers)
5. Validate documentation structure
6. Report combined status

**Environment Variables:**
- None currently required

**Expected Output:**
```
✓ Syntax check complete
✓ All gates pass (or show environment limitations)
✓ Critical documentation present
```

---

## Why Certain Gates May Show as "Env dep" (Environment Dependent)

### BUG-01: Attractor Table Complete Gate

**Local Execution:** ✅ PASS  
**CI Execution:** May differ depending on runner environment

**Why:** The validator compiles and runs C code that:
- Generates SHA-256 hashes
- Uses standard library math functions
- Validates attractor state coprimality

**In CI:** Should succeed on Ubuntu runners; if it fails, it's a genuine failure (not environmental).

### BUG-03: AArch64 Vectorpulse Gate

**Local Execution:** ✅ PASS  
**CI Execution:** Environment-dependent

**Why:** Validates ARM64 assembly (vectra_pulse.S) with:
- Architecture-specific barriers (dmb ish)
- Register allocation patterns
- Phase space calculations

**In CI:** Ubuntu runners are x86, not ARM64. The validator *checks* for ARM64 compatibility but cannot execute native ARM64 code. Use `-target aarch64-linux-gnu` for cross-compilation validation.

### BUG-08: Lyapunov Convergence Gate

**Local Execution:** ✅ PASS  
**CI Execution:** May succeed (uses standard library math)

**Why:** Pure math validation (no architecture-specific code)
- Computes φ = (1-H)·C
- Validates bounds [0, 1]
- No device-specific dependencies

**In CI:** Should pass on any Linux system with math library.

### BUG-06: CtiScanner Race Condition Gate

**Local Execution:** ✅ PASS (with pthread support)  
**CI Execution:** Depends on pthread availability

**Why:** Multi-threaded validator using POSIX pthreads
- 4 concurrent threads
- 1000 iterations each
- Memory barrier protection

**In CI:** Ubuntu runners have pthread; should execute successfully.

---

## Gate Pass Criteria

Each gate succeeds when:

1. **BUG-01:** 41 attractors generated, gcd(Δr, 41)=1 verified, SHA-256 matches expected
2. **BUG-03:** ARM64 assembly loads correctly, no undefined symbols, validates phase calculations
3. **BUG-08:** φ ∈ [0, 1] bounds enforced, convergence assertion passes
4. **BUG-06:** 4 threads complete without deadlock, barrier ordering verified, exit code 0

---

## Local Validation

To run gates locally (before CI):

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential clang libm-dev

# Run all gates
make attractor-table-complete-gate     # BUG-01
make aarch64-vectorpulse-gate          # BUG-03
make lyapunov-convergence-gate         # BUG-08
make cti-race-condition-gate           # BUG-06

# Expected: All exit code 0
```

---

## Secrets and Environment Variables

Currently, **no secrets required** for CI jobs.

### For Future (Release Builds)

When signing configuration is added, these secrets will be needed:

```
KEYSTORE_FILE         — Base64-encoded .keystore file
KEYSTORE_PASSWORD     — Password for keystore
KEY_PASSWORD          — Password for signing key
PLAY_CONSOLE_TOKEN    — (Optional) For Play Store deployment
```

**Status:** TBD (not configured yet)

---

## CI Status and Observability

### Visible in PR Checks

When a PR is created, GitHub will show:

```
✓ Safety Gates CI / safety-gates
```

Clicking through shows:
- Step-by-step execution log
- Gate output (pass/fail)
- Environment diagnostics
- Suggested next steps

### Status Page

For all checks on the repository, visit:
```
https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions
```

---

## Known Limitations

### Cannot Execute in CI

| Capability | Reason | Workaround |
|-----------|--------|-----------|
| Native ARM64 code | Ubuntu runners are x86 | Cross-compile checks only |
| Android NDK builds | Not installed (3+ GB) | Use local build or Docker |
| Device testing | No physical devices | Manual device validation |
| APK signing | Keystore not configured | Local signing + manual upload |
| Vectra QEMU testing | QEMU not available | Future (separate workflow) |

### Informational Only (Not Failures)

The following gates may show warnings in CI but are **not failures**:

- "Gate requires specific build environment" — Expected on generic runners
- "ARM64 assembly validated for cross-compilation" — Not a failure
- "pthread validation completed" — Success indicator, not failure

---

## Troubleshooting CI Failures

### If a gate fails in CI but passes locally:

1. **Check runner environment:**
   ```bash
   # In CI job output, look for:
   # - Compiler version
   # - Available libraries
   # - Architecture
   ```

2. **Reproduce on similar system:**
   ```bash
   # Docker (Ubuntu 20.04 equivalent):
   docker run -it ubuntu:20.04 bash
   apt-get update && apt-get install -y build-essential clang
   # Clone repo and run gates
   ```

3. **Check for environment variables:**
   ```bash
   env | grep -i "CI\|GITHUB\|PATH\|LD"
   ```

### If a gate fails in both CI and local:

1. **This is a real bug** (not CI infrastructure)
2. Review the gate's validation logic
3. Check for missing dependencies
4. Consult the bug's implementation record (docs/BUGXX_IMPLEMENTATION_RECORD.md)

---

## Future CI Enhancements

- [ ] APK build workflow (requires NDK setup)
- [ ] Device validation (requires hardware)
- [ ] Artifact storage (APK uploads)
- [ ] Release signing (Keystore configuration)
- [ ] Performance benchmarking
- [ ] Coverage reporting

---

## Related Documentation

- `docs/SAFETY_CASCADE_COMPLETION.md` — All 8 bugs resolved
- `docs/F_GAP_CLOSURE_ROADMAP.md` — Phase 2 (device validation)
- `docs/00_BUG_MASTER_INDEX.md` — Bug registry and status
- `AGENTS.md` — Repository authority and governance
- `CLAUDE.md` — Claude Code adapter configuration

---

**Document:** GitHub Actions CI Configuration  
**Status:** ✅ CONFIGURED (Phase 1 complete)  
**Next:** Phase 2 (device validation) when hardware available
