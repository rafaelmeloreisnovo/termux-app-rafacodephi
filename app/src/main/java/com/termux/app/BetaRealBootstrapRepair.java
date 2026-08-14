package com.termux.app;

import android.app.Activity;
import android.os.Build;
import android.system.Os;
import android.system.OsConstants;
import android.system.StructStat;

import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxRuntimePaths;

import org.json.JSONObject;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Beta-only migration helper for replacing an already materialized bridge
 * prefix with the real-pkg bootstrap carried by the new beta.
 *
 * The old prefix is renamed to a sibling backup before TermuxInstaller is
 * invoked. $HOME is not touched. The backup is removed only after the shared
 * BootstrapReadinessGate proves the new runtime is real-pkg ready.
 */
public final class BetaRealBootstrapRepair {

    private static final String LOG_TAG = "BetaRealBootstrapRepair";
    private static final String BACKUP_NAME = ".usr-before-real-pkg-beta";
    private static final String FAILED_NAME = ".usr-failed-real-pkg-beta";
    private static final int PROFILE_LIMIT = 64 * 1024;

    private BetaRealBootstrapRepair() {}

    public static void repair(Activity activity, Runnable whenDone) {
        TermuxRuntimePaths.init(activity);
        try {
            byte[] candidate = BootstrapWizardSource.loadAcceptedBytes(activity);
            String source = "WIZARD_SELECTED";
            if (candidate == null) {
                candidate = TermuxInstaller.loadZipBytes();
                source = "EMBEDDED";
            }

            // All immutable candidate checks happen before the current prefix is
            // moved. A profile-shaped but hash-mismatched archive cannot trigger
            // migration of a working/diagnostic old runtime.
            verifyRealPackageArchive(activity, candidate);
            verifyCandidateIntegrity(candidate);
            Logger.logInfo(LOG_TAG, "beta repair candidate accepted source=" + source
                + " blake3=" + BootstrapIntegrityVerifier.blake3Hex(candidate));

            File filesDir = TermuxRuntimePaths.filesDir();
            File prefix = TermuxRuntimePaths.prefixDir();
            File backup = new File(filesDir, BACKUP_NAME);
            File failed = new File(filesDir, FAILED_NAME);

            recoverInterruptedBackup(activity, prefix, backup, failed);

            BootstrapReadinessGate.Report before = BootstrapReadinessGate.evaluate(activity);
            if (before.isPass()) {
                cleanupTreeBestEffort(backup, filesDir);
                cleanupTreeBestEffort(failed, filesDir);
                whenDone.run();
                return;
            }

            if (backup.exists()) {
                throw new IllegalStateException("BETA_REAL_BOOTSTRAP_BACKUP_ALREADY_EXISTS: " + backup);
            }
            if (prefix.exists() && !prefix.renameTo(backup)) {
                throw new IllegalStateException("BETA_REAL_BOOTSTRAP_BACKUP_RENAME_FAILED: " + prefix + " -> " + backup);
            }

            final boolean hadBackup = backup.exists();
            Logger.logWarn(LOG_TAG, "bridge/unready prefix moved to backup=" + backup
                + " home_preserved=" + TermuxRuntimePaths.homeDirPath());

            TermuxInstaller.setupBootstrapIfNeeded(activity, () -> {
                BootstrapReadinessGate.Report after = BootstrapReadinessGate.evaluate(activity);
                if (after.isPass()) {
                    cleanupTreeBestEffort(backup, filesDir);
                    cleanupTreeBestEffort(failed, filesDir);
                    Logger.logInfo(LOG_TAG, "real-pkg beta repair PASS; previous prefix backup retired");
                    whenDone.run();
                    return;
                }

                Logger.logError(LOG_TAG, "installer returned but shared real-pkg gate is still BLOCKED: " + after.reason);
                try {
                    restoreBackupAfterRejectedInstall(prefix, backup, failed, hadBackup);
                } catch (Throwable restoreError) {
                    Logger.logStackTraceWithMessage(LOG_TAG, "Failed to restore pre-repair prefix", restoreError);
                }
                whenDone.run();
            });
        } catch (Throwable error) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Beta real-pkg repair preflight failed", error);
            throw new IllegalStateException("BETA_REAL_BOOTSTRAP_REPAIR_BLOCKED: " + error.getMessage(), error);
        }
    }

    /** Validate the archive identity before the existing prefix is moved. */
    static void verifyRealPackageArchive(Activity activity, byte[] zipBytes) throws Exception {
        if (zipBytes == null || zipBytes.length == 0) throw new IllegalArgumentException("BOOTSTRAP_ARCHIVE_EMPTY");
        JSONObject profile = null;
        boolean symlinks = false;
        try (ZipInputStream input = new ZipInputStream(new ByteArrayInputStream(zipBytes))) {
            ZipEntry entry;
            while ((entry = input.getNextEntry()) != null) {
                String name = entry.getName();
                if ("SYMLINKS.txt".equals(name)) symlinks = true;
                if (!"BOOTSTRAP_PROFILE.json".equals(name)) continue;
                ByteArrayOutputStream output = new ByteArrayOutputStream();
                byte[] buffer = new byte[4096];
                int total = 0;
                int read;
                while ((read = input.read(buffer)) != -1) {
                    total += read;
                    if (total > PROFILE_LIMIT) throw new IllegalArgumentException("BOOTSTRAP_PROFILE_TOO_LARGE");
                    output.write(buffer, 0, read);
                }
                profile = new JSONObject(new String(output.toByteArray(), StandardCharsets.UTF_8));
            }
        }
        if (profile == null) throw new IllegalStateException("BOOTSTRAP_PROFILE_MISSING");
        if (!symlinks) throw new IllegalStateException("BOOTSTRAP_SYMLINK_MANIFEST_MISSING");
        if (!"rafcodephi-bootstrap-profile/v1".equals(profile.optString("schema", "")))
            throw new IllegalStateException("BOOTSTRAP_PROFILE_SCHEMA_MISMATCH");
        if (!"real-pkg".equals(profile.optString("profile", "")) ||
            !"real-pkg".equals(profile.optString("package_layer", "")))
            throw new IllegalStateException("BETA_REQUIRES_REAL_PKG_PROFILE profile="
                + profile.optString("profile", "UNKNOWN") + " package_layer="
                + profile.optString("package_layer", "UNKNOWN"));
        if (!activity.getPackageName().equals(profile.optString("package_name", "")))
            throw new IllegalStateException("BOOTSTRAP_PACKAGE_NAME_MISMATCH");
        if (!expectedArch().equals(profile.optString("arch", "")))
            throw new IllegalStateException("BOOTSTRAP_ARCH_MISMATCH expected=" + expectedArch()
                + " observed=" + profile.optString("arch", "UNKNOWN"));
        if (profile.optBoolean("claim_allowed", true) || profile.optBoolean("release_allowed", true))
            throw new IllegalStateException("BOOTSTRAP_PROFILE_CLAIM_BOUNDARY_OPEN");
    }

    private static void verifyCandidateIntegrity(byte[] zipBytes) {
        String expected = BootstrapIntegrityVerifier.expectedHashForCurrentAbi();
        if (expected == null || expected.isEmpty())
            throw new IllegalStateException("BETA_REAL_BOOTSTRAP_BLAKE3_EXPECTATION_MISSING");
        String actual = BootstrapIntegrityVerifier.blake3Hex(zipBytes);
        if (!expected.equalsIgnoreCase(actual)) {
            throw new SecurityException("BETA_REAL_BOOTSTRAP_BLAKE3_MISMATCH expected="
                + expected.toLowerCase(Locale.US) + " actual=" + actual.toLowerCase(Locale.US));
        }
    }

    private static String expectedArch() {
        String abi = Build.SUPPORTED_ABIS != null && Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "";
        if ("armeabi-v7a".equals(abi)) return "arm";
        if ("arm64-v8a".equals(abi)) return "aarch64";
        if ("x86".equals(abi)) return "i686";
        if ("x86_64".equals(abi)) return "x86_64";
        return abi.toLowerCase(Locale.US);
    }

    private static void recoverInterruptedBackup(Activity activity, File prefix, File backup, File failed)
        throws Exception {
        if (!backup.exists()) return;
        if (!prefix.exists()) {
            if (!backup.renameTo(prefix))
                throw new IllegalStateException("INTERRUPTED_BETA_REPAIR_BACKUP_RESTORE_FAILED");
            return;
        }

        // If both trees exist after an interruption, trust only the same strong
        // readiness gate used by orchestration. A profile alone is insufficient.
        if (BootstrapReadinessGate.evaluate(activity).isPass()) {
            cleanupTreeBestEffort(backup, TermuxRuntimePaths.filesDir());
            return;
        }

        if (failed.exists()) cleanupTreeBestEffort(failed, TermuxRuntimePaths.filesDir());
        if (failed.exists())
            throw new IllegalStateException("INTERRUPTED_BETA_REPAIR_QUARANTINE_NOT_CLEAN");
        if (!prefix.renameTo(failed))
            throw new IllegalStateException("INTERRUPTED_BETA_REPAIR_FAILED_PREFIX_QUARANTINE_FAILED");
        if (!backup.renameTo(prefix))
            throw new IllegalStateException("INTERRUPTED_BETA_REPAIR_BACKUP_RESTORE_FAILED");
    }

    private static void restoreBackupAfterRejectedInstall(File prefix, File backup, File failed, boolean hadBackup)
        throws Exception {
        if (!hadBackup || !backup.exists()) return;
        if (failed.exists()) cleanupTreeBestEffort(failed, TermuxRuntimePaths.filesDir());
        if (failed.exists()) throw new IllegalStateException("REJECTED_PREFIX_QUARANTINE_NOT_CLEAN");
        if (prefix.exists() && !prefix.renameTo(failed))
            throw new IllegalStateException("REJECTED_REAL_PKG_PREFIX_QUARANTINE_FAILED");
        if (!backup.renameTo(prefix))
            throw new IllegalStateException("PRE_REPAIR_PREFIX_RESTORE_FAILED");
    }

    private static void cleanupTreeBestEffort(File root, File filesDir) {
        if (root == null || !root.exists()) return;
        try {
            String allowed = filesDir.getCanonicalPath() + File.separator;
            String actual = root.getCanonicalPath();
            if (!actual.startsWith(allowed)) throw new SecurityException("cleanup outside filesDir: " + actual);
            deleteNode(root, allowed);
        } catch (Throwable error) {
            Logger.logWarn(LOG_TAG, "Deferred cleanup for " + root + ": " + error.getMessage());
        }
    }

    private static void deleteNode(File file, String allowedPrefix) throws Exception {
        StructStat stat = Os.lstat(file.getAbsolutePath());
        boolean symlink = (stat.st_mode & OsConstants.S_IFMT) == OsConstants.S_IFLNK;
        if (symlink) {
            if (!file.delete() && file.exists()) throw new IllegalStateException("symlink delete failed: " + file);
            return;
        }

        String canonical = file.getCanonicalPath();
        if (!canonical.startsWith(allowedPrefix)) throw new SecurityException("delete outside filesDir: " + canonical);
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) for (File child : children) deleteNode(child, allowedPrefix);
        }
        if (!file.delete() && file.exists()) throw new IllegalStateException("delete failed: " + file);
    }
}
