# RAFCODEPHI — independent C11 microcore v1

**State: isolated candidate, NOT wired to APK/Termux/PRoot/QEMU. No Android, cryptography or hardware performance claim.**

## License and provenance boundaries

- New standalone files under raf_authorial_v1/src, tools and tests: SPDX MIT. Full text: raf_authorial_v1/LICENSE. Copyright/clean-room assertions still require human source-and-commit provenance review.
- Original Termux application: inherited GNU GPL-3.0-only in root COPYING, unchanged. Renaming, optimizing and rebuilding inherited source does not transfer copyright.
- termux-shared, terminal-emulator/view, Android/AOSP/AndroidX and upstream dependencies: original per-file and per-component licenses remain effective.
- Distribution of a combined derivative app under GPLv3 remains subject to GPLv3 obligations; the independent MIT source retains its MIT notice. Review the exact integration boundary.
- No inherited Termux, PRoot or QEMU source was copied into this module.

## Compiler phase metadata

Each C implementation file starts with one exact approved first-line comment, e.g.

    /* RAF_OPT:PROFILE=speed;UNROLL=4 */

The C compiler ignores comments. The explicit build-time Python standard-library validator interprets this marker and selects args:

| Profile | Flags |
| --- | --- |
| stable | -O2 |
| size | -Os |
| speed | -O3 |
| UNROLL=1/4 | -DRAF_UNROLL=1/4 |

The parser rejects malformed, duplicated, unknown profiles, NUL, trigraph spellings and line splices. It never evaluates arbitrary commands. Whitespace and comments are NOT binary instructions without an explicitly specified input transform. Source layout and toolchain can affect emitted code, but not prove improvement.

## Isolation and dependencies

PURE_CORE: portable C11, caller-owned memory, no system includes, libc, heap, syscalls, JNI, Android, network, dynamic loading or package management. raf_fold32 is a **noncryptographic** FNV-1a demonstration, never for authenticating APKs, bootstrap archives, artifacts or signatures.

BUILD_TOOL: Python 3 stdlib, C compiler and nm are build-time dependencies, NOT pure runtime dependencies. Android/Linux ABI services remain prerequisites for a real device application.

HOSTED_TEST: stdio and assert are used only in test vectors, never linked into pure object files.

Focused gates:

    python3 raf_authorial_v1/tools/raf_gate.py gate gcc
    python3 raf_authorial_v1/tools/raf_gate.py gate clang
    python3 raf_authorial_v1/tools/raf_gate.py cross clang

Cross compiler generates ARMv7/AArch64 objects only; it does NOT prove execution on an Android device, linked ELF, package manager or VM.

## Promotion criteria

1. Independent source ancestry and file-level license review.
2. Exact-head GCC and Clang host vectors; adversarial parser tests.
3. ARMv7/AArch64 object symbol, ABI, assembly and size audit.
4. Separate review of actual Android/Termux adapter with bootstrap regression and signing gates.
5. Physical Moto E7 ARMv7 install and runtime evidence before claiming device support or speedup.

STATE=IMPLEMENTED_UNTESTED until terminal gates.
claim_allowed=false; APK_EXECUTION=TOKEN_VAZIO_NOT_RUN; DEPENDENCY_REPLACEMENT=TOKEN_VAZIO_NOT_IMPLEMENTED.
Rollback: keep draft unmerged; preserve commits and original upstream notice files.
