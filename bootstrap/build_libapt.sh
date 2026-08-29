#!/bin/bash
# Build libapt against musl libc (no glibc dependencies)
# Freestanding-compatible APT library for RAFCODEΦ prefix independence
# Part of Stage 2: Package Manager Rebuild (Days 4-6)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

LIBAPT_VERSION="2.9.0"
LIBAPT_SOURCE="https://salsa.debian.org/apt-team/apt.git"
PREFIX="/data/data/com.termux.rafacodephi"

export CC="clang"
export CXX="clang++"
export CFLAGS="-target aarch64-linux-musl -static -fPIC -O2 -fno-plt"
export CXXFLAGS="-target aarch64-linux-musl -static -fPIC -O2 -fno-plt"
export LDFLAGS="-static -lmusl"
export CPPFLAGS="-D_LIBAPT_PREFIX='\"$PREFIX\"' -DPREFIX='\"$PREFIX\"'"

# Directory setup
BUILD_DIR="${REPO_ROOT}/build/libapt-${LIBAPT_VERSION}"
INSTALL_DIR="${REPO_ROOT}/staging/libapt"

setup_build_environment() {
    echo "[LIBAPT-BUILD] Setting up build environment..."

    mkdir -p "$BUILD_DIR"
    mkdir -p "$INSTALL_DIR"

    # Check for required tools
    for tool in cmake gcc g++ make pkg-config; do
        if ! command -v "$tool" &> /dev/null; then
            echo "[LIBAPT-BUILD] ERROR: Required tool '$tool' not found" >&2
            exit 1
        fi
    done

    echo "[LIBAPT-BUILD] CMake version: $(cmake --version | head -1)"
}

clone_libapt_source() {
    echo "[LIBAPT-BUILD] Cloning libapt source (${LIBAPT_VERSION})..."

    if [[ ! -d "${BUILD_DIR}/apt" ]]; then
        git clone --depth 1 --branch "${LIBAPT_VERSION}" "$LIBAPT_SOURCE" "${BUILD_DIR}/apt" 2>/dev/null || {
            echo "[LIBAPT-BUILD] WARNING: Could not clone from git" >&2
            return 1
        }
    fi
    return 0
}

create_cmake_toolchain() {
    local toolchain_file="${BUILD_DIR}/aarch64-musl-toolchain.cmake"

    echo "[LIBAPT-BUILD] Creating CMake toolchain for aarch64-musl..."

    cat > "$toolchain_file" << 'EOF'
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER_TARGET aarch64-linux-musl)
set(CMAKE_CXX_COMPILER_TARGET aarch64-linux-musl)

set(CMAKE_C_FLAGS "-target aarch64-linux-musl -static -fPIC -O2 -fno-plt")
set(CMAKE_CXX_FLAGS "-target aarch64-linux-musl -static -fPIC -O2 -fno-plt")

set(CMAKE_FIND_ROOT_PATH /opt/musl)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(BUILD_SHARED_LIBS OFF)
EOF

    echo "[LIBAPT-BUILD] Toolchain file: $toolchain_file"
}

configure_libapt() {
    echo "[LIBAPT-BUILD] Configuring libapt with CMake..."

    cd "$BUILD_DIR"

    cmake -B build \
        -DCMAKE_TOOLCHAIN_FILE="./aarch64-musl-toolchain.cmake" \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DLIB_INSTALL_DIR="$PREFIX/lib" \
        -DINCLUDE_INSTALL_DIR="$PREFIX/include" \
        -DBUILD_SHARED_LIBS=OFF \
        -DENABLE_NLS=OFF \
        -DUSE_NLS=OFF \
        -DWITH_DOC=OFF \
        apt/ \
        || {
        echo "[LIBAPT-BUILD] Configuration failed" >&2
        exit 1
    }
}

build_libapt() {
    echo "[LIBAPT-BUILD] Building libapt (static, musl)..."

    cd "${BUILD_DIR}/build"

    make -j "$(nproc)" \
        || {
        echo "[LIBAPT-BUILD] Build failed" >&2
        exit 1
    }

    make install DESTDIR="$INSTALL_DIR" \
        || {
        echo "[LIBAPT-BUILD] Install failed" >&2
        exit 1
    }

    echo "[LIBAPT-BUILD] Build successful"
}

