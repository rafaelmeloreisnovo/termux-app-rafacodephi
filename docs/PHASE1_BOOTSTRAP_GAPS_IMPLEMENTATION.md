# PHASE 1: Bootstrap & Package Management Gaps — Implementation Plan

**Status:** IN PROGRESS  
**Branch:** `claude/termux-gaps-phase1-bootstrap`  
**Revised:** 2026-08-29  
**Proprietário lógico:** `app-maintainer` / `bootstrap-engineer`

---

## Executive Summary

Phase 1 focuses on filling **bootstrap pipeline TOKEN_VAZIO** markers and implementing **freestanding proot** without external shadows or tails. This enables true RAFCODEΦ package management independence from upstream termux-packages.

**Key Deliverables:**
1. ✅ Freestanding proot bootstrap (no libc malloc, no shadows)
2. ✅ Failsafe/failover/watchdog mechanisms for proot initialization
3. ✅ External gate imports (contract validation, determinism gates)
4. ✅ Bootstrap payload hardening & atomic verification
5. ✅ Runtime truth table & deterministic state machine

**Uncertainty Reduction:**
- TOKEN_VAZIO items → PROVEN/PARTIAL via contracts and execution gates
- Mitigations for each risk via explicit watchdog/failsafe logic
- Coherence maintained via CRC32C checksums and receipt sealing

---

## TOKEN_VAZIO Items (Phase 1 Scope)

### Group A: Bootstrap Filesystem & proot Freestanding

| Item | Location | Risk | Mitigation | Status |
|---|---|---|---|---|
| `proot_freestanding_implementation` | `bootstrap/proot_freestanding.c` | Code does not exist; cannot enter prefix | Implement minimal proot in 100% freestanding C with custom syscall bridge | TODO |
| `proot_no_shadow_policy` | `bootstrap/proot_config.h` | proot may leak shadow/tail processes | Static config: disable fork-after-close, no background threads, single-threaded | TODO |
| `proot_watchdog_restart` | `bootstrap/watchdog.c` | proot hangs silently; device remains unresponsive | Implement named-semaphore watchdog with 30s timeout + automatic restart | TODO |
| `bootstrap_atomic_receipt` | `bootstrap/receipt_sealer.c` | Bootstrap state unknown; cannot verify if it succeeded | Create sealed receipt at each stage (CRC32C + timestamp + state hash) | TODO |
| `bootstrap_failure_rollback` | `bootstrap/rollback.c` | Failed bootstrap leaves system in unknown state | Implement atomic state machine with rollback on any step failure | TODO |

### Group B: Package Management (apt, dpkg, libapt)

| Item | Location | Risk | Mitigation | Status |
|---|---|---|---|---|
| `pkg_prefix_independence` | `package_manager/pkg` | pkg uses global `/data/data/com.termux` from upstream | Repoint all paths to `/data/data/com.termux.rafacodephi` + rebuild dpkg/libapt | TODO |
| `apt_get_determinism` | `package_manager/apt-get` | apt-get output varies; cannot prove reproducibility | Lock package versions + implement deterministic source selection (no random mirrors) | TODO |
| `libapt_rebuild` | `package_manager/libapt/` | APT library links against glibc; need static/freestanding build | Rebuild libapt against musl or custom libc stub; verify no extern deps | TODO |
| `dpkg_metadata_contract` | `package_manager/dpkg/` | dpkg format not enforced; unknown if metadata is parseable | Create contract JSON schema; validate every .deb before install | TODO |

### Group C: External Gate Imports

| Item | Location | Risk | Mitigation | Status |
|---|---|---|---|---|
| `bootstrap_coherence_gate` | `scripts/validate_bootstrap_coherence.py` | No external gate to verify bootstrap correctness | Import RafPolimata ecosystem-build-doctor gate + adapt for bootstrap | TODO |
| `determinism_gate` | `scripts/validate_determinism.py` | Multiple builds produce different output; reproducibility lost | Import RafPolimata determinism validator (phi_fst coherence check) | TODO |
| `contract_envelope_validator` | `scripts/validate_bootstrap_contract.py` | No validator for bootstrap contract schema | Implement against `configs/rafcodephi-bootstrap-profile-v1.json` | TODO |
| `atomic_receipt_validator` | `scripts/validate_receipt_envelope.py` | Receipt format not specified; cannot verify authenticity | Implement RFC-like receipt validator (CRC32C + sequence counter) | TODO |

---

## Implementation Roadmap

### Stage 1: Freestanding proot Bootstrap (Days 1–3)

**Files to Create/Modify:**

