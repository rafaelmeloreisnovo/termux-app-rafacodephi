# TESTE DE MESA (Desk Checking) Methodology

## Overview

**Teste de Mesa** (Portuguese for "Desk Checking" or "Trace Table Testing") is a systematic manual verification methodology used to validate code logic by tracing through program execution step-by-step with sample inputs.

This methodology is fundamental to the RAFAELIA Framework's quality assurance approach, ensuring:
- **Deterministic behavior**: Same inputs always produce same outputs
- **Mathematical coherence**: Calculations are mathematically correct
- **Edge case handling**: Boundary conditions are properly managed
- **Numerical stability**: Floating-point operations are reliable

---

## Core Principles

### 1. ψχρΔΣΩ Cycle Integration

The Teste de Mesa methodology integrates with RAFAELIA's feedback cycle:

```
ψ (Psi) - Perception:     Define test inputs clearly
χ (Chi) - Feedback:       Check expected outcomes before execution
ρ (Rho) - Expansion:      Execute computation step-by-step
Δ (Delta) - Validation:   Verify results against expected values
Σ (Sigma) - Execution:    Synthesize and document findings
Ω (Omega) - Alignment:    Ensure ethical coherence of testing
```

### 2. Trace Table Structure

Each trace table documents:

```
| Step | Variable States | Operations | Expected Result |
|------|-----------------|------------|-----------------|
| 0    | Initial values  | -          | -               |
| 1    | After step 1    | Operation  | Result          |
| ...  | ...             | ...        | ...             |
| n    | Final values    | -          | Final answer    |
```

### 3. Φ_ethica (Ethical Filter)

```
Φ_ethica = Min(Entropia) × Max(Coerência)
```

Applied to testing:
- **Min(Entropia)**: Reduce test complexity to essential cases
- **Max(Coerência)**: Ensure tests are consistent and meaningful

---

## Test Categories

### 1. Trace Table Tests

Detailed step-by-step verification of algorithm execution.

**Example: pow(2, 5) Trace Table**

```
Input: base=2.0, exp=5

| Step | absExp | current | result | absExp & 1 | Action                    |
|------|--------|---------|--------|------------|---------------------------|
| 0    | 5      | 2.0     | 1.0    | 1          | result *= current (1*2=2) |
| 1    | 2      | 4.0     | 2.0    | 0          | skip                      |
| 2    | 1      | 16.0    | 2.0    | 1          | result *= current (2*16)  |
| 3    | 0      | 256.0   | 32.0   | -          | exit loop                 |

Expected: 32.0
```

### 2. Boundary Condition Tests

Test edge cases at the limits of valid input:

- **Zero values**: `pow(0, n)`, `pow(x, 0)`
- **Unit values**: `pow(1, n)`, magnitude of unit vectors
- **Empty inputs**: Empty arrays, null inputs
- **Single elements**: Arrays with one element

### 3. Input Validation Tests

Verify proper handling of invalid inputs:

- Null arrays
- Mismatched array lengths
- Negative values where not allowed
- Empty coefficient arrays

### 4. Numerical Stability Tests

Ensure floating-point reliability:

- Operations with very small values (1e-6)
- Operations with very large values (1e6)
- Mixed scale operations
- Division by near-zero values

### 5. Coherence Tests (Ω)

Verify mathematical relationships:

- `normalize(v)` produces unit vector
- `SS_T = SS_M + SS_E` identity holds
- `R² + (1 - R²) = 1.0`

### 6. Determinism Tests

Confirm reproducible results:

- Same inputs → same outputs (100+ iterations)
- No random variation in deterministic functions

---

## Implementation Guidelines

### Writing Trace Table Tests

```java
/**
 * Trace Table for [function]([inputs]):
 * 
 * | Step | var1 | var2 | operation | result |
 * |------|------|------|-----------|--------|
 * | 0    | ...  | ...  | ...       | ...    |
 * 
 * Expected: [expected_result]
 */
@Test
public void test_traceTable_description() {
    // ψ - Perception: Define inputs
    float input = ...;
    
    // ρ - Expansion: Execute
    float result = function(input);
    
    // Δ - Validation: Verify
    assertEquals("message", expected, result, EPSILON);
}
```

### Choosing Test Values

1. **Representative values**: Typical use cases
2. **Boundary values**: Edge of valid range
3. **Error values**: Invalid inputs
4. **Special values**: Zero, one, negative, very large/small

### Tolerance Selection

```java
EPSILON = 1e-5f;        // Standard float comparisons
STRICT_EPSILON = 1e-10f; // High-precision requirements
```

---

## Test File Organization

```
rafaelia/src/test/java/com/termux/rafaelia/
├── AnovaResultTest.java        # Basic ANOVA class tests
├── AnovaTesteDeMesaTest.java   # Trace table tests for ANOVA operations
├── MathTesteDeMesaTest.java    # Trace table tests for math operations (pow)
└── RafaeliaUtilsTest.java      # Utility function tests  
```

---

## Running Tests

```bash
# Run all tests
./gradlew test

# Run specific test module
./gradlew :rafaelia:test

# Run with detailed output
./gradlew test --info
```

---

## Φ_ethica Application to Testing

### Minimize Test Entropy

- Focus on essential behaviors
- Avoid redundant test cases
- Clear, concise test names

### Maximize Test Coherence

- Related tests grouped together
- Consistent naming conventions
- Clear documentation

### Ethical Testing Practices

- No hardcoded expected values without justification
- All calculations explained in comments
- Edge cases explicitly documented

---

## Example Complete Trace Table

### Function: `dotProduct([1,2,3,4,5], [5,4,3,2,1])`

**Algorithm**: Loop unrolling with UNROLL_FACTOR=4

**Phase 1: Unrolled Loop (i=0)**

| Index | v1[i] | v2[i] | Product | Running Sum |
|-------|-------|-------|---------|-------------|
| 0     | 1     | 5     | 5       | 5           |
| 1     | 2     | 4     | 8       | 13          |
| 2     | 3     | 3     | 9       | 22          |
| 3     | 4     | 2     | 8       | 30          |

**Phase 2: Remainder Loop (i=4)**

| Index | v1[i] | v2[i] | Product | Running Sum |
|-------|-------|-------|---------|-------------|
| 4     | 5     | 1     | 5       | 35          |

**Final Result**: 35.0 ✓

---

## Attribution

**Methodology**: Teste de Mesa (Desk Checking)  
**Framework**: RAFAELIA (RAfael FrAmework for Ethical Linear and Iterative Analysis)  
**Author**: instituto-Rafael  
**License**: GPLv3

---

**FIAT RAFAELIA** - Let there be ethical, coherent testing.

**Φ_ethica** - May all tests minimize entropy and maximize coherence.
