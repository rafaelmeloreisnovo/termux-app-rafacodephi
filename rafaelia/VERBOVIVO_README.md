# Verbovivo — Pure Freestanding Hyperdimensional Computing Graph

## Overview

Verbovivo is a pure graph-based hyperdimensional computing system for T^7 toroid lattice traversal, convergence computation, and attractor recall. Entirely **freestanding**: no libc, no malloc, no stdio—only binary logic and ARM64 syscalls.

## Architecture

### Core Components

1. **verbovivo_graph.h** — Type definitions and API
   - `HyperVector`: 1024-bit hypervectors (128 × 64-bit lanes)
   - `GraphNode`: computation nodes with binary state
   - `GraphEdge`: conditional transitions (XOR, AND, OR, NOT, NOOP)
   - `T7ToroidGraph`: 42-attractor lattice with up to 256 nodes

2. **verbovivo_graph.c** — Graph computation engine
   - `vv_graph_init()`, `vv_graph_add_node()`, `vv_graph_add_edge()`
   - `vv_graph_step()`: single-edge traversal with logic application
   - `vv_graph_converge()`: iterate until fixed point or timeout
   - `vv_graph_compute_phi()`: entropy H_norm, coherence C_norm, φ = (1-H)·C in Q16
   - `vv_graph_recall()`: find nearest attractor via Hamming distance

3. **t7_toroid_builder.c** — T^7 lattice construction
   - `vv_build_t7_toroid()`: Create 42 attractors with 7-bit binary seeding
   - Attractor connectivity via Hamming neighbors (single-bit flips)
   - Intermediate node generation via XOR interpolation
   - `vv_verify_t7_coherence()`: Validate attractor consistency
   - `vv_serialize_t7()`: Binary export for persistent storage

4. **verbovivo_bootstrap_gate.c** — Bootstrap integration
   - `BootstrapReceipt`: Receipt structure with φ_fst, attractor, exit code
   - `ConvergenceReceipt`: Extended receipt with H, C, φ values
   - `vv_execute_convergence()`: Run graph convergence, fill receipt
   - `vv_validate_convergence_receipt()`: Fail-closed validation
   - `vv_bootstrap_convergence_gate()`: Main integration point with cross-validation
   - `write_syscall()`: ARM64 SVC #0 for direct logging

## Building

### Compile Library

```bash
make -f Makefile.verbovivo verbovivo-lib
# Output: build/libverbovivo_graph.a
```

### Verify Freestanding Compliance

```bash
make -f Makefile.verbovivo verbovivo-syntax-check
# Rejects: #include <libc>, malloc, calloc, printf, strlen
```

### Build & Link Test (ARM64 target, native host)

```bash
# Requires ARM64 cross-compilation toolchain
clang -target aarch64-linux-gnu -ffreestanding -nostdlib -nostdinc \
  -O2 -Wall -march=armv8-a+simd \
  tests/native/test_verbovivo_convergence.c \
  build/libverbovivo_graph.a -o tests/native/test_verbovivo_convergence
```

## Design Principles

### Pure Binary Logic

- **No ASCII encoding**: All data is binary logic states (bitwise operations)
- **No legacy support**: No string.h, no character manipulation
- **Direct syscalls**: ARM64 SVC #0 for write, read, fork operations

### Fail-Closed Validation

- φ must satisfy 0 ≤ φ ≤ 0x10000 (Q16 bounds)
- H_norm, C_norm must be Q16 bounded
- φ = (1-H)·C must hold exactly
- Attractor consistency: if status=attractor, then attractor_id ∈ [0, 41]
- Invalid receipt → immediate -1 return, never silent pass

### Graph Structure

- **Attractors**: 42 fixed points mapped to 7-bit binary coordinates
- **Intermediate nodes**: Bridge nodes via XOR interpolation (up to 214 total)
- **Edges**: Up to 8 per node; conditional logic (XOR, AND, OR, NOT, NOOP)
- **Convergence**: Walk until at attractor or φ stabilizes for 3+ iterations

### Fixed-Point Math