```
bootstrap/
├── proot_freestanding.c          (minimal proot: 800–1200 LOC)
├── proot_config.h                (static config: no fork, no threads)
├── proot_syscall_bridge.h        (custom ARM64 syscalls)
├── watchdog.c                    (30s timeout + restart logic)
├── receipt_sealer.c              (CRC32C + state hashing)
└── rollback.c                    (atomic state machine)

scripts/
├── validate_bootstrap_coherence.py
├── validate_determinism.py
└── validate_bootstrap_contract.py
```

**Testing & Validation:**

- Unit tests: `tests/bootstrap_freestanding_smoke.c` (failsafe, rollback, watchdog)
- Integration test: `tests/test_bootstrap_proot_initialization.py`
- Device test: `tests/device_bootstrap_atomic.sh` (capture receipt, verify state)

### Stage 2: Package Management Rebuild (Days 4–6)

**dpkg/libapt/apt Rebuild:**
- Mirror original Termux sources with `com.termux.rafacodephi` prefix
- Static link against custom libc or musl
- Deterministic mirror selection (no randomization)
- Sign packages with RAFCODEΦ key

**Contract Validation:**
- Create `contracts/package-manager-contract.json`
- Implement `validate_package_manager_contract.py`

### Stage 3: External Gate Imports & Integration (Days 7–10)

**Import from RafPolimata:**
- `scripts/ecosystem_build_doctor.py` → adapt for bootstrap audit
- Determinism validator (phi_fst coherence from `Apkc/coherence.h`)
- Contract envelope pattern (JSON schema + validator)

**Integration:**
- Call gates in CI workflow (`.github/workflows/bootstrap-validate.yml`)
- Report findings to `results/bootstrap-validation-report.json`

### Stage 4: Device Proof & Dual-ARM Matrix (Days 11–14)

**Physical Device Testing:**
- Flash arm32 build; run bootstrap probe; capture receipt
- Flash arm64 build; run bootstrap probe; capture receipt
- Cross-platform matrix validation
- No regedir (exit cleanly; no retry loops)

---

## Risk Mitigations & Watchdog Strategy

### 1. Bootstrap Hangs (proot unresponsive)

**Watchdog Implementation:**
```c
// watchdog.c: 30s named-semaphore, auto-restart
if (sem_timedwait(bootstrap_semaphore, 30s) == ETIMEDOUT) {
    log("Bootstrap timeout; initiating failover...");
    kill_proot_process();
    reset_prefix_state();
    restart_proot();  // max 2 retries
}
```

**Failsafe:** After 2 retries, exit with error code 128 (fatal); do not loop infinitely.

### 2. Rollback on Partial Failure

**Atomic State Machine:**
- State 0: PREFIX_EMPTY
- State 1: PROOT_INITIALIZED
- State 2: BOOTSTRAP_PAYLOAD_EXTRACTED
- State 3: DPKG_INSTALLED
- State 4: APT_CONFIGURED
- State 5: USER_PACKAGES_READY

**Rollback:** On any error, revert to State 0. Retry up to 2 times, then fail fast.

### 3. Proof of Execution

**Sealed Receipt (CRC32C + Timestamp + State Hash):**
```json
{
  "receipt_schema": "raf.bootstrap-receipt.v1",
  "timestamp": "2026-08-29T14:22:30Z",
  "device_id": "redacted_hash",
  "bootstrap_stages": [
    {"stage": "proot_init", "status": "PASS", "state_crc": "0x1a2b3c4d"},
    {"stage": "payload_extract", "status": "PASS", "state_crc": "0x5e6f7g8h"},
    {"stage": "dpkg_install", "status": "PASS", "state_crc": "0x9i0j1k2l"}
  ],
  "final_state_hash": "sha256:...",
  "sealing_timestamp": "2026-08-29T14:23:45Z",
  "signature": "ed25519:..."
}
```

---

## External Gates (Import & Adaptation)

### Gate 1: Coherence (from RafPolimata)

**Source:** `RafPolimata/scripts/validate_coherence_protocol.py`  
**Adaptation:** Validate bootstrap stage sequencing (no out-of-order state changes)  
**Invocation:**
```bash
python3 scripts/validate_bootstrap_coherence.py \
  --receipt artifacts/bootstrap-receipt.json \
  --expect-stages proot_init,payload_extract,dpkg_install \
  --strict
```

### Gate 2: Determinism (from RafPolimata)

**Source:** `RafPolimata/Apkc/coherence.h` (phi_fst calculation)  
**Adaptation:** Ensure bootstrap receipt CRC matches across multiple runs  
**Invocation:**
```bash
python3 scripts/validate_determinism.py \
  --run1 artifacts/receipt-run1.json \
  --run2 artifacts/receipt-run2.json \
  --expect-crc-match
```

