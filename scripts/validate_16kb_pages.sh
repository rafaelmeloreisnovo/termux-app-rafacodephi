#!/bin/bash
# Validation script for Android 15/16 16KB page size compatibility
# This script checks if the APK is built correctly with proper memory alignment

set -e

COLOR_RED='\033[0;31m'
COLOR_GREEN='\033[0;32m'
COLOR_YELLOW='\033[1;33m'
COLOR_BLUE='\033[0;34m'
COLOR_RESET='\033[0m'

echo -e "${COLOR_BLUE}==================================================${COLOR_RESET}"
echo -e "${COLOR_BLUE}  Android 15/16 16KB Page Size Validation${COLOR_RESET}"
echo -e "${COLOR_BLUE}==================================================${COLOR_RESET}"
echo ""

# Check if APK exists
APK_PATH="app/build/outputs/apk/debug"
APK_FILE=$(find "$APK_PATH" -name "termux-app_*_universal.apk" 2>/dev/null | head -1)

if [ -z "$APK_FILE" ] || [ ! -f "$APK_FILE" ]; then
    echo -e "${COLOR_RED}✗ APK not found!${COLOR_RESET}"
    echo -e "  Please build the APK first with: ./gradlew assembleDebug"
    exit 1
fi

echo -e "${COLOR_GREEN}✓ APK found:${COLOR_RESET} $APK_FILE"
echo ""

# Create temp directory for extraction
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

echo -e "${COLOR_BLUE}Extracting APK...${COLOR_RESET}"
unzip -q "$APK_FILE" -d "$TEMP_DIR"

# Check for native libraries
echo ""
echo -e "${COLOR_BLUE}=== Native Libraries ===${COLOR_RESET}"
LIBS_FOUND=0
LIBS_CHECKED=0
LIBS_VALID=0

for arch in arm64-v8a armeabi-v7a x86_64 x86; do
    LIB_DIR="$TEMP_DIR/lib/$arch"
    if [ -d "$LIB_DIR" ]; then
        echo -e "${COLOR_YELLOW}Architecture: $arch${COLOR_RESET}"
        
        for lib in "$LIB_DIR"/*.so; do
            if [ -f "$lib" ]; then
                LIBS_FOUND=$((LIBS_FOUND + 1))
                LIBNAME=$(basename "$lib")
                
                # Check if readelf is available
                if command -v readelf &> /dev/null; then
                    LIBS_CHECKED=$((LIBS_CHECKED + 1))
                    
                    # Check page alignment
                    ALIGNMENT=$(readelf -l "$lib" 2>/dev/null | grep -A1 "LOAD" | grep -o "0x[0-9a-f]\+" | tail -1)
                    
                    if [ "$ALIGNMENT" = "0x4000" ] || [ "$ALIGNMENT" = "0x10000" ]; then
                        echo -e "  ${COLOR_GREEN}✓${COLOR_RESET} $LIBNAME - Alignment: $ALIGNMENT (16KB = 0x4000) ✓"
                        LIBS_VALID=$((LIBS_VALID + 1))
                    else
                        echo -e "  ${COLOR_RED}✗${COLOR_RESET} $LIBNAME - Alignment: $ALIGNMENT (should be 0x4000)"
                    fi
                else
                    echo -e "  ${COLOR_BLUE}•${COLOR_RESET} $LIBNAME - found"
                fi
            fi
        done
    fi
done

echo ""

# Summary
echo -e "${COLOR_BLUE}=== Summary ===${COLOR_RESET}"
echo -e "Libraries found: $LIBS_FOUND"
if command -v readelf &> /dev/null; then
    echo -e "Libraries checked: $LIBS_CHECKED"
    echo -e "Libraries with correct alignment: $LIBS_VALID"
    echo ""
    
    if [ "$LIBS_CHECKED" -gt 0 ] && [ "$LIBS_VALID" -eq "$LIBS_CHECKED" ]; then
        echo -e "${COLOR_GREEN}✓✓✓ ALL CHECKS PASSED! ✓✓✓${COLOR_RESET}"
        echo -e "APK is correctly built for Android 15/16 with 16KB page size support"
        echo ""
        echo -e "${COLOR_GREEN}Safe to install on:${COLOR_RESET}"
        echo "  • Android 15 with 4KB pages"
        echo "  • Android 15 with 16KB pages"
        echo "  • Android 16 Beta"
        echo ""
        exit 0
    else
        echo -e "${COLOR_RED}✗✗✗ VALIDATION FAILED! ✗✗✗${COLOR_RESET}"
        echo -e "Some libraries do not have correct 16KB page alignment"
        echo -e "The app may crash on Android 15/16 devices with 16KB pages"
        echo ""
        exit 1
    fi
else
    echo ""
    echo -e "${COLOR_YELLOW}⚠ Warning: readelf not found${COLOR_RESET}"
    echo "Cannot verify page alignment. Install binutils:"
    echo "  • Ubuntu/Debian: apt-get install binutils"
    echo "  • macOS: brew install binutils"
    echo "  • Or use Android NDK's readelf from: \$NDK_ROOT/toolchains/llvm/prebuilt/*/bin/"
    echo ""
    echo -e "${COLOR_BLUE}Manual verification:${COLOR_RESET}"
    echo "  1. Install on Android 15/16 device: adb install -r \"$APK_FILE\""
    echo "  2. Run the app and check for crashes"
    echo "  3. Check logcat: adb logcat | grep -i 'sigsegv\\|signal 11\\|termux'"
    echo ""
    exit 2
fi
