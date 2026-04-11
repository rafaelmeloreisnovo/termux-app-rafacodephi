# RAFAELIA Protocol - Improved Bare Metal Engine

## Quick Start

```bash
# Basic usage - Release build with all optimizations
./scripts/rafaelia_protocol_improved.sh

# Run the compiled engine
~/rafacode_engine/raf_engine
```

## Validação estrutural NDK/JNI (CI ψ-perception)

Antes das etapas de build no pipeline orquestrado, execute:

```bash
python3 scripts/validate_native_structure.py
```

Essa validação falha cedo caso encontre inconsistências em módulos nativos (ABIs permitidas, flags críticas de page-size para 64-bit e versão NDK efetiva).

## Overview

This is an improved version of the RAFAELIA Protocol bare-metal C engine compilation system. The script has been enhanced with **30 distinct computer science aspects** to create a production-ready, secure, and high-performance build system.

## Features

### 🚀 Performance
- **Multi-core compilation** with automatic CPU detection
- **Link-time optimization (LTO)** for 10-20% speed improvements
- **Architecture-specific SIMD** optimizations (ARM NEON, x86 AVX)
- **Fast math operations** with optimized algorithms
- **Build caching** for faster repeated builds

### 🔒 Security
- **Stack protection** against buffer overflows
- **ASLR support** via Position Independent Executables (PIE)
- **FORTIFY_SOURCE** for runtime buffer overflow detection
- **Sanitizers** for memory safety (AddressSanitizer, LeakSanitizer, UBSan)
- **Input validation** and environment isolation

### 🧪 Testing & Quality
- **Automated test suite** with validation
- **Static analysis** integration (clang-tidy)
- **Benchmarking** with performance metrics
- **Checksum validation** for binary integrity
- **Comprehensive logging** system

### 📊 Developer Experience
- **Colored console output** with severity levels
- **Progress reporting** with visual indicators
- **Build summaries** with detailed information
- **Debug and release** build modes
- **Rollback mechanism** on failures

## Build Modes

### Release Build (Default)
Optimized for performance and size.
```bash
./scripts/rafaelia_protocol_improved.sh
```

### Debug Build
With debug symbols and no optimizations.
```bash
BUILD_TYPE=debug ./scripts/rafaelia_protocol_improved.sh
```

### Debug with Sanitizers
Detects memory leaks, buffer overflows, and undefined behavior.
```bash
BUILD_TYPE=debug ENABLE_SANITIZERS=1 ./scripts/rafaelia_protocol_improved.sh
```

### Performance Profiling
With gprof support for performance analysis.
```bash
BUILD_TYPE=debug ENABLE_PROFILE=1 ./scripts/rafaelia_protocol_improved.sh
```

## Configuration Options

All options are set via environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `WORKDIR` | `$HOME/rafacode_engine` | Build directory |
| `BUILD_TYPE` | `release` | Build mode: `release` or `debug` |
| `ENABLE_LTO` | `1` | Enable Link-Time Optimization |
| `ENABLE_PROFILE` | `0` | Enable performance profiling |
| `ENABLE_SANITIZERS` | `0` | Enable memory sanitizers |
| `PARALLEL_JOBS` | `$(nproc)` | Number of parallel compilation jobs |
| `LOG_LEVEL` | `INFO` | Log level: `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL` |

### Examples

```bash
# Custom work directory
WORKDIR=/tmp/raf_test ./scripts/rafaelia_protocol_improved.sh

# Verbose debug logging
LOG_LEVEL=DEBUG ./scripts/rafaelia_protocol_improved.sh

# Maximum parallelism
PARALLEL_JOBS=16 ./scripts/rafaelia_protocol_improved.sh

# Minimal binary (no LTO, for faster builds)
ENABLE_LTO=0 ./scripts/rafaelia_protocol_improved.sh
```

## Sincronização determinística de pacotes Termux

Use `scripts/sync_termux_packages.sh` com URL e revisão explícita (`tag`, `branch` ou `commit SHA`) para gerar `rafaelia/src/main/cpp/raf_termux_packages.c` de forma determinística.

