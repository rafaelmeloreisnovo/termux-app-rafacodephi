package com.termux.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.os.Build;
import android.os.Environment;
import android.system.Os;
import android.system.OsConstants;
import android.system.StructStat;
import android.util.Pair;
import android.view.WindowManager;

import com.termux.rafacodephi.BuildConfig;
import com.termux.rafacodephi.R;
import com.termux.shared.errors.Error;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxRuntimePaths;
import com.termux.shared.termux.TermuxUtils;
import com.termux.shared.termux.crash.TermuxCrashUtils;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;
import com.termux.shared.android.PackageUtils;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Installs the Termux bootstrap into the app-private directory assigned by
 * Android. The compile-time TermuxConstants prefix remains a separate binary
 * compatibility contract; a relocated runtime is accepted only for an
 * explicitly bridge/relocatable bootstrap profile.
 */
public final class TermuxInstaller {

    private static final String LOG_TAG = "TermuxInstaller";
    private static final int COPY_BUFFER = 16 * 1024;

    private TermuxInstaller() {}

    private static void logPhase(String phase, String detail) {
        Logger.logInfo(LOG_TAG, "phase=" + phase + " " + detail);
    }

    /** Performs bootstrap setup if necessary. */
    public static void setupBootstrapIfNeeded(final Activity activity, final Runnable whenDone) {
        TermuxRuntimePaths.init(activity);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N &&
            !PackageUtils.isCurrentUserThePrimaryUser(activity)) {
            showFatalPreflight(activity, "PRIMARY_USER_REQUIRED",
                "The Termux runtime must run as the primary Android user for this build.");
            return;
        }

        try {
            verifyRuntimeFilesDirectoryWritable(activity);
        } catch (Throwable t) {
            showFatalPreflight(activity, "RUNTIME_FILES_DIR_NOT_WRITABLE",
                "Android assigned filesDir=" + TermuxRuntimePaths.filesDirPath() + "\n" + t);
            return;
        }

        final File prefix = TermuxRuntimePaths.prefixDir();
        if (runtimePrefixReady(prefix)) {
            Logger.logInfo(LOG_TAG, "runtime prefix already ready path=" + prefix
                + " layout=" + TermuxRuntimePaths.layoutState());
            whenDone.run();
            return;
        }

        final ProgressDialog progress = ProgressDialog.show(activity, null,
            activity.getString(R.string.bootstrap_installer_body), true, false);

