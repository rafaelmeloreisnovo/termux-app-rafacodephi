# Package Name Configuration Guide

**Status**: BUG-04 IMPLEMENTED  
**Date**: 2026-08-23  
**Authority**: termux-app-rafacodephi  

## Overview

The termux-app package name is now fully configurable via environment variables. Previously hardcoded as `com.termux.rafacodephi`, it can now be customized without modifying source code.

## Configuration Variables

### Primary

| Variable | Purpose | Default | Set In |
|----------|---------|---------|--------|
| `TERMUX_APP_PACKAGE_NAME` | Application ID (package name for the main app) | `com.termux.rafacodephi` | app/build.gradle |
| `TERMUX_BOOTSTRAP_PACKAGE_NAME` | Bootstrap metadata package name | `com.termux.rafacodephi` | app/build.gradle |

### Secondary (Propagated)

These are automatically derived from the primary values and cannot be overridden:

- `TERMUX_API_PACKAGE_NAME` = `{TERMUX_APP_PACKAGE_NAME}.api`
- `TERMUX_BOOT_PACKAGE_NAME` = `{TERMUX_APP_PACKAGE_NAME}.boot`
- `TERMUX_FLOAT_PACKAGE_NAME` = `{TERMUX_APP_PACKAGE_NAME}.window`
- `TERMUX_WIDGET_PACKAGE_NAME` = `{TERMUX_APP_PACKAGE_NAME}.widget`
- `TERMUX_STYLING_PACKAGE_NAME` = `{TERMUX_APP_PACKAGE_NAME}.styling`
- `TERMUX_TASKER_PACKAGE_NAME` = `{TERMUX_APP_PACKAGE_NAME}.tasker`

## Usage

### Environment Variables

Set before building:

```bash
# Use default package name
./gradlew :app:assembleDebug

# Use custom package name
TERMUX_APP_PACKAGE_NAME=com.example.myapp ./gradlew :app:assembleDebug

# Use custom bootstrap package name
TERMUX_BOOTSTRAP_PACKAGE_NAME=com.example.bootstrap TERMUX_APP_PACKAGE_NAME=com.example.myapp ./gradlew :app:assembleDebug
```

### Local Build Configuration

Create `local.properties` (not committed to git) and add:

```properties
# local.properties
termux.app.package.name=com.example.myapp
termux.bootstrap.package.name=com.example.bootstrap
```

Then update `app/build.gradle` to read from properties:

```groovy
appPackageName = project.findProperty('termux.app.package.name') ?: System.getenv("TERMUX_APP_PACKAGE_NAME") ?: "com.termux.rafacodephi"
bootstrapMetadataPackageName = project.findProperty('termux.bootstrap.package.name') ?: System.getenv("TERMUX_BOOTSTRAP_PACKAGE_NAME") ?: "com.termux.rafacodephi"
```

## Implementation Details

### Files Modified

1. **app/build.gradle**
   - Line 84: `namespace = project.ext.appPackageName` (was hardcoded)
   - Lines 124-125: Added buildConfigField exports for runtime access

2. **app/src/main/java/com/termux/app/BootstrapHandoffReceiver.java**
   - Line 11: Import BuildConfig
   - Lines 41-43: Dynamic action/authority construction from BuildConfig.TERMUX_PACKAGE_NAME

### BuildConfig Fields

The following are now available as BuildConfig constants:

```java
String BuildConfig.TERMUX_PACKAGE_NAME              // Main app package
String BuildConfig.BOOTSTRAP_METADATA_PACKAGE_NAME  // Bootstrap metadata package
```

### Manifest Placeholders

The AndroidManifest.xml can reference the package name via placeholder:

```xml
${TERMUX_PACKAGE_NAME}
```

This is automatically substituted from `project.ext.appPackageName` during build.

## Constraints & Guarantees

### Rules (must be satisfied)

1. **Uniqueness**: Each build must have a unique package name (no two APKs with same package name)
2. **Stability**: Package name cannot change between updates on same device (it's the canonical app identity)
3. **Schema**: Must be valid Android package name format (alphanumeric + dots)

### Falsifier

The build fails if:
- `TERMUX_APP_PACKAGE_NAME` contains invalid characters
- `TERMUX_BOOTSTRAP_PACKAGE_NAME` is empty but `TERMUX_APP_PACKAGE_NAME` is set (implicit contract violation)
- Namespace cannot be set to the configured value

## Side-by-Side Builds

This enables canonical side-by-side installation:

```bash
# Production
TERMUX_APP_PACKAGE_NAME=com.termux.rafacodephi ./gradlew :app:assembleRelease
# → APK: termux-rafcodephi-release-universal.apk

# Development
TERMUX_APP_PACKAGE_NAME=com.termux.rafacodephi.dev ./gradlew :app:assembleDebug
# → APK: termux-rafcodephi-debug-universal.apk

# Testing
TERMUX_APP_PACKAGE_NAME=com.termux.rafacodephi.test ./gradlew :app:assembleDebug
# → APK: termux-rafcodephi-debug-universal.apk
```

All three can coexist on the same Android device.

## Verification

To verify configuration was applied:

```bash
# Check generated BuildConfig
unzip -p build/outputs/apk/debug/termux-rafcodephi-debug-universal.apk \
  classes.dex | strings | grep "TERMUX_PACKAGE_NAME"

# Or via Android (post-install)
adb shell am dump-config com.termux.rafacodephi | grep package
```

## Migration from Hardcoded Values

### Before (BUG-04)

- Package name hardcoded in 5+ locations
- Required source code changes to customize
- Side-by-side builds required fork + separate repository

### After (BUG-04 fixed)

- Package name set via environment variable
- No source code changes needed
- Side-by-side builds in same repository

## Related

- **BUG-04**: Package hardcode → configuration (this work)
- **BUG-05**: Stack overflow (independent parallel)
- **BUG-07**: Hash mismatch (independent parallel)
- **Phase 7**: Frida Desktop integration (blocked until all independent bugs closed)

---

**Status**: VERIFIED_LOCAL  
**Exit Criterion**: Package name configurable via environment; verified in build.gradle, BootstrapHandoffReceiver, and manifest placeholders  
**Claim Allowed**: false (device validation needed for full certification)  

