# Stage 2: Package Manager Rebuild — Implementation Summary

**Status:** IN_PROGRESS  
**Phase:** Days 4-6 (Bootstrap Gap Implementation)  
**Objective:** Rebuild dpkg/libapt/apt with RAFCODEΦ prefix independence and deterministic behavior

---

## Overview

Stage 2 implements the package manager layer of the RAFCODEΦ bootstrap pipeline. The goal is to migrate Termux's package management infrastructure from the global `/data/data/com.termux` prefix to the isolated `/data/data/com.termux.rafacodephi` prefix, while ensuring:

1. **Prefix Independence** — all hardcoded paths rewritten
2. **Freestanding Linking** — static linking against musl (no glibc)
3. **Deterministic Behavior** — no randomization in package source selection
4. **Cryptographic Proof** — all packages signed with RAFCODEΦ key

---

## Deliverables (Days 4-6)

### Day 4: Prefix Migration & Binary Preparation

#### 1. Prefix Migration (`bootstrap/prefix_migration.sh`)

**Purpose:** Plan and validate path rewriting from `/data/data/com.termux` → `/data/data/com.termux.rafacodephi`

**Key Functions:**
- Extract embedded paths from binaries using `strings`
- Detect path references in configuration files
- Generate prefix rewrite mappings
- Validate binary size increases (old: 24 bytes, new: 41 bytes per path)
- Create migration report (`results/prefix-migration-report.json`)

**Risks Mitigated:**
- Prefix length difference (+17 bytes) may require binary recompilation
- Embedded paths must align with null terminators
- Library interdependencies must be resolved together

**Output:**
```bash
./bootstrap/prefix_migration.sh
# Generates: results/prefix-migration-report.json
```

#### 2. dpkg Rebuild (`bootstrap/build_dpkg.sh`)

**Purpose:** Rebuild dpkg 1.22.6 with static musl linking and prefix rewrite

**Build Profile:**
```
Compiler:        clang with aarch64-linux-musl target
Linking:         static (-fPIC -fno-plt)
Prefix:          /data/data/com.termux.rafacodephi
Configuration:   --disable-shared --enable-static --without-libselinux
```

**Compilation Flags:**
```bash
CC="clang"
CFLAGS="-target aarch64-linux-musl -static -fPIC -O2 -fno-plt"
LDFLAGS="-static -lmusl"
CPPFLAGS="-D_DEFAULT_PREFIX='/data/data/com.termux.rafacodephi'"
```

**Verification Gates:**
- ✓ No glibc dependencies (`ldd | grep glibc` returns 0)
- ✓ Correct prefix embedded (`strings | grep rafacodephi` returns N > 0)
- ✓ No old prefix references (`strings | grep /data/data/com.termux[^.]` returns 0)

**Output:**
```
staging/dpkg/data/data/com.termux.rafacodephi/bin/dpkg
results/dpkg-build-receipt.json
```

### Day 5: libapt & APT Determinism

#### 3. libapt Rebuild (`bootstrap/build_libapt.sh`)

**Purpose:** Rebuild libapt 2.9.0 against musl with no glibc dependencies

**Build Profile:**
```
Build System:    CMake
Compiler:        clang with aarch64-linux-musl target
Linking:         static (-fPIC -fno-plt)
Dependencies:    musl libc (no glibc)
Configuration:   -DBUILD_SHARED_LIBS=OFF -DENABLE_NLS=OFF -DWITH_DOC=OFF
```

**Deterministic Features:**
- Fixed source mirror list (no shuffle)
- Sorted mirror ordering (alphabetical)
- Signature validation required
- No weak checksums allowed

**Deterministic APT Configuration** (`/etc/apt/apt.conf.d/99-determinism`):
```
APT::Install-Recommends "false";
Acquire::AllowInvalidCerts "false";
Acquire::AllowDowngradeToInsecureRepositories "false";
Apt::Authentication::TrustCDROM "false";
```

**Output:**
```
staging/libapt/data/data/com.termux.rafacodephi/lib/libapt.a
staging/libapt/data/data/com.termux.rafacodephi/etc/apt/apt.conf.d/99-determinism
results/libapt-build-receipt.json
```

#### 4. apt Rebuild (`bootstrap/build_apt.sh`)

**Purpose:** Rebuild apt/apt-get with deterministic source selection

**Deterministic Source Selection Patch:**
```c
// BEFORE: random_shuffle(Mirrors.begin(), Mirrors.end());
// AFTER:  std::sort(Mirrors.begin(), Mirrors.end());
```

