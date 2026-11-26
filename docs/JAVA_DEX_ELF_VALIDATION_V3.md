# Java / DEX / ELF — validation V3

Parent: PR #545, `df14657125a77e24073575a3ac9953913a195da3`.
Authority: this repository owns Android artifacts. `termux-packages` owns
native package objects. This change is review-only; no merge/release/promotion.

## Findings and changes

| Boundary | Observation | Correction / evidence |
| --- | --- | --- |
| Prior contract | Required the comment `without library calls`, removed in phase 3; reproduced exit 1 | Check actual forbidden dependency references; preserve v1 manifest |
| Prior CI | 18 success + 9 skipped on the PR head lookup; Java and ARM32 jobs checked out merge-ref `108ef922e136cdbc7daeaf56e68d42024bb76b7f` | Relevant workflows now explicitly check out the PR head; receipt records checked-out commit, event SHA and outcomes |
| DEX inclusion | Canonical run 44071230540 artifact 14322757146 contains ARM32, ARM64, universal debug APKs | Direct class_defs/class_data inspection found the class and four public static int APIs with DEX code in all three |
| Release shrinker | Unreferenced public API could disappear | Explicit keep rule for this one class; assembleDebug/assembleRelease inspect every generated APK |
| DEX gate | String search is insufficient | Validate header/checksums, bounded tables, class definition, method owner/signature/access/code presence and multidex uniqueness |
| Device execution | No device receipt | TOKEN_VAZIO_PHYSICAL_EVIDENCE_MISSING; DEX inspection is not bytecode execution or full ART verification |

The DEX inspector accepts legacy 035/037/038/039/040 and rejects unsupported
containers, including 041. It is a scoped presence/structure gate, not a complete
DEX verifier, instruction purity analyzer, APK signature verifier or malware scan.
The three downloaded APKs belong to the predecessor merge-ref, not the new source.

## Repeat

```sh
bash scripts/verify_lowlevel_contract.sh
bash scripts/verify_pure_java_bit_core.sh
bash scripts/verify_java_bit_android.sh . /tmp/raf-bit-new-output
python3 scripts/verify_java_bit_dex.py app/build/outputs/apk/debug
```

Requires hosted JDK, Python 3, Bash and Android build-tools D8 (CI pins
build-tools 35.0.0). Local D8 used 8.6.27, JDK 21.0.13. Java compilation targets
release 8 for the Android API-21 floor. JDK 21 marks the Java-8 *option* obsolete:
only options-category lint is excluded; code warnings remain `-Xlint:all` and
`-Werror`. This is a deliberate compatibility boundary, not a code-warning waiver.
These tools are build dependencies, not new runtime dependencies of the bit core.

`verify_java_bit_android.sh` refuses an existing output directory. It produces
`bit-core-device.jar`; it includes test/measurement classes and is not a replacement
APK or app bootstrap. In an authorized Termux session:

```sh
dalvikvm -cp bit-core-device.jar com.termux.rafacodephi.lowlevel.AuthorialBitOpsAdversarial
dalvikvm -cp bit-core-device.jar com.termux.rafacodephi.lowlevel.BitOpsBenchmark
```

The benchmark compares the existing SWAR count with `Integer.bitCount` over the
same seeded 16-KiB input: five warmup pairs, nine alternating-order rounds, raw
nanoseconds, word count and checked checksums. JVM and ART results remain separate.
It does not select a backend, prove a speedup or compare Java with native RBIT
(which is a different operation). New native integration needs the same operation,
equivalent inputs and device receipts before choosing a faster path.

## Architecture decision

| Candidate | Current decision |
| --- | --- |
| Java primitive core → DEX → ART | Preserve portable backend; inspect emitted bytes |
| C/ASM → ELF per ABI | Separate producer gate in termux-packages; not injected into Java |
| Syscall / JNI / process I/O | PLATFORM_GATE / HOSTED_BOUNDARY only; pure arithmetic has no OS work to perform |
| NEON/GPU | Pending detected hardware, workload size and measured crossover; no automatic activation |
| Cache/allocator/lock-free/LZ4/network | NOT_NEEDED for this stateless scalar module; avoid unmeasured runtime complexity |
| Toroidal/RLL models | NOT_NEEDED for executable-format and bit-operation validation |

Routing applies the requested origin, topology, graph, BITRAF, Vectra, compiler,
profiling and evidence checks to the actual producer chain. It neither changes
skills nor creates an autonomous task. No third-party algorithm was imported;
existing project/upstream licenses remain authoritative.

Rollback: revert this scoped successor on the review branch; keep predecessors.
F_ok: JVM correctness/mutation, emitted DEX presence and predecessor APK evidence.
F_gap: new full APK build, ART/device, native consumer receipt, measured hardware
choice and independent review. F_next: exact-head CI then the same-device benchmark.
`claim_allowed=false`.

## Successor S2 — CI observations

The first new CI passed the real D8 build, nine DEX tests and Java vectors, but
also showed `rg: command not found`: the old shell `if rg ...` treated unavailable
scanner as a clean result. S2 replaces that call with a Python check executed as
a prerequisite; three forbidden-dependency fixtures must exit 21.

Separate debt is preserved: provider verifier has the same 25 failures before
and after this delta; the September service receipt rejects even baseline hashes;
bootstrap-install checks a missing prose token in baseline and current; two legacy
rewrite jobs call a Gradle task absent from the unchanged baseline. None of these
states is erased, skipped deliberately or relabeled PASS by this change. Repairing
provider IPC or reconstructing bootstrap tasks requires their own bounded source
and regression gates. Full new builds and physical Android remain separate.