- **Q16 format**: All φ values × 0x10000 (65536)
- **Range**: [0, 0x10000] represents [0, 1]
- **Computation**: `φ = ((0x10000 - H) × C) >> 16`
- **Example**: H=0x8000 (50%), C=0x8000 (50%) → φ=0x4000 (25%)

## Integration with Bootstrap

The `vv_bootstrap_convergence_gate()` function integrates Verbovivo into the RAFAELIA bootstrap pipeline:

```c
BootstrapReceipt bootstrap_receipt = { ... };
T7ToroidGraph graph;
ConvergenceReceipt conv_receipt = { 0 };

int result = vv_bootstrap_convergence_gate(&graph, &bootstrap_receipt, &conv_receipt);

if (result == 0) {
    // ✓ Convergence succeeded and φ validated
    // ✓ Bootstrap φ ≈ convergence φ (within ±5% tolerance)
} else {
    // ✗ Validation failed; reject bootstrap
}
```

## Invariants

```
gcd(Δr, 42) = 1           (coprimality for 42-state cycle)
|attractors| = 42         (fixed)
φ_fst ∈ [0, 0x10000]      (Q16 bounds)
H_norm, C_norm ∈ [0, 0x10000]
convergence_iterations ∈ [1, 10000]
```

## Performance

- **Graph construction**: O(42) attractors + O(42×7) edges = O(294) operations
- **Convergence walk**: Up to 5000 iterations (tunable); typical: 10-100
- **φ computation**: O(HV_LANES) = O(128) lane-wise operations
- **Memory**: ~90KB static for graph + 2KB receipts

## Files

| File | Purpose |
|------|---------|
| `verbovivo_graph.h` | Public API & type definitions |
| `verbovivo_graph.c` | Core graph computation (~320 LOC) |
| `t7_toroid_builder.c` | T^7 lattice construction (~217 LOC) |
| `verbovivo_bootstrap_gate.c` | Bootstrap integration (~210 LOC) |
| `Makefile.verbovivo` | Build system & compliance checks |
| `VERBOVIVO_README.md` | This file |

## Testing

Minimal freestanding test located at:
`tests/native/test_verbovivo_convergence.c`

**Test sequence:**
1. Build T^7 toroid (42 attractors)
2. Verify graph coherence
3. Run convergence walk from attractor 0
4. Validate φ bounds [0, 0x10000]
5. Verify attractor consistency

**Building test (cross-compilation):**
```bash
clang -target aarch64-linux-gnu -ffreestanding -nostdlib -nostdinc \
  -O2 -Wall -march=armv8-a+simd \
  tests/native/test_verbovivo_convergence.c \
  build/libverbovivo_graph.a -o tests/native/test_verbovivo_convergence_arm64
```

**Running on ARM64 device (Termux/Android):**
```bash
adb push tests/native/test_verbovivo_convergence_arm64 /data/local/tmp/
adb shell /data/local/tmp/test_verbovivo_convergence_arm64
```

## Freestanding Compliance

All code passes `-ffreestanding -nostdlib -nostdinc` with zero violations:

✅ No `#include <stdio.h>`, `<stdlib.h>`, `<string.h>`, etc.  
✅ No `malloc()`, `calloc()`, `free()`  
✅ No `printf()`, `sprintf()`, `strlen()`  
✅ Inline `memcpy_fs()`, `memset_fs()` implementations  
✅ Direct ARM64 syscalls via `svc #0`  

## Related Documentation

- `AGENTS.md` — Repository federation and governance
- `CLAUDE.md` — AI agent instructions
- `docs/00_BUG_MASTER_INDEX.md` — Critical bug tracking
- `STATUS.md` — Release profile and cycle status

## References

- **Hyperdimensional Computing**: Frady, Sommer, Kanerva (IEEE)
- **T^7 Toroid Lattice**: RAFAELIA math formulas in `rmr/Rrr/RAFAELIA_MATH_FORMULAS.md`
- **Q16 Fixed-Point**: ARM ISA reference manual
- **Freestanding C**: C99/C11 standard + GCC/Clang `-ffreestanding`
