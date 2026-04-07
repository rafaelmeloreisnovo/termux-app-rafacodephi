# Rafaelia Learning Materials (Old)

This directory contains learning and experimental files that were used during the development and research phase of the Rafaelia module.

## Contents

This directory includes:
- **Shell scripts (.sh)**: Various experimental automation and orchestration scripts
- **Python files (.py)**: Research implementations, prototypes, and analysis tools
- **C files (.c)**: Low-level experiments, benchmarks, and algorithm implementations
- **Assembly files (.s, .asm)**: Bare-metal and performance-critical code experiments

## Purpose

These files represent the learning journey and experimentation that led to the creation of the optimized Rafaelia module found in `../src/`. They cover:

- **Structures and Architecture**: Low-level memory operations, cache optimization
- **Logic and Algorithms**: Mathematical operations, vector processing, matrix computations
- **Performance Research**: Benchmarking, profiling, optimization techniques
- **System Integration**: Various approaches to integrating with Termux

## Current Module

The production Rafaelia module is located in the parent `src/` directory and provides:
- Optimized C/JNI implementation (`src/main/cpp/rafaelia.c`)
- Java interface layer (`src/main/java/com/termux/rafaelia/`)
- Zero external dependencies
- High-performance memory and mathematical operations
- Integration with Termux app

## Usage

These files are kept for:
1. **Historical reference**: Understanding the evolution of the implementation
2. **Learning resource**: Examples of various approaches and techniques
3. **Future research**: Potential ideas for further optimization

**Note**: These files are not part of the production build and are not compiled or used by the Termux application.


## Auditoria desta estrutura

Consulte [AUDITORIA.md](./AUDITORIA.md) para achados de organização e governança do acervo legado.