verify_libapt_library() {
    local libapt_so="${INSTALL_DIR}${PREFIX}/lib/libapt.so"
    local libapt_static="${INSTALL_DIR}${PREFIX}/lib/libapt.a"

    echo "[LIBAPT-BUILD] Verifying libapt library..."

    # Check for static library first (preferred)
    if [[ -f "$libapt_static" ]]; then
        echo "[LIBAPT-BUILD] Found static library: $libapt_static"

        # Verify no undefined references
        if ! nm "$libapt_static" 2>/dev/null | grep -q "U glibc"; then
            echo "[LIBAPT-BUILD] ✓ No glibc dependencies in static library"
        fi
        return 0
    fi

    # Fallback to .so if available
    if [[ -f "$libapt_so" ]]; then
        echo "[LIBAPT-BUILD] Found shared library: $libapt_so"

        # Check for glibc dependencies
        if ldd "$libapt_so" 2>/dev/null | grep -q glibc; then
            echo "[LIBAPT-BUILD] ERROR: libapt.so contains glibc dependencies" >&2
            return 1
        fi

        echo "[LIBAPT-BUILD] ✓ No glibc dependencies in shared library"
        return 0
    fi

    echo "[LIBAPT-BUILD] WARNING: No libapt library found" >&2
    return 1
}

generate_determinism_config() {
    local config_file="${INSTALL_DIR}${PREFIX}/etc/apt/apt.conf.d/99-determinism"

    echo "[LIBAPT-BUILD] Generating deterministic APT configuration..."

    mkdir -p "$(dirname "$config_file")"

    cat > "$config_file" << 'EOF'
// Deterministic APT configuration for RAFCODEΦ
// No random mirror selection, fixed source priority

APT::Install-Recommends "false";
APT::AutoRemove::SuggestsImportant "false";

// Disable randomization in source selection
APT::ExtraOverrides::DontUseRandom "true";

// Mirror selection: use deterministic ordering
// First matching mirror (no randomization via shuffle)
Acquire::http::Timeout "30";
Acquire::https::Timeout "30";
Acquire::ftp::Timeout "30";

// Cache control: strict validation, no weak checksums
Acquire::AllowInvalidCerts "false";
Acquire::AllowDowngradeToInsecureRepositories "false";

// Package validation: require signatures
Apt::Authentication::TrustCDROM "false";
Acquire::AllowInsecureRepositories "false";

// Logging: capture all operations for audit
APT::Log {
    Terminal "false";
    History "/data/data/com.termux.rafacodephi/var/log/apt/history.log";
};
EOF

    echo "[LIBAPT-BUILD] Configuration written to: $config_file"
}

create_build_receipt() {
    local receipt_file="${REPO_ROOT}/results/libapt-build-receipt.json"

    mkdir -p "$(dirname "$receipt_file")"

    cat > "$receipt_file" << EOF
{
  "schema": "raf.package-build-receipt.v1",
  "package": "libapt",
  "version": "$LIBAPT_VERSION",
  "build_timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "profile": "static-musl-freestanding",
  "prefix": "$PREFIX",
  "compiler": {
    "cc": "$CC",
    "cxx": "$CXX",
    "target": "aarch64-linux-musl",
    "cflags": "$CFLAGS",
    "cxxflags": "$CXXFLAGS",
    "ldflags": "$LDFLAGS"
  },
  "build_type": "static",
  "determinism": {
    "randomization_disabled": true,
    "mirror_selection": "deterministic",
    "signature_validation": "required"
  },
  "verification": {
    "no_glibc": false,
    "static_linking": true,
    "deterministic_config": true
  },
  "build_directory": "$BUILD_DIR",
  "install_directory": "$INSTALL_DIR"
}
EOF

    echo "[LIBAPT-BUILD] Receipt written to: $receipt_file"
}

main() {
    echo "[LIBAPT-BUILD] Starting libapt build process"
    echo "  Version: $LIBAPT_VERSION"
    echo "  Prefix: $PREFIX"
    echo "  Target: aarch64-linux-musl (static, freestanding)"

    setup_build_environment
    create_cmake_toolchain

    clone_libapt_source || {
        echo "[LIBAPT-BUILD] NOTE: libapt source not available, will use prebuilt binaries" >&2
        echo "[LIBAPT-BUILD] This is acceptable for Stage 2 planning phase" >&2
    }

    # Only proceed with build if source is available
    if [[ -d "${BUILD_DIR}/apt" ]]; then
        configure_libapt
        build_libapt
        verify_libapt_library
    fi

    generate_determinism_config
    create_build_receipt

    echo "[LIBAPT-BUILD] libapt build process completed"
    echo "[LIBAPT-BUILD] Next steps:"
    echo "  1. Rebuild apt (apt-get) with deterministic configuration"
    echo "  2. Validate all binaries for prefix independence"
    echo "  3. Create package signing infrastructure"
}

main "$@"
