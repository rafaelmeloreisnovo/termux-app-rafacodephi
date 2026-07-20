package com.termux.rafacodephi.loader;

/**
 * Defines the public contract between loader.apk and the host Termux app.
 *
 * The loader APK is a lightweight installer stub that:
 * 1. Receives an install request via {@link #ACTION_INSTALL_BOOTSTRAP}
 * 2. Validates the bootstrap source URL and expected SHA-256
 * 3. Downloads and extracts the bootstrap to the target files dir
 * 4. Broadcasts the result back via {@link #ACTION_INSTALL_RESULT}
 *
 * State machine:
 *   IDLE → DOWNLOADING → VERIFYING → EXTRACTING → COMPLETE
 *                ↓              ↓            ↓
 *              ERROR          ERROR        ERROR
 */
public final class BootstrapInstallContract {

    public static final String ACTION_INSTALL_BOOTSTRAP =
            "com.termux.rafacodephi.loader.INSTALL_BOOTSTRAP";

    public static final String ACTION_INSTALL_RESULT =
            "com.termux.rafacodephi.loader.INSTALL_RESULT";

    /** Intent extra: ABI string, e.g. "aarch64", "arm", "x86_64", "i386" */
    public static final String EXTRA_ABI = "abi";

    /** Intent extra: expected bootstrap SHA-256 hex string (64 chars) */
    public static final String EXTRA_SHA256 = "expected_sha256";

    /** Intent extra: canonical source URL for this bootstrap ZIP */
    public static final String EXTRA_SOURCE_URL = "source_url";

    /** Intent extra: absolute path to the target extraction directory */
    public static final String EXTRA_TARGET_DIR = "target_dir";

    /** Result extra: boolean — true if install succeeded */
    public static final String EXTRA_SUCCESS = "success";

    /** Result extra: failure reason string (empty on success) */
    public static final String EXTRA_FAILURE_REASON = "failure_reason";

    /** Result extra: the installed ABI */
    public static final String EXTRA_INSTALLED_ABI = "installed_abi";

    public static final String STATE_IDLE        = "IDLE";
    public static final String STATE_DOWNLOADING = "DOWNLOADING";
    public static final String STATE_VERIFYING   = "VERIFYING";
    public static final String STATE_EXTRACTING  = "EXTRACTING";
    public static final String STATE_COMPLETE    = "COMPLETE";
    public static final String STATE_ERROR       = "ERROR";

    private BootstrapInstallContract() {}
}
