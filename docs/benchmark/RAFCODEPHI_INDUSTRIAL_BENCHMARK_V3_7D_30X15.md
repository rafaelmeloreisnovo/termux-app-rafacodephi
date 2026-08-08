# RAFCODEΦ Industrial Benchmark V3 — 7D / 30×15

Status: **engineering architecture + implementation ledger**, not certification.
Scope: internal Termux RAFCODEΦ benchmark/Vectra surface only.
External Vectras application: **NOT_REQUIRED**.

## 0. Evidence boundary

The current system has already crossed the boundary from "benchmark source exists" to "a specific packaged ELF can be executed through Android's real linker and can produce a persistent receipt". That execution proof must remain logically separate from stronger claims about timer validity, repeatability, thermal stability, PMU access, energy, isolated silicon performance, and cross-device ranking.

V3 therefore splits claims into independent gates:

1. `EXECUTION_PROOF` — did the intended artifact execute and finish under the observed route?
2. `MEASUREMENT_VALIDITY` — are time units, timer semantics and workload identity valid for numerical interpretation?
3. `SERIES_VALIDITY` — are repeated samples homogeneous, intact and sufficiently numerous?
4. `ENVIRONMENT_VALIDITY` — are thermal/DVFS/memory/scheduler conditions observed and bounded?
5. `COMPARABILITY_VALIDITY` — are two results produced by the same workload contract and normalization?
6. `ENERGY_VALIDITY` — is energy measured by a valid instrument rather than inferred from metadata?
7. `PUBLICATION_VALIDITY` — can the result be reproduced and audited outside the originating run?

No gate inherits PASS from another gate.

## 1. Implemented V3 changes

### 1.1 PA protocol 00000002

Implemented in `app/src/main/cpp/freestanding/raf_pa_core.c`:

- ARM32 and AArch64 now use direct `clock_gettime(CLOCK_MONOTONIC)` syscalls.
- Elapsed values are nanoseconds on both architectures.
- The previous ARM32 `(sec << 32) | nsec` representation is removed, eliminating the second-boundary arithmetic defect.
- The payload emits a minimum observed timer-read overhead.
- The workload identity field no longer includes elapsed time; timing is excluded from the deterministic identity.
- R0…R5 remain separate heterogeneous micro-workloads.

This closes a **measurement semantics defect**, but the new protocol still requires physical execution before timing can be promoted on a device.

### 1.2 ARM32 capability namespace hotfix

Implemented in `app/src/main/cpp/lowlevel/baremetal.h`:

- ARM32 predefines AArch64-only `HWCAP_SVE` and `HWCAP2_SVE2` to zero before the existing decoder fallback can assign AArch64 numeric bit positions.
- Therefore an ARM32 `HWCAP2` bit cannot be relabeled as `CAP_SVE2` merely because it shares the same numeric bit position.

This is a fail-closed architecture namespace boundary. A positive SVE/SVE2 claim now requires the AArch64 namespace.

### 1.3 Receipt v3

Implemented in `PaBenchmarkReceipt`:

- schema `rafcodephi.pa-elf-runtime-receipt/v3`;
- legacy v2 receipt fallback retained;
- PA protocol 1 remains valid as **execution-only evidence**;
- PA protocol 2 can separately enable `claim_allowed_timing_measurement` when timer/workload predicates are complete;
- parsed R0…R5 workload rows are persisted structurally;
- explicit `series_id`, `series_index`, `series_target_n` and `series_governed` fields prevent ad-hoc runs from silently becoming a series;
- pre/post environment snapshots are bound into each new governed receipt;
- isolated-silicon, environment-stability, reproducibility and cross-device claims remain false.

The old physical PA run is therefore not discarded. Its execution proof survives, while its old timing representation is prevented from silently becoming calibrated V3 evidence.

### 1.4 Governed homogeneous series analyzer

Implemented in `PaBenchmarkSeriesAnalyzer`:

- minimum distribution target `n >= 30`;
- aggregation is constrained to the **same explicit series id**, ELF hash, ABI set, linker, protocol, workload id, operation count and flags;
- ad-hoc timing receipts are counted but cannot be promoted into a governed series;
- different R workloads and different series are never pooled;
- deterministic score/checksum drift invalidates a series instead of splitting the drift into convenient subgroups;
- exceeding the declared target count invalidates the series contract;
- statistics include `n`, min, max, mean, median, sample SD, CV, MAD, Q1, Q3, IQR and a declared approximate two-sided 95% mean interval;
- environment coverage and severe thermal-interference count are retained in the series report;
- reaching n=30 allows only a distribution summary; it does **not** automatically allow environmental-stability, reproducibility or cross-device claims.

### 1.5 Environment evidence bound to executions

Implemented by `BenchmarkEnvironmentSnapshot`, `PaBenchmarkRunner` and `PaBenchmarkReceipt`:

- Android thermal status when API/runtime exposes it;
- battery level, voltage and battery temperature with explicit `BATTERY_NOT_CPU_SOC` scope;
- memory availability/pressure;
- best-effort per-CPU cpufreq/sysfs observations and governors;
- unavailable fields remain `UNAVAILABLE`/`PARTIAL`;
- no energy or CPU-temperature claim is generated from metadata;
- every new runner execution captures a snapshot immediately before and after the ELF process and stores both in the receipt;
- severe thermal state is recorded as interference evidence and does not silently delete the sample.

This closes the **snapshot-to-execution binding** gap. It does not yet establish an environment-stability model: governor visibility, cpufreq coverage and thermal state are observations, not proof that frequency was locked or scheduler interference was absent.

### 1.6 Governed n=30 runner

Implemented in `BenchmarkMenuActivity` + `PaBenchmarkRunner`:

- single ad-hoc PA observation remains available;
- a separate `Run Governed 30-Trial Series` path creates an explicit unique series id;
- every trial receives an index and declared target;
- no hidden warm-up samples are thrown away;
- no arbitrary outliers are deleted;
- cancellation takes effect after the current trial so the receipt already being produced is preserved;
- execution/timing failure stops the governed series fail-closed;
- the series analyzer is run after completion/interruption.

### 1.7 Seven-gate machine-readable claim matrix

Implemented in `BenchmarkClaimMatrix`:

- materializes `EXECUTION_PROOF`, `MEASUREMENT_VALIDITY`, `SERIES_VALIDITY`, `ENVIRONMENT_VALIDITY`, `COMPARABILITY_VALIDITY`, `ENERGY_VALIDITY`, `PUBLICATION_VALIDITY` independently;
- `gate_inheritance=false` prevents a PA execution PASS from cascading into broader claims;
- `composite_score_allowed=false` is explicit;
- comparability, energy and public ranking remain blocked;
- environment can be observed while its stability claim remains false;
- the overall state remains narrower than the strongest individual gate.

### 1.8 CI contract

The Vectra-grade workflow now installs the focused test dependency and runs `tests/test_industrial_benchmark_v3_contract.py` before the Android build. The contract locks:

- monotonic nanosecond timer semantics;
- removal of the ARM32 packed-seconds timer representation;
- deterministic workload identity independent from timing;
- ARM32 SVE/SVE2 namespace gating;
- receipt v3 claim separation;
- explicit series-id n>=30 aggregation rules;
- pre/post environment binding;
- the seven independent claim gates and closed broad claims.

## 2. Seven antiderivative directions

"Antiderivative" here is used operationally: instead of optimizing one local metric, each direction reconstructs the wider condition that must exist for the local observation to be meaningful.

### D1 — Metrology / clock reconstruction

Local observation: elapsed number.
Required reconstruction: clock source → unit → overhead → monotonicity → wrap behavior → serialization → syscall/counter route → uncertainty.

Controls:

1. common nanosecond unit across ARM32/AArch64;
2. timer-read overhead measured separately;
3. no elapsed-time term inside deterministic workload identity;
4. explicit zero/failed timer state;
5. duration/operation ratio reported per workload only;
6. calibration metadata retained with the run;
7. cross-clock comparisons blocked unless conversion is explicit.

### D2 — Statistical reconstruction

Local observation: one R0…R5 sample.
Required reconstruction: repeated homogeneous series → raw sample retention → distribution → uncertainty → drift.

Controls:

