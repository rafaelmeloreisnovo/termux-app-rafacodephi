# BITRAF — eight-cycle temporal release gate

## Purpose

The existing BITRAF machine closes four ordered phases before advancing logical
`time`:

```text
0 INPUT
1 PROCESS
2 OUTPUT
3 SEMANTIC
```

This module adds a second boundary without redefining that machine:

```text
4 ordered phases
→ 1 complete logical cycle
→ 8 complete logical cycles
→ 1 atomic release
→ frequency advances by 0.1 Hz
```

The first staged result cannot be published when cycle 1 begins. It becomes
observable only when the final phase of cycle 8 has been accepted with evidence.

## Canonical configuration

```text
logical frame           100,000 us = 100 ms
scheduler cadence       10 frames/s
base reference          10,000 mHz = 10.0 Hz
frequency increment        100 mHz = 0.1 Hz
window                   8 logical cycles
phases per cycle         4
observations per release 32
nominal window           800 ms
upper configured bound   999 Hz
```

Frequency is stored in millihertz and phase in unsigned Q0.32 turns. The kernel
uses no floating point, trigonometric runtime, heap allocation, sleep call or
clock syscall.

## Release law

For window index `w` and phase `p`:

```text
expected sequence =
(w0,p0) (w0,p1) (w0,p2) (w0,p3)
...
(w7,p0) (w7,p1) (w7,p2) (w7,p3)
```

The completed-cycle mask must become:

```text
0b11111111 = 0xFF
```

Only then:

```text
released_digest = staged_digest
release_epoch++
frequency_mhz += 100
```

Frequency does not change inside an incomplete window. Partial state remains
staged and non-publishable.

## Fail-closed conditions

The gate latches a fault when:

- a phase arrives out of order;
- an observation has no evidence binding;
- a completed window does not contain all eight cycle bits;
- configuration changes the four-phase/eight-cycle structural law.

A latched gate does not recover implicitly. `raf_phase_release_gate_reset_fault`
must be called explicitly. Reset discards the partial window but preserves the
last completed release epoch.

## The 10 Hz / 100 ms alias anchor

At 10 Hz, one 100 ms frame spans exactly one full reference turn:

\[
10\;\mathrm{Hz}\times0.1\;\mathrm{s}=1\;\text{turn}.
\]

Sampling only at those boundaries repeats the same phase. The module records
this as:

```text
RAF_PHASE_GATE_FLAG_ALIAS_ANCHOR
```

It is a calibration anchor, not proof that an oscillation was observed.

After the first eight-cycle release, the frequency becomes 10.1 Hz:

\[
10.1\times0.1=1.01\;\text{turns}.
\]

The residual phase is:

\[
0.01\;\text{turn}=3.6^\circ
\]

per logical frame. The Q0.32 accumulator records that residual without floating
point.

## Physical-clock boundary

The module does not sleep and does not read a clock. A caller may drive one
logical cycle every 100 ms, but source code alone does not prove that cadence on
Android.

```text
kernel sequencing implemented       FATO
host native test                    local evidence
Android NDK final-head build        TOKEN_VAZIO until observed
100 ms physical cadence             TOKEN_VAZIO until receipt
ARM32 device execution              TOKEN_VAZIO
ARM64 device execution              TOKEN_VAZIO
runtime hookup to BITRAF caller     TOKEN_VAZIO
claim_allowed                       false
release_allowed                     false
```

## Validation

The canonical native gate compiles and runs:

```text
rafaelia/src/main/cpp/raf_phase_release_gate.c
tests/native/test_raf_phase_release_gate.c
```

The test proves:

- cycles 1–7 produce no release;
- phase 3 of cycle 8 produces exactly one release;
- the frequency changes from 10.0 to 10.1 Hz after that release;
- the first 10.1 Hz frame produces the expected Q0.32 residual;
- phase disorder and missing evidence latch faults;
- reset is explicit.

The Python source contract rejects heap use, floating point, trigonometric
runtime, physical sleep, missing NDK integration and promotion of device claims.