        new Thread(() -> {
            try {
                installRuntimeBootstrap(activity);
                activity.runOnUiThread(whenDone);
            } catch (final Throwable t) {
                rollbackFailedBootstrapInstall();
                if (BuildConfig.BOOTSTRAP_BAREMETAL_STRICT) {
                    Logger.logStackTraceWithMessage(LOG_TAG,
                        "Bootstrap installation failed in strict mode", t);
                }
                showBootstrapErrorDialog(activity, whenDone,
                    Logger.getStackTracesMarkdownString(null, Logger.getStackTracesStringArray(t)));
            } finally {
                activity.runOnUiThread(() -> {
                    try {
                        progress.dismiss();
                    } catch (RuntimeException ignored) {
                    }
                });
            }
        }, "rafcodephi-bootstrap-install").start();
    }

    private static void installRuntimeBootstrap(Activity activity) throws Exception {
        TermuxRuntimePaths.init(activity);
        File filesDir = TermuxRuntimePaths.filesDir();
        File staging = TermuxRuntimePaths.stagingPrefixDir();
        File prefix = TermuxRuntimePaths.prefixDir();

        logPhase("runtime-path", "filesDir=" + filesDir
            + " prefix=" + prefix
            + " canonicalPrefix=" + TermuxConstants.TERMUX_PREFIX_DIR_PATH
            + " layout=" + TermuxRuntimePaths.layoutState());

        deleteTreeInsideRuntime(staging);
        if (prefix.exists() && !runtimePrefixReady(prefix)) deleteTreeInsideRuntime(prefix);
        ensureDirectory(staging, 0700);

        logPhase("zip-load", "loading accepted wizard bootstrap or embedded bootstrap");
        byte[] zipBytes = loadZipBytes(activity);
        verifyBootstrapZipIntegrity(zipBytes);
        verifyRelocationContract(zipBytes);

        final List<Pair<String, String>> symlinks = new ArrayList<>(64);
        final String canonicalStaging = staging.getCanonicalPath() + "/";
        long files = 0L;
        long dirs = 0L;
        long bytes = 0L;

        try (ZipInputStream input = new ZipInputStream(new ByteArrayInputStream(zipBytes))) {
            ZipEntry entry;
            byte[] buffer = new byte[COPY_BUFFER];
            while ((entry = input.getNextEntry()) != null) {
                String name = entry.getName();
                validateZipEntryName(name);

                if ("SYMLINKS.txt".equals(name)) {
                    BufferedReader reader = new BufferedReader(new InputStreamReader(input, StandardCharsets.UTF_8));
                    String line;
                    while ((line = reader.readLine()) != null) {
                        String[] parts = line.split("←", -1);
                        if (parts.length != 2 || parts[0].isEmpty() || parts[1].isEmpty())
                            throw new IllegalArgumentException("Malformed SYMLINKS.txt line: " + line);
                        String target = parts[0];
                        String relativeLink = parts[1];
                        if (relativeLink.startsWith("/") || relativeLink.contains(".."))
                            throw new SecurityException("Unsafe symlink destination: " + line);
                        File link = new File(staging, relativeLink);
                        validatePathInStaging(canonicalStaging, link, "symlink");
                        ensureDirectory(link.getParentFile(), 0700);
                        symlinks.add(Pair.create(target, link.getAbsolutePath()));
                    }
                    continue;
                }

                File target = new File(staging, name);
                validatePathInStaging(canonicalStaging, target, "zip entry");
                if (entry.isDirectory()) {
                    ensureDirectory(target, 0700);
                    dirs++;
                    continue;
                }

                ensureDirectory(target.getParentFile(), 0700);
                try (FileOutputStream output = new FileOutputStream(target)) {
                    int read;
                    while ((read = input.read(buffer)) != -1) {
                        output.write(buffer, 0, read);
                        bytes += read;
                    }
                    output.flush();
                    output.getFD().sync();
                }
                boolean executable = name.startsWith("bin/") || name.startsWith("libexec/") ||
                    name.startsWith("lib/apt/apt-helper") || name.startsWith("lib/apt/methods/");
                Os.chmod(target.getAbsolutePath(), executable ? 0700 : 0600);
                files++;
            }
        }

        if (symlinks.isEmpty()) throw new IllegalStateException("No SYMLINKS.txt encountered");
        for (Pair<String, String> link : symlinks) {
            Os.symlink(link.first, link.second);
        }

        verifyRuntimeBinary(new File(staging, "bin/sh"), "sh", true);
        verifyRuntimeBinary(new File(staging, "bin/pkg"), "pkg", true);
        verifyRuntimeBinary(new File(staging, "bin/busybox"), "busybox", true);
        verifyRuntimeBinary(new File(staging, "bin/proot"), "proot", true);

        if (prefix.exists()) deleteTreeInsideRuntime(prefix);
        if (!staging.renameTo(prefix)) {
            throw new IllegalStateException("STAGING_TO_PREFIX_RENAME_FAILED staging=" + staging + " prefix=" + prefix);
        }

        if (!runtimePrefixReady(prefix)) {
            throw new IllegalStateException("POST_INSTALL_PREFIX_READINESS_FAILED: " + prefix);
        }

        ensureDirectory(TermuxRuntimePaths.homeDir(), 0700);
        ensureDirectory(TermuxRuntimePaths.storageHomeDir(), 0700);
        ensureDirectory(new File(TermuxRuntimePaths.envDirPath()), 0700);

        BootstrapBaremetalGuard.selftest();
        BootstrapBaremetalGuard.validateAfterBootstrap(prefix.getAbsolutePath());
        TermuxShellEnvironment.writeEnvironmentToFile(activity);

        Logger.logInfo(LOG_TAG, "Bootstrap installed: files=" + files + " dirs=" + dirs
            + " bytes=" + bytes + " prefix=" + prefix
            + " source=" + BootstrapWizardSource.status(activity)
            + " layout=" + TermuxRuntimePaths.layoutState());
    }

    private static void verifyRuntimeFilesDirectoryWritable(Context context) throws Exception {
        File files = context.getFilesDir();
        if (files == null) throw new IllegalStateException("Context.getFilesDir() returned null");
        if (!files.exists() && !files.mkdirs() && !files.isDirectory())
            throw new IllegalStateException("Cannot create Android-assigned files directory");
        if (!files.isDirectory()) throw new IllegalStateException("Android-assigned files path is not a directory");

        File probe = new File(files, ".rafcodephi-write-probe.tmp");
        try (FileOutputStream output = new FileOutputStream(probe)) {
            output.write(0x52);
            output.flush();
            output.getFD().sync();
        }
        if (!probe.isFile() || probe.length() != 1L)
            throw new IllegalStateException("Private filesystem write probe was not durable");
        if (!probe.delete()) Logger.logWarn(LOG_TAG, "Could not delete write probe " + probe);
    }

    private static boolean runtimePrefixReady(File prefix) {
        File sh = new File(prefix, "bin/sh");
        File pkg = new File(prefix, "bin/pkg");
        File busybox = new File(prefix, "bin/busybox");
        File proot = new File(prefix, "bin/proot");
        return prefix.isDirectory() && sh.isFile() && sh.canExecute()
            && pkg.isFile() && pkg.canExecute()
            && busybox.isFile() && busybox.canExecute()
            && proot.isFile() && proot.canExecute();
    }

    /**
     * Public embedded-bootstrap API retained for existing build/tests. This does
     * not consult the wizard document source because it has no Context.
     */
    public static byte[] loadZipBytes() {
        System.loadLibrary("termux-bootstrap");
        return getZip();
    }

    /** Prefer a fully accepted wizard bootstrap.zip, otherwise use embedded bytes. */
    private static byte[] loadZipBytes(Context context) throws Exception {
        byte[] selected = BootstrapWizardSource.loadAcceptedBytes(context);
        if (selected != null) {
            Logger.logInfo(LOG_TAG, "Using wizard-selected canonical bootstrap.zip");
            return selected;
        }
        Logger.logInfo(LOG_TAG, "No accepted wizard bootstrap selected; using embedded bootstrap");
        return loadZipBytes();
    }

    public static native byte[] getZip();

    private static void verifyBootstrapZipIntegrity(byte[] zipBytes) {
        String expected = BootstrapIntegrityVerifier.expectedHashForCurrentAbi();
        if (expected == null || expected.isEmpty())
            throw new IllegalStateException("BOOTSTRAP_BLAKE3_EXPECTATION_MISSING");
        String actual = BootstrapIntegrityVerifier.blake3Hex(zipBytes);
        if (!actual.equalsIgnoreCase(expected)) {
            throw new SecurityException("BOOTSTRAP_BLAKE3_MISMATCH expected="
                + expected.toLowerCase(Locale.US) + " actual=" + actual.toLowerCase(Locale.US));
        }
        Logger.logInfo(LOG_TAG, "Bootstrap BLAKE3 verified: " + actual);
    }

    /**
     * Adopted-storage runtime can only consume an explicitly bridge profile.
     * Real apt/dpkg ELFs may encode the canonical prefix and are therefore
     * blocked until rebuilt/validated for the runtime layout.
     */
    private static void verifyRelocationContract(byte[] zipBytes) throws Exception {
        if (!TermuxRuntimePaths.isRelocatedLayout()) return;

        JSONObject profile = null;
        try (ZipInputStream input = new ZipInputStream(new ByteArrayInputStream(zipBytes))) {
            ZipEntry entry;
            while ((entry = input.getNextEntry()) != null) {
                if (!"BOOTSTRAP_PROFILE.json".equals(entry.getName())) continue;
                ByteArrayOutputStream output = new ByteArrayOutputStream();
                byte[] buffer = new byte[4096];
                int total = 0;
                int read;
                while ((read = input.read(buffer)) != -1) {
                    total += read;
                    if (total > 64 * 1024) throw new IllegalArgumentException("BOOTSTRAP_PROFILE_TOO_LARGE");
                    output.write(buffer, 0, read);
                }
                profile = new JSONObject(new String(output.toByteArray(), StandardCharsets.UTF_8));
                break;
            }
        }

        if (profile == null) throw new IllegalStateException("RELOCATED_RUNTIME_REQUIRES_BOOTSTRAP_PROFILE");
        boolean bridge = "bridge".equalsIgnoreCase(profile.optString("profile"))
            && "bridge".equalsIgnoreCase(profile.optString("package_layer"))
            && !profile.optBoolean("claim_allowed", true);
        if (!bridge) {
            throw new IllegalStateException("RELOCATED_RUNTIME_BLOCKED_FOR_REAL_OR_UNPROVEN_PACKAGE_LAYER profile="
                + profile.optString("profile", "UNKNOWN") + " package_layer="
                + profile.optString("package_layer", "UNKNOWN"));
        }
        Logger.logWarn(LOG_TAG, "Relocated Android-assigned app-private path accepted for bridge bootstrap only; real-pkg relocation claim remains false");
    }

    private static void verifyRuntimeBinary(File file, String name, boolean required) throws Exception {
        if (!file.isFile()) {
            if (required) throw new IllegalStateException("Missing runtime binary " + name + " at " + file);
            return;
        }
        StructStat stat = Os.stat(file.getAbsolutePath());
        if ((stat.st_mode & 0100) == 0)
            throw new IllegalStateException("Runtime binary not owner-executable: " + name + " mode=0" + Integer.toOctalString(stat.st_mode));
    }

    private static void validateZipEntryName(String name) {
        if (name == null || name.isEmpty() || name.startsWith("/") || name.contains("../") || name.equals(".."))
            throw new SecurityException("Unsafe bootstrap zip entry: " + name);
    }

    private static void validatePathInStaging(String canonicalStaging, File file, String label) throws Exception {
        String canonical = file.getCanonicalPath();
        if (!canonical.startsWith(canonicalStaging))
            throw new SecurityException("Unsafe " + label + " outside runtime staging: " + canonical);
    }

    private static void ensureDirectory(File directory, int mode) throws Exception {
        if (directory == null) throw new IllegalStateException("Directory parent is null");
        if (!directory.exists() && !directory.mkdirs() && !directory.isDirectory())
            throw new IllegalStateException("Could not create directory: " + directory);
        if (!directory.isDirectory()) throw new IllegalStateException("Not a directory: " + directory);
        Os.chmod(directory.getAbsolutePath(), mode);
    }

    private static void deleteTreeInsideRuntime(File file) throws Exception {
        if (file == null || !file.exists()) return;
        String runtimeRoot = TermuxRuntimePaths.filesDir().getCanonicalPath();
        String absolute = file.getCanonicalPath();
        if (!(absolute.equals(runtimeRoot) || absolute.startsWith(runtimeRoot + "/")))
            throw new SecurityException("Refusing delete outside Android-assigned filesDir: " + absolute);
        if (absolute.equals(runtimeRoot))
            throw new SecurityException("Refusing delete of runtime filesDir root");
        deleteNode(file, runtimeRoot);
    }

    private static void deleteNode(File file, String runtimeRoot) throws Exception {
        StructStat stat = Os.lstat(file.getAbsolutePath());
        boolean symlink = (stat.st_mode & OsConstants.S_IFMT) == OsConstants.S_IFLNK;
        if (!symlink && file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) {
                for (File child : children) {
                    String canonical = child.getCanonicalPath();
                    if (!canonical.startsWith(runtimeRoot + "/")) {
                        // A symlink can resolve outside the root. Delete the link itself instead of following it.
                        StructStat childStat = Os.lstat(child.getAbsolutePath());
                        boolean childSymlink = (childStat.st_mode & OsConstants.S_IFMT) == OsConstants.S_IFLNK;
                        if (!childSymlink) throw new SecurityException("Delete traversal outside runtime root: " + canonical);
                    }
                    deleteNode(child, runtimeRoot);
                }
            }
        }
        if (!file.delete() && file.exists()) throw new IllegalStateException("Failed to delete " + file);
    }

    private static void rollbackFailedBootstrapInstall() {
        try {
            deleteTreeInsideRuntime(TermuxRuntimePaths.stagingPrefixDir());
            File prefix = TermuxRuntimePaths.prefixDir();
            if (!runtimePrefixReady(prefix)) deleteTreeInsideRuntime(prefix);
        } catch (Throwable t) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Bootstrap rollback failed", t);
        }
    }

    public static void showBootstrapErrorDialog(Activity activity, Runnable whenDone, String message) {
        Logger.logErrorExtended(LOG_TAG, "Bootstrap Error:\n" + message);
        sendBootstrapCrashReportNotification(activity, message);
        activity.runOnUiThread(() -> {
            try {
                new AlertDialog.Builder(activity)
                    .setTitle(R.string.bootstrap_error_title)
                    .setMessage(R.string.bootstrap_error_body)
                    .setNegativeButton(R.string.bootstrap_error_abort, (dialog, which) -> {
                        dialog.dismiss();
                        activity.finish();
                    })
                    .setPositiveButton(R.string.bootstrap_error_try_again, (dialog, which) -> {
                        dialog.dismiss();
                        setupBootstrapIfNeeded(activity, whenDone);
                    })
                    .show();
            } catch (WindowManager.BadTokenException ignored) {
            }
        });
    }

    private static void showFatalPreflight(Activity activity, String state, String detail) {
        String message = state + "\n" + detail + "\n"
            + "runtimeFilesDir=" + TermuxRuntimePaths.filesDirPath() + "\n"
            + "runtimePrefix=" + TermuxRuntimePaths.prefixDirPath() + "\n"
            + "canonicalPrefix=" + TermuxConstants.TERMUX_PREFIX_DIR_PATH + "\n"
            + "layout=" + TermuxRuntimePaths.layoutState();
        Logger.logError(LOG_TAG, message);
        sendBootstrapCrashReportNotification(activity, message);
        new AlertDialog.Builder(activity)
            .setTitle(R.string.bootstrap_error_title)
            .setMessage(message)
            .setPositiveButton(android.R.string.ok, null)
            .show();
    }

    private static void sendBootstrapCrashReportNotification(Activity activity, String message) {
        String title = TermuxConstants.TERMUX_APP_NAME + " Bootstrap Error";
        TermuxCrashUtils.sendCrashReportNotification(activity, LOG_TAG, title, null,
            "## " + title + "\n\n" + message + "\n\n" + TermuxUtils.getTermuxDebugMarkdownString(activity),
            true, false, TermuxUtils.AppInfoMode.TERMUX_AND_PLUGIN_PACKAGES, true);
    }

    /** Setup ~/storage links under the runtime-resolved HOME. */
    static void setupStorageSymlinks(final Context context) {
        TermuxRuntimePaths.init(context);
        final String tag = "termux-storage";
        new Thread(() -> {
            try {
                File storage = TermuxRuntimePaths.storageHomeDir();
                clearDirectoryOnly(storage);

                createSymlinkSafely(tag, Environment.getExternalStorageDirectory(), storage, "shared");
                createSymlinkSafely(tag, Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS), storage, "documents");
                createSymlinkSafely(tag, Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), storage, "downloads");
                createSymlinkSafely(tag, Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM), storage, "dcim");
                createSymlinkSafely(tag, Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES), storage, "pictures");
                createSymlinkSafely(tag, Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MUSIC), storage, "music");
                createSymlinkSafely(tag, Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES), storage, "movies");
                createSymlinkSafely(tag, Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PODCASTS), storage, "podcasts");
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
                    createSymlinkSafely(tag, Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_AUDIOBOOKS), storage, "audiobooks");

                File[] external = context.getExternalFilesDirs(null);
                if (external != null) {
                    for (int i = 0; i < external.length; i++)
                        if (external[i] != null) createSymlinkSafely(tag, external[i], storage, "external-" + i);
                }
                File[] media = context.getExternalMediaDirs();
                if (media != null) {
                    for (int i = 0; i < media.length; i++)
                        if (media[i] != null) createSymlinkSafely(tag, media[i], storage, "media-" + i);
                }
                Logger.logInfo(tag, "Runtime storage links ready at " + storage);
            } catch (Throwable t) {
                Logger.logStackTraceWithMessage(tag, "Setup Storage Error", t);
            }
        }, "rafcodephi-storage-links").start();
    }

    private static void clearDirectoryOnly(File directory) throws Exception {
        if (!directory.exists()) ensureDirectory(directory, 0700);
        File[] children = directory.listFiles();
        if (children != null) for (File child : children) deleteNode(child, TermuxRuntimePaths.filesDir().getCanonicalPath());
        ensureDirectory(directory, 0700);
    }

    private static void createSymlinkSafely(String tag, File source, File storage, String name) {
        try {
            if (source == null) return;
            File link = new File(storage, name);
            if (link.exists()) link.delete();
            Os.symlink(source.getAbsolutePath(), link.getAbsolutePath());
        } catch (Throwable t) {
            Logger.logWarn(tag, "Symlink " + name + " unavailable: " + t.getMessage());
        }
    }
}