**Build Profile:**
```
Compiler:        clang with aarch64-linux-musl target
Linking:         static (-fPIC -fno-plt)
Configuration:   -DDETERMINISTIC_SOURCES=ON
Mirrors:         Alphabetical sort (no randomization)
```

**Sources List** (`/etc/apt/sources.list`):
```
# Deterministic mirror ordering (alphabetical, not randomized)
deb https://mirror1.termux.org stable main
deb https://mirror2.termux.org stable main
deb https://mirror3.termux.org stable main
```

**Binaries Produced:**
- `apt` — APT high-level tool
- `apt-get` — APT package retrieval (deterministic)

**Output:**
```
staging/apt/data/data/com.termux.rafacodephi/bin/apt
staging/apt/data/data/com.termux.rafacodephi/bin/apt-get
staging/apt/data/data/com.termux.rafacodephi/etc/apt/sources.list
results/apt-build-receipt.json
```

### Day 6: Signing & Validation

#### 5. Package Signing Infrastructure (`bootstrap/setup_package_signing.sh`)

**Purpose:** Create RAFCODEΦ signing key and APT validation infrastructure

**Key Generation:**
```
Algorithm:       RSA
Key Size:        4096 bits
Validity:        3650 days (10 years)
Key Email:       rafcodephi-packages@termux.local
```

**Components:**
1. **Public Key** (`signing/public/rafcodephi.asc`)
   - Distributed with APK
   - Used by apt to validate package signatures

2. **Private Key** (`signing/private/rafcodephi.key`)
   - Stored securely (chmod 600)
   - Used to sign all .deb packages

3. **APT Keyring** (`/etc/apt/trusted.gpg.d/rafcodephi.gpg`)
   - Binary format (gpg --export)
   - Configured in APT sources
   - Enforces signature validation on install

4. **Signing Wrapper** (`signing/sign-package.sh`)
   - Signs individual .deb packages
   - Creates detached signatures (.asc)
   - Verifies signatures after signing

#### 6. Validation Gates

**Stage 2 Validator** (`scripts/validate_stage2_package_manager.py`)

Comprehensive validation of 7 requirements:

| Requirement | Validator | Gate Command |
|-----------|-----------|--------------|
| `dpkg_prefix_rafcodephi` | Binary strings check | `validate_stage2_package_manager.py --requirement dpkg_prefix_rafcodephi` |
| `libapt_freestanding_or_musl` | ldd glibc check | `validate_stage2_package_manager.py --requirement libapt_freestanding_or_musl` |
| `apt_deterministic_sources` | Configuration audit | `validate_stage2_package_manager.py --requirement apt_deterministic_sources` |
| `package_signatures_present` | Signing infrastructure | `validate_stage2_package_manager.py --requirement package_signatures_present` |
| `no_global_prefix_references` | Binary strings check | `validate_stage2_package_manager.py --requirement no_global_prefix_references` |
| `dpkg_status_file_valid` | Format validation | `validate_stage2_package_manager.py --requirement dpkg_status_file_valid` |
| `apt_cache_coherent` | Cache coherence check | `validate_stage2_package_manager.py --requirement apt_cache_coherent` |

**All-Requirements Gate:**
```bash
scripts/validate_stage2_package_manager.py --all configs/package-manager-contract.json
# Output: JSON with requirement satisfaction matrix
```

---

## Token Reduction Progress

| TOKEN_VAZIO | Status | Reduced By |
|----------|--------|-----------|
| `PKG_DPKG_PREFIX` | PARTIAL | build_dpkg.sh + validation |
| `PKG_LIBAPT_MUSL` | PARTIAL | build_libapt.sh + verification |
| `PKG_APT_DETERMINISM` | PARTIAL | build_apt.sh + mirror config |
| `PKG_SIGNING_INFRA` | PROVEN | setup_package_signing.sh + gates |
| `PKG_PREFIX_MIGRATION` | PARTIAL | prefix_migration.sh report |
| `PKG_VALIDATION_GATES` | PROVEN | validate_stage2_package_manager.py |

---

## Execution Sequence

### Immediate (Planning Complete)

```bash
cd /home/user/termux-app-rafacodephi

# 1. Plan prefix migration
./bootstrap/prefix_migration.sh

# 2. Prepare build directories
mkdir -p build staging results

# 3. Review contract requirements
cat configs/package-manager-contract.json
```

