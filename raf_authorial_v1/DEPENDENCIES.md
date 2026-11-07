# RAFCODEPHI / Termux: scoped dependency and rights manifest v1

This manifest is about the **current repository boundary**, not a claim of full dependency eradication or Android conformance. Sources are producer-owned; separate build-time and runtime requirements.

| Scope/owner | Observed source | Rights boundary | Dependency class | Current decision |
| --- | --- | --- | --- | --- |
| Termux Android app | termux-app-rafacodephi master@f6f6adb4; COPYNG-equivalent root file COPYING (blob f288702d2fa16d3cdf0035b15a9fcbc552cd88e7) | Inherited GNU GPL-3.0-only | Android runtime | PRESERVE_UNMODIFIED |
| termux-shared | same repository termux-shared/LICENSE.md (blob fb22a386c707baacbdd8669f1966623d2a487bb9) | MIT by default, GPLv3 and AOSP GPLv2+Classpath/Apache exceptions per file | Android shared component | PRESERVE |
| terminal-emulator / terminal-view | same repository LICENSE.md, Jack Palevich lineage | Apache-2.0 components | Android app components | PRESERVE / review file ancestry |
| Termux packages producer | rafaelmeloreisnovo/termux-packages main, LICENSE.md (blob a901a9b5b014d9e50316d300066ed0069f1f4bd8) | Build infrastructure Apache-2.0 outside package dirs; scripts/patches inside package dirs inherit **each package's own license** | Bootstrap/build/package runtime | NEVER relabel uniformly |
| Bootstrap default | termux-app-rafacodephi app/build.gradle | Default URL resolves upstream termux/termux-packages releases; optional external bootstraps are URL+SHA256 paired; package variant must match | Runtime payload | Not freestanding, use pinned artifact + actual installed proof |
| Native C11 standalone candidate | termux-app-rafcodephi: raf_authorial_v1/src | MIT at raf_authorial_v1/LICENSE; attribution/provenance review still required | Pure runtime leaf | 0 third-party runtime imports in host object gate |
| Core build selector | raf_authorial_v1/tools/raf_gate.py | MIT, Python3 standard-library only | BUILD_TIME | Do not bundle into pure runtime |
| Android build/Gradle/JVM/SDK | termux-app-rafcodephi settings.gradle, app/build.gradle, gradle/ | Respect tool/provider terms; individual versions not exhausted by this targeted read | BUILD_TIME + Android hosted | Cannot remove from Android APK by relabeling |
| Vectras integration | rafaelmeloreisnovo/Vectras-VM-Android | Root GPL terms and separate MIT-scoped authorial modules require path-level checks | External application/consumer | No proof of APK integration from this PR |
| PRoot / QEMU / BusyBox / lib dependencies | upstream or optional package payloads (distinct provider) | LICENSE_AND_EXACT_ARTIFACT=TOKEN_VAZIO_PROVENANCE_UNBOUND pending resolved bill of materials | Optional or runtime; exact BOM unknown | DO NOT equate replacement stub to compatible runtime |

## Reconciliation invariants

- SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM. No Android hardware claim from the new C11 host vectors.
- Neither GNU GPLv3 nor individual Apache/MIT/copyleft notices are replaced by the new MIT license file. The combined executable distribution remains constrained by the relevant inherited licenses.
- Whether every individual file in a large repository is proprietary/authorial/inherited is not inferable from a repo title, rename or build result.
- Freestanding C objects require a compiler/linker for production; host-side Python/profile tool does not enter their runtime dependency graph.
- Raw syscalls, inline assembly, ELF entry points and Android/JNI must reside in narrowly scoped PLATFORM_GATE/HOSTED_BOUNDARY modules, not pure core.
- The word void is a C type, not an execution environment or a way to remove OS/ABI responsibilities.
- Profile hints must pass explicit validation; formatting alone is not a machine instruction. Defer ARM asm to ISA-safe modules with instruction-level falsifiers and measured benefit.

## Next / rollback

Next: exact-run scoped CI; inspect real per-module object relocation/undefined symbols and benchmarks; build signed package-variant-pinned ARMv7 APK only in a distinct integration PR with E2E runtime receipts. Preserve pre-existing GPL and package-specific notices.
Rollback: leave PR#523 in DRAFT/HOLD, do not merge or delete sources, supersede receipts on correction.