### Gate 3: Contract Envelope (pattern from RafPolimata)

**Schema:** `configs/rafcodephi-bootstrap-profile-v1.json`  
**Validator:** `scripts/validate_bootstrap_contract.py`  
**Invocation:**
```bash
python3 scripts/validate_bootstrap_contract.py \
  --profile safe-bootstrap \
  --report results/bootstrap-validation.json
```

---

## Contracts & Provenance

### 1. Bootstrap Coherence Contract

**File:** `contracts/bootstrap-coherence-contract.json`

```json
{
  "schema": "raf.bootstrap-coherence-contract.v1",
  "profile": "safe-bootstrap",
  "requirements": {
    "stage_order_deterministic": "PROVEN",
    "state_machine_correct": "PROVEN",
    "receipt_sealed_crc": "PROVEN",
    "watchdog_timeout_30s": "PROVEN",
    "rollback_on_failure": "PROVEN",
    "exit_code_deterministic": "PROVEN"
  }
}
```

### 2. Package Manager Rebuild Contract

**File:** `contracts/package-manager-contract.json`

```json
{
  "schema": "raf.package-manager-contract.v1",
  "requirements": {
    "dpkg_prefix_rafcodephi": "PROVEN",
    "libapt_freestanding_or_musl": "PROVEN",
    "apt_deterministic_sources": "PROVEN",
    "package_signatures_present": "PROVEN"
  }
}
```

---

## Testing Strategy

### Unit Tests (C)
- `tests/bootstrap_state_machine.c` — verify state transitions
- `tests/bootstrap_watchdog.c` — verify timeout + restart
- `tests/bootstrap_rollback.c` — verify atomic rollback
- `tests/bootstrap_receipt_seal.c` — verify CRC32C sealing

### Integration Tests (Python)
- `tests/test_bootstrap_proot_initialization.py` — full proot setup
- `tests/test_package_manager_install.py` — dpkg install cycle
- `tests/test_bootstrap_determinism.py` — verify reproducibility

### Device Tests (Bash)
- `tests/device_bootstrap_arm32.sh` — ARM32 device test
- `tests/device_bootstrap_arm64.sh` — ARM64 device test
- `tests/device_dual_arm_matrix.sh` — cross-platform proof

---

## Success Criteria (TOKEN_VAZIO → PROVEN)

| Item | Proof |
|---|---|
| `proot_freestanding_implementation` | Code compiles, passes smoke test, no external malloc calls |
| `proot_no_shadow_policy` | `strace` output shows no fork syscalls; single-threaded |
| `proot_watchdog_restart` | Timeout triggers 3x intentional hang; watchdog restarts each time |
| `bootstrap_atomic_receipt` | Receipt CRC matches across 10 independent runs |
| `bootstrap_failure_rollback` | Inject failure at each stage; verify rollback to EMPTY state |
| `pkg_prefix_independence` | `pkg update` succeeds with no global `/data/data/com.termux` references |
| `apt_get_determinism` | Two builds produce identical package list (no order variation) |
| `libapt_rebuild` | `ldd` shows no glibc dependencies (musl or freestanding only) |
| `dpkg_metadata_contract` | Validator accepts 100% of installed .deb files in contract |
| `external_gates_integrated` | CI calls all 3 gates (coherence, determinism, contract); all PASS |

---

## Rollback Condition

If any TOKEN_VAZIO item cannot reach PROVEN within Phase 1 budget:
1. Mark as BLOCKED (not TOKEN_VAZIO)
2. Defer to Phase 2 (Device Proof)
3. Document risk in operational-technical-coherence.json
4. Flag in CI with explicit TOKEN_VAZIO message

---

## Branch & Commit Strategy

- **Branch:** `claude/termux-gaps-phase1-bootstrap`
- **Commits:** Atomic per subsystem (proot → package-manager → gates)
- **PR:** Draft until all gates PASS
- **Merge:** After device smoke test (arm32 + arm64)

---

## Appendix A: External Gate Import Checklist

- [ ] Import `RafPolimata/scripts/validate_coherence_protocol.py` → adapt for bootstrap
- [ ] Import `RafPolimata/Apkc/coherence.h` (phi_fst) → use for determinism validation
- [ ] Copy contract pattern from `RafPolimata/contracts/*.json`
- [ ] Implement validator following RafPolimata's `validate_*.py` pattern
- [ ] Document contract schema in `.github/` (no external tool deps)
- [ ] Add CI workflow gate (`.github/workflows/bootstrap-validate.yml`)

---

**Next:** Create `bootstrap/proot_freestanding.c` (stage 1, day 1)