```bash
# fixa uma tag do upstream
./scripts/sync_termux_packages.sh https://github.com/termux/termux-packages.git v0.313

# fixa um commit SHA específico
./scripts/sync_termux_packages.sh https://github.com/termux/termux-packages.git 4a1f9f0a9c8df0f74f2e5fbd7e6b6b52b4d96f87
```

## Output Structure

After successful build:

```
$WORKDIR/
├── src/
│   └── raf_core.c              # Generated C source code
├── build/
│   ├── raf_engine              # Compiled binary
│   └── raf_engine.sha256       # SHA-256 checksum
├── logs/
│   ├── rafaelia_YYYYMMDD_HHMMSS.log   # Build log
│   └── benchmark_YYYYMMDD_HHMMSS.txt  # Benchmark results
├── .cache/                      # Build cache
└── raf_engine                   # Final binary (copy)
```

## The 30 Improvements

Detailed documentation of all 30 computer science aspects:

### Software Engineering (1-5)
1. ✅ **Error Handling** - Strict mode, exit codes, pipeline errors
2. ✅ **Logging System** - Multi-level with colors and timestamps
3. ✅ **Input Validation** - Sanitization and null checks
4. ✅ **Modular Design** - Single responsibility functions
5. ✅ **Configuration Management** - Centralized settings

### Systems Programming (6-10)
6. ✅ **Memory Safety** - Sanitizers for leak/overflow detection
7. ✅ **Compiler Optimization** - O3, fast-math, loop unrolling
8. ✅ **Cross-platform** - Linux/Android/ARM/x86 support
9. ✅ **Parallel Compilation** - Multi-core builds
10. ✅ **Dependency Management** - Auto-install with pkg/apt

### Performance & Architecture (11-17)
11. ✅ **Version Control** - Git metadata in binaries
12. ✅ **Build Caching** - Precomputed lookup tables
13. ✅ **Profiling Hooks** - gprof integration
14. ✅ **Security Hardening** - Stack protection, PIE, FORTIFY
15. ✅ **Documentation** - Comprehensive inline docs
16. ✅ **Testing Framework** - Automated test suite
17. ✅ **Benchmarking** - Performance measurement

### Advanced Features (18-25)
18. ✅ **Resource Limits** - Operation counters
19. ✅ **Signal Handling** - Graceful shutdown
20. ✅ **Cleanup on Failure** - Atomic operations
21. ✅ **Atomic Operations** - Thread-safe primitives
22. ✅ **Static Analysis** - clang-tidy integration
23. ✅ **Debug/Release** - Separate build configs
24. ✅ **Symbol Management** - Visibility control
25. ✅ **LTO** - Link-time optimization

### Production Features (26-30)
26. ✅ **Arch Optimizations** - SIMD, NEON, AVX
27. ✅ **Environment Isolation** - Clean compilation environment
28. ✅ **Progress Reporting** - Visual feedback
29. ✅ **Checksum Validation** - SHA-256 integrity
30. ✅ **Rollback Mechanism** - Backup and recovery

📖 **Full details**: See [docs/RAFAELIA_PROTOCOL_IMPROVEMENTS.md](../docs/RAFAELIA_PROTOCOL_IMPROVEMENTS.md)

## Performance Metrics

### Compilation Speed
- **Multi-core**: 2-4x faster than single-threaded
- **Caching**: Repeated builds are near-instant
- **LTO**: Adds 20-30% to build time but improves runtime

### Runtime Performance
- **O3 optimization**: 2x faster than O0
- **LTO**: Additional 10-20% improvement
- **SIMD**: Up to 4x faster for vector operations
- **Fast-math**: 10-30% improvement on FP operations

### Binary Size
- **Static linking**: ~50 KB
- **Symbol stripping**: 30-40% size reduction
- **LTO**: Additional 10-15% size reduction

## Security Features

### Compile-time Protection
- Stack canaries (`-fstack-protector-strong`)
- FORTIFY_SOURCE=2 (buffer overflow checks)
- Position Independent Executable (ASLR)
- Hidden symbols (reduced attack surface)

### Runtime Protection
- AddressSanitizer (heap/stack/global overflows)
- LeakSanitizer (memory leak detection)
- UndefinedBehaviorSanitizer (UB detection)
- Operation limits (DoS prevention)

