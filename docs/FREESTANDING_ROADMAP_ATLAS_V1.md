# Freestanding Roadmap Atlas V1 — Resume Without Re-reading

Machine cursor: `configs/freestanding-work-cursor.v1.json`.

## Resume invariant

```text
HEAD -> cursor -> last receipt -> active node -> gate -> successor receipt -> next node
```

A restart does not begin with a full repository crawl unless cursor provenance
fails. The cursor routes; receipts/evidence prove.

| node | state | bounded objective |
|---|---|---|
| N010 | SOURCE_OBSERVED | canonical typed TOKEN_VAZIO dictionary/schema/validator |
| N020 | CI_SCOPE_PASS | fixed-size Q16 pure leaf + host vectors + ARMv7/AArch64 assembly probe |
| N030 | ACTIVE | compose dictionary validation into existing freestanding auditor |
| N040 | QUEUED | route placeholder BLAKE3 to authoritative KAT-backed producer |
| N050 | QUEUED | canonical/alias/consumer graph for 43 duplicate excess blobs |
| N060 | QUEUED | fixed-block CRC leaf, compile-time specialized/unrolled |
| N070 | QUEUED | bounded BITRAF encode/extract leaves |
| N080 | QUEUED | exact ELF + PT_INTERP/DT_NEEDED/undef/reloc/symbol/section audit |
| N090 | QUEUED | exact-artifact physical ARM32/ARM64 receipts |

## N020 evidence

Control-plane run `36273254098`, policy job `108491098854`:

```text
pure_core_source_policy=PASS
host-native-q16-vectors compile_exit=0 run_exit=0 branches=[]
armv7a-none-eabi compile_exit=0 branches=[]
aarch64-none-elf compile_exit=0 branches=[]
RAFAELIA ZERO compatibility=PASS
source_files_total=359
source_unique_blobs=316
exact_duplicate_excess=43
```

This is source/compiler/host-vector/assembly evidence, not physical-device proof.

Provider Protection run `36273253772` failed on an independent governance
lane; it is preserved as `TOKEN_VAZIO_GOVERNANCE_BLOCKED`, not mislabeled as
a Q16 failure.

## Interruption rule

Before stopping a material sequence: append a receipt and update only cursor
head/history/active-node/open-tokens. Do not copy the corpus into the cursor.