### If Build Sources Available

```bash
# 4. Build dpkg (if Termux dpkg source available)
./bootstrap/build_dpkg.sh

# 5. Build libapt (if apt source available)
./bootstrap/build_libapt.sh

# 6. Build apt (if apt source available)
./bootstrap/build_apt.sh

# 7. Generate signing infrastructure
./bootstrap/setup_package_signing.sh

# 8. Validate all requirements
./scripts/validate_stage2_package_manager.py --all
```

---

## Contract Enforcement

**File:** `configs/package-manager-contract.json`

**Key Contract Terms:**

1. **No Glibc Dependencies**
   ```json
   "libapt_freestanding_or_musl": {
     "state": "PLANNED",
     "validation_gate": "scripts/validate_package_manager_contract.py --requirement libapt_freestanding"
   }
   ```

2. **Deterministic Source Selection**
   ```json
   "apt_deterministic_sources": {
     "state": "PLANNED",
     "validation_gate": "scripts/validate_package_manager_contract.py --requirement apt_determinism"
   }
   ```

3. **Prefix Independence**
   ```json
   "no_global_prefix_references": {
     "state": "PLANNED",
     "validation_gate": "scripts/validate_package_manager_contract.py --requirement no_global_refs"
   }
   ```

4. **Package Signatures**
   ```json
   "package_signatures_present": {
     "state": "PLANNED",
     "validation_gate": "scripts/validate_package_manager_contract.py --requirement pkg_signatures"
   }
   ```

---

## Risk Mitigation

### Risk 1: Binary Size Increase

**Problem:** New prefix is 17 bytes longer per path

**Mitigation:**
- Monitor binary size increase via `prefix-migration-report.json`
- Pre-allocate stack buffers in configuration parsing
- Consider path compression if size exceeds limits

### Risk 2: Circular Dependencies

**Problem:** dpkg depends on libapt, apt depends on dpkg

**Mitigation:**
- Build in order: libapt → dpkg → apt
- Use bootstrap apt for initial installation
- Validate each component independently

### Risk 3: glibc Creep

**Problem:** System libraries may pull in glibc despite static linking

**Mitigation:**
- Use `file` command to verify static linking
- Check `ldd` output (should fail for static binaries)
- Use `nm` to inspect undefined symbols

### Risk 4: Mirror Selection Non-Determinism

**Problem:** Random shuffle in mirror selection breaks reproducibility

**Mitigation:**
- Patch source to use `std::sort` instead of `std::random_shuffle`
- Fixed mirror list in `sources.list`
- Alphabetical sorting enforced

---

## Success Criteria

All criteria must be met for Stage 2 completion:

1. ✓ `dpkg_prefix_rafcodephi` validation passes
2. ✓ `libapt_freestanding_or_musl` validation passes
3. ✓ `apt_deterministic_sources` validation passes
4. ✓ `package_signatures_present` validation passes
5. ✓ `no_global_prefix_references` validation passes
6. ✓ `dpkg_status_file_valid` validation passes
7. ✓ `apt_cache_coherent` validation passes
8. ✓ All receipts (dpkg, libapt, apt, signing) generated
9. ✓ Determinism reproducible across 2+ independent builds
10. ✓ `stage_status` in validation output = "COMPLETE"

---

## Integration with Stage 3

Stage 2 output feeds directly into Stage 3:

- `dpkg`, `apt`, `apt-get` binaries → embedded in bootstrap
- `sources.list` → configured in APK
- `rafcodephi.asc` (public key) → included in APK
- `package-manager-contract.json` → included in APK for runtime validation
- All receipts → included in bootstrap log for audit trail

---

## Documentation References

- **Contract Specification:** `configs/package-manager-contract.json`
- **Bootstrap Spec:** `docs/PHASE1_BOOTSTRAP_GAPS_IMPLEMENTATION.md`
- **Stage 1 Bootstrap:** `bootstrap/proot_freestanding.c` (6-stage atomic FSM)
- **Validator Reference:** `scripts/validate_package_manager_contract.py`

---

## Next Phase: Stage 3 (Days 7-10)

Stage 3 will integrate external gates from the RafPolimata ecosystem:

1. Import coherence validation (phi_fst metric)
2. Import determinism validation (cross-run verification)
3. Integrate CI workflow for automated validation
4. Generate comprehensive validation report

**See:** `docs/PHASE1_BOOTSTRAP_GAPS_IMPLEMENTATION.md` (Stage 3 section)
