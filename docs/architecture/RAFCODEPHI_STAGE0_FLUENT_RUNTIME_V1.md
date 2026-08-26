# RAFCODEPHI Stage0 ARM32 + Fluent Runtime Boundary V1

Status: `MATERIALIZED_FOR_REVIEW / claim_allowed=false / release_allowed=false`

## Role

RAFCODEPHI is the Android/Termux packaging and runtime environment for RAFAELIA compiler artifacts. It is not the compiler source authority.

Compiler authority: `rafaelmeloreisnovo/RafPolimata`.

Governance/routing authority: `rafaelmeloreisnovo/Mapa`.

## Target packaging model

When a validated Stage0 compiler binary exists, package it by ABI under a stable internal path such as:

`libexec/rafaelia/rafcc-stage0-arm32`

and later:

- `libexec/rafaelia/rafcc-arm32`
- `libexec/rafaelia/rafcc-arm64`

No binary is authorized for release from this document alone. Binary/source/toolchain hashes remain `TOKEN_VAZIO` until a producer receipt is bound.

## Compile path

For a Stage0-supported source/profile, the desired on-device route is:

`source -> rafcc-stage0-arm32 -> RAFIR/A32 -> ELF32`

The normal on-device Stage0 path must not require:

- Gradle;
- Android NDK;
- clang/gcc;
- external assembler;
- external linker;
- Python lowering;
- libc/heap in the Stage0 compiler core.

This does not claim that the Android app itself is freestanding. Android/Java/JNI/Gradle remain integration surfaces and are governed separately.

Invariant:

`APP_INTEGRATION != STAGE0_COMPILER_CORE`

## Event/receipt path

Stage0 compile/runtime transitions use `RAFAELIA_FLUENT_EVENT/v1` produced by the RafPolimata codec authority.

The event plane is independent of artifact format:

`ELF32 | ELF64 | DEX | APK/ZIP -> same event envelope`

RAFCODEPHI may persist canonical event bytes locally and optionally forward them to a Fluent-compatible endpoint. SQL/Room is not required for compiler evidence.

## APK inclusion gate

Before any Stage0 compiler binary is embedded in a distributable APK, require:

1. producer repository/ref/path;
2. source SHA-256;
3. compiler/bootstrap identity;
4. build flags/profile;
5. binary SHA-256;
6. ELF machine/ABI audit;
7. no unexpected `DT_NEEDED`/PT_INTERP for the Stage0 core;
8. deterministic smoke fixture;
9. physical ARM32 execution receipt;
10. license/authorship classification;
11. APK packaged-path/hash proof.

Any missing field stays `TOKEN_VAZIO` and blocks a release claim.

## Runtime gates

### R1 — invocation

`rafcc-stage0-arm32 fixture.raf -o fixture.so`

### R2 — artifact

Verify ARM32 ELF identity, expected exports and artifact SHA-256.

### R3 — execution

Run a minimal produced artifact on a compatible ARM32 Android/Termux environment and record stdout/stderr/exit evidence.

### R4 — event continuity

Verify compile accepted -> codegen -> artifact sealed -> gate result events share sequence/component/source/artifact identity without SQL mediation.

## Migration policy

Existing Gradle/NDK build routes remain available for Android application assembly and unsupported compiler profiles.

Stage0 is introduced as a narrower independent route first. It must not silently replace a working Android build path until its own gates close.

## R3 summary

- `F_ok`: packaging/runtime contract defined without conflating app dependencies with Stage0 dependencies.
- `F_gap`: validated Stage0 ARM32 binary and physical execution receipt do not yet exist.
- `F_next`: consume the first producer-signed/hash-bound Stage0 artifact from RafPolimata, execute it on ARM32, then add it to APK packaging only after the inclusion gate passes.