1. n=1 is `OBSERVED_LIMITED`;
2. n>=30 enables a distribution summary, not a global benchmark claim;
3. mean + median are both retained;
4. SD/CV capture classical dispersion;
5. MAD/IQR capture robust dispersion;
6. deterministic checksum drift invalidates the series;
7. no arbitrary outlier deletion without a versioned policy.

### D3 — Environmental reconstruction

Local observation: workload duration.
Required reconstruction: thermal state + DVFS + battery/power mode + memory pressure + scheduler/interference.

Controls:

1. pre/post thermal status;
2. pre/post cpufreq availability and values where readable;
3. governor/policy metadata where readable;
4. battery state separated from CPU temperature;
5. memory pressure captured;
6. severe thermal state is an interference signal, not a hidden correction factor;
7. unavailable kernel fields remain unavailable.

### D4 — Provenance / falsifiability reconstruction

Local observation: stdout line.
Required reconstruction: source → build → ELF hash → linker → process → stdout hash → receipt history → claim state.

Controls:

1. artifact SHA-256;
2. stdout SHA-256;
3. exact linker path;
4. ABI/process bitness;
5. timeout/truncation markers;
6. append history plus atomic latest pointer;
7. negative evidence is retained and never overwritten by a later PASS.

### D5 — Workload-realism reconstruction

Local observation: synthetic kernel.
Required reconstruction: microkernel → subsystem stress → representative application behavior.

Controls:

1. preserve microbenchmarks for diagnosis;
2. add larger memory footprints and stride sweeps;
3. add compression/hash/parse/linear-algebra workloads;
4. add app-level Macrobenchmark scenarios separately;
5. add GPU/Vulkan only under a separate metric family;
6. add on-device AI only with accuracy/quality gates;
7. never mix application-realism scores with microkernel rates without a versioned normalization contract.

### D6 — Comparability reconstruction

Local observation: "device A is faster".
Required reconstruction: same workload version + same input + same units + same software route + same environmental disclosure + same aggregation.

Controls:

1. workload definition hash;
2. input/fixture hash;
3. protocol version;
4. compiler/build identity;
5. device/environment metadata;
6. reference baseline version;
7. uncertainty propagated into any normalized comparison.

### D7 — Operational/publication reconstruction

Local observation: local result.
Required reconstruction: repeatable harness → machine-readable result → CI contract → device receipt → review → publication boundary.

Controls:

1. schema-versioned JSON receipts;
2. deterministic file naming and atomic writes;
3. CI source-contract tests;
4. physical-device validation separated from CI simulation;
5. regression thresholds declared before use;
6. invalidation reasons machine-readable;
7. public claims require a release artifact and reproducible method, not screenshots alone.

## 3. Thirty reference players / ecosystems and the force to borrow

These are reference patterns, not claims of conformance or affiliation.

| # | Player / ecosystem | Practice to absorb into RAFCODEΦ |
|---|---|---|
| 1 | SPEC CPU | explicit run/reporting rules, repeatability, conditions of observation |
| 2 | SPECpower | synchronize performance with power/temperature instrumentation |
| 3 | MLCommons / MLPerf Mobile | fixed quality targets, reference implementations, submission rules |
| 4 | Google Android Microbenchmark | repeated focused in-process measurements |
| 5 | Google Android Macrobenchmark | app-level scenarios plus traces |
| 6 | Perfetto | system-wide trace correlation and machine-queryable trace analysis |
| 7 | Android Simpleperf | cycles/instructions/cache sampling where kernel policy permits |
| 8 | Google Benchmark | warm-up, repetitions, mean/median/SD/CV and context metadata |
| 9 | EEMBC CoreMark | compact self-verifying processor workload |
| 10 | EEMBC CoreMark-PRO | wider processor + memory workload diversity |
| 11 | UL PCMark Android | whole-device everyday-task realism |
| 12 | UL 3DMark | workload-specific device comparison and stress behavior |
| 13 | Geekbench | cross-platform workload versioning and realistic data sets |
| 14 | PassMark PerformanceTest | subsystem suites and baseline databases |
| 15 | Phoronix Test Suite | extensible test profiles, automation and result federation |
| 16 | OpenBenchmarking.org | machine-readable test/suite definitions and public result corpus |
| 17 | GFXBench | graphics API/game-like workload separation |
| 18 | Basemark | cross-platform system/graphics benchmarking patterns |
| 19 | AIDA64 | detailed hardware inventory + sensor/diagnostic context |
| 20 | Arm | architecture-defined feature namespaces and PMU semantics |
| 21 | Linux perf | PMU/perf-event model and counter provenance |
| 22 | Khronos | API conformance mindset for Vulkan/OpenGL workload boundaries |
| 23 | Qualcomm | heterogeneous CPU/GPU/NPU mobile execution reality |
| 24 | MediaTek | heterogeneous mobile SoC scheduling/DVFS reality |
| 25 | Samsung Semiconductor | big.LITTLE/mobile thermal and memory subsystem reality |
| 26 | Apple Instruments | timeline-oriented profiling and signposted measurement regions |
| 27 | Intel VTune | microarchitecture event correlation and hotspot decomposition |
| 28 | AMD uProf | CPU/event/power profiling separation |
| 29 | NVIDIA Nsight | GPU/system trace and kernel-level decomposition |
| 30 | NIST / ISO / IEEE measurement-quality practices | calibration, uncertainty, verification, audit and claim discipline |