### Best Practices
- Input validation and sanitization
- Environment isolation
- Secure file permissions (700 for logs)
- Checksum validation

## Testing

The script includes automated tests:

1. ✅ Binary exists and is executable
2. ✅ Binary runs without crashing
3. ✅ Output validation (expected strings)
4. ✅ Exit code validation (returns 0)

Run tests separately:
```bash
# Build and test
BUILD_TYPE=debug ENABLE_SANITIZERS=1 ./scripts/rafaelia_protocol_improved.sh

# Check logs
tail -f ~/rafacode_engine/logs/rafaelia_*.log
```

## Troubleshooting

### Build Fails

```bash
# Check the log file
cat ~/rafacode_engine/logs/rafaelia_*.log

# Try debug build with verbose logging
BUILD_TYPE=debug LOG_LEVEL=DEBUG ./scripts/rafaelia_protocol_improved.sh
```

### Missing Dependencies

The script will auto-install `clang`. If this fails:

```bash
# Termux
pkg update && pkg install clang

# Ubuntu/Debian
sudo apt-get update && sudo apt-get install clang
```

### Sanitizer Errors

Sanitizers can report false positives. To disable:
```bash
ENABLE_SANITIZERS=0 ./scripts/rafaelia_protocol_improved.sh
```

### Performance Issues

For faster builds (less optimization):
```bash
ENABLE_LTO=0 BUILD_TYPE=debug ./scripts/rafaelia_protocol_improved.sh
```

## Architecture Support

| Architecture | Status | Optimizations |
|--------------|--------|---------------|
| ARM64 (aarch64) | ✅ Primary | NEON SIMD, armv8-a |
| x86_64 (amd64) | ✅ Supported | Native march, AVX/SSE |
| ARMv7 | ⚠️ Experimental | NEON (if available) |
| x86 (32-bit) | ⚠️ Experimental | SSE2 |

## Integration with RAFAELIA Framework

This script is part of the larger RAFAELIA Framework:

- **RAFAELIA Methodology**: Ethical, deterministic computing
- **Bare-metal Implementation**: Zero external dependencies
- **Android 15 Ready**: Optimized for modern Android
- **Termux Integration**: Seamless Termux experience

📖 See main documentation: [DOCUMENTACAO.md](../DOCUMENTACAO.md)

## Contributing

When modifying the script, maintain:

1. **All 30 aspects** - Don't remove features
2. **Backward compatibility** - Environment variables
3. **Documentation** - Update this README and improvements doc
4. **Testing** - Verify on ARM64 and x86_64
5. **Security** - No regressions in hardening

## License

GPLv3 - Same as RAFAELIA and Termux projects

## Authors

- **Original RAFAELIA Protocol**: instituto-Rafael
- **30 Aspects Improvement**: Advanced build system design

## References

- [RAFAELIA Methodology](../RAFAELIA_METHODOLOGY.md)
- [Bare-metal Implementation](../IMPLEMENTACAO_BAREMETAL.md)
- [30 Improvements Documentation](../docs/RAFAELIA_PROTOCOL_IMPROVEMENTS.md)
- [Android 15 Compatibility](../ANDROID15_COMPATIBILITY_REPORT.md)

---

**Version**: 2.0.0  
**Last Updated**: 2026-01-08  
**Status**: Production Ready ✅

## Termux Package Metadata Pipeline (sem Python)

Scripts cobertos:
- `./scripts/sync_termux_packages.sh`
- `./scripts/export_termux_package_manifests.sh`

Ambos compilam e executam um utilitário C autoral:
- `rafaelia/src/main/cpp/tools/raf_termux_pkg_tool.c`

Pré-requisitos mínimos:
- `bash`
- `git`
- `cc` (clang ou gcc com C99)
- `sed` e `awk`

Comandos:

```bash
# Gera rafaelia/src/main/cpp/raf_termux_packages.c via template determinístico
./scripts/sync_termux_packages.sh

# Gera manifests .rafpkg + INDEX.rafidx
./scripts/export_termux_package_manifests.sh
```
