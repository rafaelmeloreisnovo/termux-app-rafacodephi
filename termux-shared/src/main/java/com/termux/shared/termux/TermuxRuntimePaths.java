package com.termux.shared.termux;

import android.content.Context;

import androidx.annotation.NonNull;

import java.io.File;

/**
 * Runtime-resolved app-private Termux paths.
 *
 * TermuxConstants remains the compile-time/canonical prefix contract used by
 * binaries built for com.termux.rafacodephi. Android may, however, assign the
 * package private data directory to adopted storage (for example /mnt/expand).
 * This class records that Android-assigned location without pretending that a
 * binary compiled with the canonical prefix is relocatable.
 */
public final class TermuxRuntimePaths {

    private static volatile String filesDirPath = TermuxConstants.TERMUX_FILES_DIR_PATH;
    private static volatile boolean initialized;

    private TermuxRuntimePaths() {}

    public static synchronized void init(@NonNull Context context) {
        File assigned = context.getFilesDir();
        if (assigned != null) filesDirPath = assigned.getAbsolutePath();
        initialized = true;
    }

    public static boolean isInitialized() {
        return initialized;
    }

    @NonNull
    public static String filesDirPath() {
        return filesDirPath;
    }

    @NonNull
    public static File filesDir() {
        return new File(filesDirPath());
    }

    @NonNull
    public static String prefixDirPath() {
        return filesDirPath() + "/usr";
    }

    @NonNull
    public static File prefixDir() {
        return new File(prefixDirPath());
    }

    @NonNull
    public static String stagingPrefixDirPath() {
        return filesDirPath() + "/usr-staging";
    }

    @NonNull
    public static File stagingPrefixDir() {
        return new File(stagingPrefixDirPath());
    }

    @NonNull
    public static String homeDirPath() {
        return filesDirPath() + "/home";
    }

    @NonNull
    public static File homeDir() {
        return new File(homeDirPath());
    }

    @NonNull
    public static String binDirPath() {
        return prefixDirPath() + "/bin";
    }

    @NonNull
    public static String libDirPath() {
        return prefixDirPath() + "/lib";
    }

    @NonNull
    public static String tmpDirPath() {
        return prefixDirPath() + "/tmp";
    }

    @NonNull
    public static String storageHomeDirPath() {
        return homeDirPath() + "/storage";
    }

    @NonNull
    public static File storageHomeDir() {
        return new File(storageHomeDirPath());
    }

    @NonNull
    public static String envDirPath() {
        return prefixDirPath() + "/etc/termux";
    }

    @NonNull
    public static String envFilePath() {
        return envDirPath() + "/termux.env";
    }

    @NonNull
    public static String envTempFilePath() {
        return envDirPath() + "/termux.env.tmp";
    }

    /** Normalize only Android's primary-user /data/user/0 alias. */
    @NonNull
    public static String normalizePrimaryUserAlias(@NonNull String path) {
        if (path.startsWith("/data/user/0/")) {
            return "/data/data/" + path.substring("/data/user/0/".length());
        }
        return path;
    }

    public static boolean isCanonicalLayout() {
        return TermuxConstants.TERMUX_FILES_DIR_PATH.equals(
            normalizePrimaryUserAlias(filesDirPath()));
    }

    public static boolean isRelocatedLayout() {
        return !isCanonicalLayout();
    }

    @NonNull
    public static String layoutState() {
        return isCanonicalLayout() ? "CANONICAL" : "RELOCATED_ANDROID_ASSIGNED";
    }

    /**
     * A relocated Java/runtime path is not evidence that canonical-prefix ELFs
     * are relocatable. Real package binaries must carry an explicit compatible
     * build contract before they may be promoted.
     */
    public static boolean realPkgRelocationClaimAllowed() {
        return false;
    }
}
