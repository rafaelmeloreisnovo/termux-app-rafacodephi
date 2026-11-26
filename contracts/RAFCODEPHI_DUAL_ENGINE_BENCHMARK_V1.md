# RAFCODEPHI Dual Engine — Contract V1 (DRAFT/HOLD)

## Authority and reuse
- One Android application and source repository: rafaelmeloreisnovo/termux-app-rafacodephi.
- CLI tool producer: rafaelmeloreisnovo/termux-packages. Do not copy Bash/APT/Ninja or their build recipes into app repository.
- Existing PaBenchmarkRunner.java is Java control code launching a native ELF, NOT a Java-only workload. Java-only engine = NOT_IMPLEMENTED.
- Existing app/src/main/cpp/freestanding/raf_pa_core.c provides native infrastructure, but it does not implement this common V1 workload receipt yet = NOT_IMPLEMENTED.
- No Gradle/bootstrap/NDK changes, no second app/repository, no new automatic fallback.

## Contract
- Source schema: contracts/rafcodephi-dual-engine-benchmark.v1.schema.json.
- Each observation requires engine, workload identity/version/algorithm, deterministic input seed, operations, ABI, API, artifact hash, checksum, nanosecond timing and provenance.
- Typed null means not measured. Never replace missing values with zero or PASS.
- Both engines must execute exactly the same deterministic algorithm with the same input seed and operation count before comparing results.
- For same-device comparisons, require same ABI/API, identical algorithm/version/workload/seed/operations and matching checksums. Incompatible pairs are BLOCKED.
- Runtime E2E proof, temperature equivalence, silicon isolation and performance gain remain TOKEN_VAZIO until separately measured.
- A credible speed comparison requires predeclared warmup, at least 30 valid repetitions per engine, alternated execution, median and dispersion, and thermal/environment receipts. Never pool heterogeneous runs.
- Java workload must be genuinely Java/ART; running an ELF through Java ProcessBuilder is not a Java-only benchmark.
- No automatic fallback: unavailable engine is NOT_RUN, not an implicit pass.
- claim_allowed=false in schema V1 until explicit future promotion contract and device receipts.

## Next independent increments
1. Implement deterministic Java-only workload and write real v1 receipt.
2. Implement mathematically identical C freestanding ARM32/API29 and ARM64 workload with matching vectors and receipt.
3. Add optional switch in existing benchmark UI; do not interfere with terminal, Bash, APT or Ninja.
4. CI static schema validation, JVM tests, Android ARM32/ARM64 build gates, and physical Android 10 readback for each engine.

## Rollback and evidence
- SOURCE_ONLY: contract and static tests; RUN_TIME=NOT_RUN; DEVICE=TOKEN_VAZIO; claim_allowed=false.
- Rollback: revert only files introduced in this draft PR. No merge/publication by this change.
- R3 = F_ok source contract / F_gap Java-only implementation + compatible native workload / F_next Java vectors before native parity.