## 4. Fifteen complex benchmark/profiling systems — gap/edge matrix

| System | Strongest force | Edge RAFCODEΦ must cover | V3 response |
|---|---|---|---|
| SPEC CPU | reproducible CPU workload rules | controlled run conditions | explicit claim gates + governed homogeneous series |
| SPECpower | power/performance synchronization | no calibrated power meter in APK | `ENERGY_VALIDITY` remains blocked |
| MLPerf Mobile | performance + quality | workload result without correctness is unsafe | future quality predicate per AI workload |
| Android Microbenchmark | focused repeated loops | app runtime/JIT effects | keep PA ELF route separate from Java route |
| Android Macrobenchmark | end-to-end app UX | microkernel does not predict UX | future app-level family, never pooled with PA |
| Perfetto | system causal trace | local duration lacks scheduler context | future trace/receipt correlation |
| Simpleperf | hardware/software event profiling | PMU can be permission-restricted | capability probe → UNAVAILABLE, never zero |
| Google Benchmark | repetitions/statistics | n=1 noise and warm-up policy | V3 explicit n>=30 series; warm-up remains explicit future policy, not silently discarded |
| CoreMark | compact self-verification | single score can hide subsystem behavior | keep per-workload results visible |
| CoreMark-PRO | diverse processor/memory workloads | current R0…R5 data sets are tiny | larger realistic workload pack planned |
| Geekbench | cross-platform baselines | normalization can obscure method changes | version baseline and workload hashes |
| PCMark Android | everyday task realism | synthetic PA cannot represent whole device UX | separate real-app suite |
| 3DMark/GFXBench | GPU/API stress | CPU benchmark cannot infer GPU | separate Vulkan/GPU evidence family |
| PassMark | broad subsystem baseline | composite score weighting can be arbitrary | composite remains BLOCKED_BY_DESIGN |
| Phoronix/OpenBenchmarking | automation + corpus | local-only result has weak external reproducibility | export schema + fleet/replay layer planned |

## 5. Innovation methods and implementation status

### M1 — Twin receipt

Target: independently hash-bound execution and environment objects. V3 currently binds both pre/post environment objects inside the immutable execution receipt. The next refinement is to split environment into a second hash-addressed artifact and reference its digest from the execution receipt.

### M2 — Measurement lineage DAG

Target representation:

`source commit → build configuration → ELF hash → device install → execution receipt → series → statistical report → claim matrix → comparison report`.

The execution receipt, series report and seven-gate claim matrix now exist. A dedicated hash-addressed DAG artifact is still open. Missing edges remain `TOKEN_VAZIO`, not narrative assumptions.

### M3 — Counterfactual pair runs

For every optimized path, run a paired control when feasible:

- scalar vs NEON;
- warm vs first-touch;
- sequential vs randomized access;
- single-thread vs multi-thread;
- Java-mediated vs direct ELF route.

Pairs must share input hashes and environmental windows. The reported object is the paired delta plus uncertainty, not merely two unrelated scores.

### M4 — Change-point detector

