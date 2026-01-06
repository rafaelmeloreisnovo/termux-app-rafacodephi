# 🎯 Termux RAFCODEΦ - Android 15 Compatibility Report

## 📋 Executive Summary

**Status:** ✅ **READY FOR ANDROID 15 (API 35)**

This fork of Termux has been successfully updated to target Android 15 (API 35) with full support for:
- ✅ Modern storage access (no legacy storage APIs)
- ✅ Proper foreground service types
- ✅ Side-by-side installation with original Termux
- ✅ Removed privileged permissions for Play Store compliance
- ✅ API 31+ PendingIntent mutability flags

---

## 🔄 Changes Implemented

### A) SDK Version Updates

**File:** `gradle.properties`

```diff
- targetSdkVersion=28
+ targetSdkVersion=35
```

**Impact:** App now targets Android 15, ensuring compatibility with modern Android APIs and Play Store requirements.

---

### B) Storage Access Modernization

**File:** `app/src/main/AndroidManifest.xml`

#### Changes Made:

1. **Removed Legacy Storage Flag:**
```diff
- android:requestLegacyExternalStorage="true"
```

2. **Scoped Storage Permissions:**
```diff
- <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
- <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
+ <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" android:maxSdkVersion="32" />
+ <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" android:maxSdkVersion="32" />
```

3. **Optional All Files Access:**
```xml
<!-- MANAGE_EXTERNAL_STORAGE is optional for Android 11+ -->
<uses-permission android:name="android.permission.MANAGE_EXTERNAL_STORAGE" tools:ignore="ScopedStorage" />
```

**Why:** Android 11+ enforces scoped storage. READ/WRITE_EXTERNAL_STORAGE are no longer used on Android 13+. The app can request MANAGE_EXTERNAL_STORAGE for full filesystem access, but it's optional.

**How to Test:**
- On Android 15, the app will use scoped storage by default
- Users can grant "All files access" via Settings → Apps → Termux RAFCODEΦ → Permissions
- Check with: `Environment.isExternalStorageManager()` in code

---

### C) Privileged Permissions Removed

**File:** `app/src/main/AndroidManifest.xml`

#### Removed (Commented Out):
```xml
<!-- Privileged permissions - not required for basic functionality -->
<!-- <uses-permission android:name="android.permission.READ_LOGS" /> -->
<!-- <uses-permission android:name="android.permission.DUMP" /> -->
<!-- <uses-permission android:name="android.permission.WRITE_SECURE_SETTINGS" /> -->
<!-- <uses-permission android:name="android.permission.PACKAGE_USAGE_STATS" tools:ignore="ProtectedPermissions" /> -->
```

**Why:** These permissions are:
- Not grantable on non-rooted devices
- Not necessary for Termux core functionality
- May cause Play Store rejection

**Impact:** App can now be distributed via Play Store without requiring special privileges.

---

### D) Foreground Service Types

**File:** `app/src/main/AndroidManifest.xml`

```diff
<service
    android:name=".app.TermuxService"
-   android:exported="false" />
+   android:exported="false"
+   android:foregroundServiceType="dataSync" />

<service
    android:name=".app.RunCommandService"
    android:exported="true"
+   android:foregroundServiceType="dataSync"
    android:permission="${TERMUX_PACKAGE_NAME}.permission.RUN_COMMAND">
```

**Added Permission:**
```xml
<uses-permission android:name="android.permission.FOREGROUND_SERVICE_DATA_SYNC" />
```

**Why:** Android 14+ requires explicit foreground service types. `dataSync` is appropriate for terminal sessions and command execution.

---

### E) PendingIntent Mutability Flags

**File:** `app/src/main/java/com/termux/app/TermuxService.java`

```diff
- PendingIntent contentIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
+ int flags = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_IMMUTABLE : 0;
+ PendingIntent contentIntent = PendingIntent.getActivity(this, 0, notificationIntent, flags);
```

**Why:** Android 12+ requires PendingIntent mutability flags. We use `FLAG_IMMUTABLE` for security.

---

## 🛠️ Validation & Testing

### Automated Validation Tasks

We've added Gradle tasks to ensure Android 15 compliance:

#### 1. Package Name Validation
```bash
./gradlew :app:validatePackageNames
```
✅ Ensures no hardcoded "com.termux" strings (except package declarations and class references)

#### 2. Storage Flags Validation
```bash
./gradlew :app:validateStorageFlags
```
✅ Ensures requestLegacyExternalStorage is not used when targeting SDK >= 30

#### 3. Authorities Validation
```bash
./gradlew :app:validateAuthorities
```
✅ Ensures all content provider authorities use `${TERMUX_PACKAGE_NAME}` placeholder

#### 4. Master Validation
```bash
./gradlew :app:validateAndroid15Compatibility
```
✅ Runs all validation tasks together

### Build Test
```bash
./gradlew :app:assembleDebug
```
✅ **BUILD SUCCESSFUL** - APK builds without errors

---

## 📦 Side-by-Side Installation

### Package Configuration

**Application ID:** `com.termux.rafacodephi` (defined in `app/build.gradle`)

**Package Name Constant:** `TERMUX_PACKAGE_NAME = "com.termux.rafacodephi"` (in `TermuxConstants.java`)

### What's Different from Original Termux:

