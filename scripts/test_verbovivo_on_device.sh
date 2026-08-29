#!/usr/bin/env bash
# Test Verbovivo on Android device via ADB
# Usage: ./scripts/test_verbovivo_on_device.sh [clean|rebuild]

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
TEST_BINARY="$BUILD_DIR/test_verbovivo_convergence_arm64"
DEVICE_PATH="/data/local/tmp/test_verbovivo_convergence"

# Functions
print_status() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
    exit 1
}

print_warn() {
    echo -e "${YELLOW}[!]${NC} $1"
}

check_adb() {
    if ! command -v adb &> /dev/null; then
        print_error "ADB not found. Install Android SDK Platform Tools."
    fi

    if ! adb devices | grep -q "device$"; then
        print_error "No Android device connected. Run: adb devices"
    fi

    print_status "ADB device found"
}

check_clang() {
    if ! command -v clang &> /dev/null; then
        print_warn "Clang not found. ARM64 cross-compilation will not be available."
        print_warn "You can build the test on the device using Termux instead."
        return 1
    fi
    print_status "Clang found"
    return 0
}

build_library() {
    echo ""
    echo "Building Verbovivo library..."
    cd "$REPO_ROOT"
    make -f Makefile.verbovivo clean >/dev/null 2>&1 || true
    make -f Makefile.verbovivo verbovivo-lib
    print_status "Verbovivo library built"
}

build_test_binary() {
    echo ""
    echo "Building test binary for ARM64..."

    # Check if ARM64 linker is available
    if ! clang -target aarch64-linux-gnu -v 2>&1 | grep -q "aarch64"; then
        print_error "ARM64 cross-compilation not available on this host"
    fi

    mkdir -p "$BUILD_DIR"

    clang -target aarch64-linux-gnu \
        -ffreestanding -nostdlib -nostdinc \
        -O2 -Wall -march=armv8-a+simd \
        -I"$REPO_ROOT" \
        "$REPO_ROOT/tests/native/test_verbovivo_convergence.c" \
        "$BUILD_DIR/libverbovivo_graph.a" \
        -o "$TEST_BINARY" 2>&1

    if [ ! -f "$TEST_BINARY" ]; then
        print_error "Failed to build test binary"
    fi

    print_status "Test binary built: $TEST_BINARY"
}

push_to_device() {
    echo ""
    echo "Pushing binary to device..."

    if [ ! -f "$TEST_BINARY" ]; then
        print_error "Test binary not found: $TEST_BINARY"
    fi

    adb push "$TEST_BINARY" "$DEVICE_PATH" >/dev/null
    adb shell chmod +x "$DEVICE_PATH"

    print_status "Binary pushed to device: $DEVICE_PATH"
}

run_test_on_device() {
    echo ""
    echo "Running test on device..."
    echo "========================================"

    if ! adb shell "$DEVICE_PATH"; then
        TEST_EXIT=$?
        echo "========================================"
        print_error "Test failed with exit code $TEST_EXIT"
    fi

    echo "========================================"
}

collect_results() {
    echo ""
    echo "Collecting test results..."

    TEST_EXIT=0
    adb shell "$DEVICE_PATH" >/dev/null 2>&1 || TEST_EXIT=$?

    if [ $TEST_EXIT -eq 0 ]; then
        print_status "All tests PASSED (exit code 0)"
        return 0
    else
        print_error "Tests FAILED (exit code $TEST_EXIT)"
        return 1
    fi
}

cleanup() {
    echo ""
    echo "Cleaning up device..."
    adb shell rm -f "$DEVICE_PATH"
    print_status "Test binary removed from device"
}

main() {
    local action="${1:-}"

    echo "=== Verbovivo Device Test Suite ==="
    echo ""

    # Check prerequisites
    check_adb

    # Determine action
    case "$action" in
        clean)
            echo "Rebuilding from scratch..."
            build_library
            if check_clang; then
                build_test_binary
                push_to_device
            else
                print_warn "Using Termux fallback (Option B in testing guide)"
            fi
            run_test_on_device
            collect_results
            cleanup
            ;;
        rebuild)
            echo "Rebuilding test binary..."
            if check_clang; then
                build_test_binary
                push_to_device
            else
                print_error "Clang required for rebuild"
            fi
            run_test_on_device
            collect_results
            cleanup
            ;;
        *)
            # Default: use existing binary or build
            if [ -f "$TEST_BINARY" ]; then
                print_status "Using existing test binary"
            else
                if check_clang; then
                    build_library
                    build_test_binary
                else
                    print_error "Test binary not found and clang not available"
                fi
            fi

            push_to_device
            run_test_on_device
            collect_results
            cleanup
            ;;
    esac

    echo ""
    print_status "Device test suite completed"
}

# Run main
main "$@"
