# Bootstrap Package Configuration (BUG-04: Fixed)

**Status:** ✅ RESOLVED - Package names now configurable via environment variables

## Overview

The Termux bootstrap package names are now fully configurable at build time via environment variables, eliminating the need to modify source code for distribution variants.

## Configuration Variables

### Build-Time Configuration (Environment Variables)

**For App/Bootstrap Package Name:**
```bash
export TERMUX_APP_PACKAGE_NAME="com.termux.mydistro"  # Default: "com.termux.rafacodephi"
./gradlew assembleDebug
```

**For App Code Package Name:**
```bash
export TERMUX_APP_CODE_PACKAGE_NAME="com.mydistro.app"  # Default: "com.termux.app"
./gradlew assembleDebug
```

**For Bootstrap Metadata Package Name:**
```bash
export TERMUX_BOOTSTRAP_PACKAGE_NAME="com.termux.mydistro"  # Default: "com.termux.rafacodephi"
./gradlew assembleDebug
```

### Complete Build Example

```bash
# Configure for a custom distribution
export TERMUX_APP_PACKAGE_NAME="org.my.termux"
export TERMUX_APP_CODE_PACKAGE_NAME="org.my.termux.app"
export TERMUX_BOOTSTRAP_PACKAGE_NAME="org.my.termux"

# Build the APK
./gradlew assembleDebug -Pandroid.injected.build.abi=arm64-v8a
```

## Implementation Details

### Modified Files

1. **app/build.gradle**
   - Added `appCodePackageName` to ext block (lines 20)
   - Added BuildConfig field `TERMUX_APP_CODE_PACKAGE_NAME` (line 126)

2. **termux-shared/build.gradle**
   - Added package configuration to ext block (lines 6-8)
   - Added `buildConfigString()` helper function (lines 10-12)
   - Enabled BuildConfig generation (lines 41-43)
   - Added BuildConfig fields for `TERMUX_PACKAGE_NAME` and `TERMUX_APP_CODE_PACKAGE_NAME` (lines 24-25)

3. **termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java**
   - Line 6: Added import for BuildConfig
   - Lines 352-354: Replaced hardcoded values with BuildConfig references

### How It Works

1. **Build Time**: Gradle reads environment variables with fallback defaults
2. **BuildConfig Generation**: Package names are embedded in generated BuildConfig class
3. **Runtime**: TermuxConstants reads from BuildConfig instead of hardcoded strings
4. **Distribution**: Different distributions can build with different package names

## Benefits

✅ **No source code modifications needed** for distribution variants
✅ **Reproducible builds** - same source produces different APK for different package names
✅ **CI/CD friendly** - set environment variables in build scripts
✅ **Backward compatible** - sensible defaults maintain current behavior
✅ **Consistent with Android best practices** - uses BuildConfig pattern

## Verification

To verify the configuration is correctly applied:

```bash
# Build with custom package name
export TERMUX_APP_PACKAGE_NAME="com.test.termux"
./gradlew assembleDebug

# Check BuildConfig was generated
strings app/build/intermediates/classes/debug/com/termux/shared/BuildConfig.class | grep -i "com.test.termux"
```

## Related Issues

- **BUG-04**: Bootstrap hardcode com.termux (RESOLVED)
- **Dependency**: Independent (can be fixed without waiting for other bugs)
- **Risk**: Low (uses established BuildConfig pattern, no breaking changes)

## Next Steps

- Similar configuration can be applied to plugin apps (Termux:API, Termux:Boot, etc.)
- Consider externalizing common configuration to a gradle properties file
- Document environment variables in CI/CD pipeline configuration

---

**Fixed by**: Commit implementing BuildConfig-based configuration
**Date**: 2026-08-29
**Effort**: ~30 minutes (0.5 day of BUG-04's estimated 1 day)