For n>=30 time-series samples, add a non-promotional drift detector that flags thermal/DVFS regime changes. It must not silently discard either side of the change point. A flagged series remains auditable and is labeled `OBSERVED_NONSTATIONARY` until a policy is defined.

### M5 — Metamorphic workload verification

Beyond a single checksum, define transformations whose expected relation is known. Example: doubling a fixed input should change operation count predictably while preserving algorithmic output constraints. This tests the harness itself and helps detect dead-code elimination or accidental workload mutation.

### M6 — Cross-device normalization with uncertainty

Future normalized index:

`index = reference_median / DUT_median` for latency metrics or `DUT_rate / reference_rate` for throughput metrics.

Requirements:

- same workload/version/input;
- baseline receipt published;
- confidence interval or bootstrap interval propagated;
- no aggregation across dimensions without an explicit weighted model.

### M7 — Evidence debt budget

Treat unresolved evidence as a quantitative release object:

- P0: timer/protocol/artifact identity;
- P1: n>=30 governed homogeneous series + thermal/DVFS context;
- P2: PMU/trace/energy/fleet comparability;
- P3: composite/public benchmark publication.

A release may contain `TOKEN_VAZIO`, but every token must have owner, closure predicate and claim it blocks.

## 6. Current maturity after V3 code changes

Engineering estimate, not measured market certification:

| Layer | Estimated maturity | Why |
|---|---:|---|
| Physical execution evidence | 90% | direct ELF route + receipt/history exists; new protocol still needs device run |
| Timer/measurement semantics | 75% structural / NOT_MEASURED physical | clock defect fixed, unit unified, overhead emitted; physical V2 receipt pending |
| Artifact/provenance | 88% | hashes, linker, output, atomic latest/history, explicit protocol/series identity |
| Statistical engine | 78% structural / low empirical | explicit governed n>=30 analyzer implemented; no current n30 V2 physical corpus yet |
| Environmental observation | 65% structural / NOT_MEASURED series | pre/post thermal/DVFS/battery/memory bound into runner receipts; stability model still blocked |
| Claim governance | 85% structural | seven independent machine-readable gates; broad claims stay false |
| PMU/trace correlation | 15% | capability/profiling integration remains open |
| Energy measurement | 5% | no calibrated energy instrument; metadata is not energy |
| Cross-device comparability | 20% | normalization contract defined conceptually, baseline fleet absent |
| Composite score | 0% by design | remains blocked until normalization/weights/uncertainty are versioned |

## 7. Release gates

### Gate A — protocol integrity

PASS only if PA protocol 2 is observed physically with all R0…R5 rows, monotonic-ns timer marker, deterministic-score marker, no timeout/truncation and exit 0.

### Gate B — series integrity

PASS for distribution analysis only if one explicit governed series reaches its declared n>=30 target and deterministic score/checksum remain invariant.

### Gate C — environment integrity

Pre/post snapshots are now bound to every runner sample. Promotion still requires a versioned stability/interference policy; severe thermal events and incomplete cpufreq visibility remain visible instead of being filtered away.

### Gate D — PMU/trace integrity

PMU/Perfetto/Simpleperf data are optional capabilities. If inaccessible, state is `UNAVAILABLE`; the benchmark must still operate without fabricated counters.

### Gate E — cross-device comparison

Requires matching workload/version/input/build-route contracts plus a versioned baseline and uncertainty model.

### Gate F — energy

Blocked until a measurement source with declared calibration/accuracy exists.

### Gate G — publication

Requires release artifact hash, complete method document, raw receipts, series report, claim matrix, environmental disclosure and review of all blocked claims.

## 8. Immediate execution sequence

1. Compile CI for ARM32 and AArch64.
2. Run PA protocol 2 on the physical ARMv7 device.
3. Confirm ARM32 capability output no longer contains SVE/SVE2 promotion.
4. Run the explicit governed 30-trial series.
5. Inspect deterministic identity drift, distribution statistics, environment coverage and severe thermal flags.
6. Materialize and review the seven-gate claim matrix.
7. Add trace/PMU capability probes and a dedicated lineage DAG artifact.
8. Create a versioned reference baseline before any cross-device index.

## 9. Invariant

> A benchmark number is not the product. The product is the reproducible chain from workload definition to observation, uncertainty, environment and claim boundary.