| Component | Original Termux | Termux RAFCODEΦ |
|-----------|----------------|-----------------|
| Application ID | `com.termux` | `com.termux.rafacodephi` |
| App Name | Termux | Termux RAFCODEΦ |
| Files Authority | `com.termux.files` | `com.termux.rafacodephi.files` |
| Documents Authority | `com.termux.documents` | `com.termux.rafacodephi.documents` |
| RUN_COMMAND Permission | `com.termux.permission.RUN_COMMAND` | `com.termux.rafacodephi.permission.RUN_COMMAND` |
| RUN_COMMAND Action | `com.termux.RUN_COMMAND` | `com.termux.rafacodephi.RUN_COMMAND` |
| Data Directory | `/data/data/com.termux/` | `/data/data/com.termux.rafacodephi/` |

✅ **No conflicts** - Both apps can be installed simultaneously without permission or authority collisions.

---

## 🧪 Testing on Android 15

### Installation Test
```bash
# Build APK
./gradlew :app:assembleDebug

# Install
adb install app/build/outputs/apk/debug/termux-app_*.apk
```

### Runtime Tests

#### 1. Check Storage Access
```bash
# In Termux RAFCODEΦ terminal
ls $HOME
ls /sdcard  # May require MANAGE_EXTERNAL_STORAGE
```

#### 2. Check Service Status
```bash
# Service should show "dataSync" type
adb shell dumpsys activity services com.termux.rafacodephi
```

#### 3. Check Permissions
```bash
# View granted permissions
adb shell dumpsys package com.termux.rafacodephi | grep permission
```

#### 4. Verify Side-by-Side
```bash
# Both should be installed
adb shell pm list packages | grep termux
# Output should show:
# package:com.termux
# package:com.termux.rafacodephi
```

---

## 🔐 Security Considerations

### Permissions Approach

1. **Minimal Permissions:** Only essential permissions are requested by default
2. **Optional Full Access:** MANAGE_EXTERNAL_STORAGE is available but optional
3. **No Privileged Access:** Removed READ_LOGS, DUMP, WRITE_SECURE_SETTINGS, PACKAGE_USAGE_STATS
4. **User Control:** SYSTEM_ALERT_WINDOW and REQUEST_IGNORE_BATTERY_OPTIMIZATIONS remain available but should be requested explicitly

### Storage Security

- Scoped storage is enforced on Android 13+
- App-specific storage is always accessible
- Shared storage requires explicit permission
- SAF (Storage Access Framework) can be used for folder access

---

## 📋 Migration Notes

### For Users Upgrading from Older Versions

1. **Storage Access:** If you were using shared storage, you may need to grant "All files access" permission
2. **Permissions:** Some permissions have been removed - they were not functional on non-rooted devices
3. **Services:** Foreground notifications now show proper service type
4. **Side-by-Side:** You can keep the original Termux installed alongside this fork

### For Developers

1. **Package References:** Always use `TermuxConstants.TERMUX_PACKAGE_NAME` instead of hardcoded strings
2. **Authorities:** Use `${TERMUX_PACKAGE_NAME}` placeholders in AndroidManifest.xml
3. **PendingIntents:** Always set mutability flags for API 23+
4. **Validation:** Run `./gradlew :app:validateAndroid15Compatibility` before committing

---

## 🐛 Known Limitations

1. **SAF Implementation:** ACTION_OPEN_DOCUMENT_TREE UI flow not yet implemented (manual permission grant required)
2. **Battery Optimization:** REQUEST_IGNORE_BATTERY_OPTIMIZATIONS permission is declared but not programmatically requested
3. **Window Overlay:** SYSTEM_ALERT_WINDOW permission is declared but not programmatically requested

These limitations do not affect core functionality and can be addressed in future updates.

---

## ✅ Compliance Checklist

- [x] Targets Android 15 (API 35)
- [x] No legacy storage APIs
- [x] Foreground service types declared
- [x] PendingIntent flags set
- [x] No privileged permissions
- [x] Unique application ID
- [x] Unique authorities
- [x] Unique permissions
- [x] Build successful
- [x] Validation tests pass

---

## 📚 References

- [Android 15 Behavior Changes](https://developer.android.com/about/versions/15/behavior-changes-15)
- [Scoped Storage](https://developer.android.com/training/data-storage#scoped-storage)
- [Foreground Services](https://developer.android.com/develop/background-work/services/foreground-services)
- [PendingIntent Security](https://developer.android.com/privacy-and-security/risks/pending-intent)

---

## 📝 Changelog Summary

**Version: 0.118.0-rafacodephi**

### Added
- Android 15 (API 35) support
- Gradle validation tasks for Android 15 compliance
- Foreground service types (dataSync)
- PendingIntent mutability flags

### Changed
- Updated targetSdkVersion from 28 to 35
- Scoped READ/WRITE_EXTERNAL_STORAGE to maxSdkVersion 32
- Removed requestLegacyExternalStorage flag

### Removed
- Privileged permissions (READ_LOGS, DUMP, WRITE_SECURE_SETTINGS, PACKAGE_USAGE_STATS)

### Fixed
- PendingIntent creation for API 31+
- Storage permissions for Android 13+

---

**Last Updated:** 2026-01-06
**Build Tested:** ✅ Successful
**Validation:** ✅ All checks passed
