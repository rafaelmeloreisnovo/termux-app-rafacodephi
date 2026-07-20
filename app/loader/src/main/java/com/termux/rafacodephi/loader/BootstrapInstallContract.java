package com.termux.rafacodephi.loader;

/**
 * Signature-protected acquisition contract between the host and loader APKs.
 *
 * The loader may download and verify a bootstrap ZIP, but it never receives a
 * target directory and never extracts into the host application's private
 * prefix. The host remains the only installer of its own $PREFIX.
 */
public final class BootstrapInstallContract {

    public static final String HOST_PACKAGE = "com.termux.rafacodephi";
    public static final String LOADER_PACKAGE = "com.termux.rafacodephi.loader";
    public static final String HANDOFF_PERMISSION =
            "com.termux.rafacodephi.permission.BOOTSTRAP_HANDOFF";
    public static final String PROVIDER_AUTHORITY =
            "com.termux.rafacodephi.loader.bootstrap";

    public static final String ACTION_INSTALL_BOOTSTRAP =
            "com.termux.rafacodephi.loader.INSTALL_BOOTSTRAP";
    public static final String ACTION_BOOTSTRAP_VERIFIED =
            "com.termux.rafacodephi.BOOTSTRAP_VERIFIED";

    public static final String EXTRA_ABI = "abi";
    public static final String EXTRA_SHA256 = "expected_sha256";
    public static final String EXTRA_SOURCE_URL = "source_url";
    public static final String EXTRA_SUCCESS = "success";
    public static final String EXTRA_FAILURE_REASON = "failure_reason";
    public static final String EXTRA_VERIFIED_ABI = "verified_abi";
    public static final String EXTRA_VERIFIED_BYTES = "verified_bytes";

    public static final String STATE_IDLE = "IDLE";
    public static final String STATE_DOWNLOADING = "DOWNLOADING";
    public static final String STATE_VERIFYING = "VERIFYING";
    public static final String STATE_PUBLISHING = "PUBLISHING";
    public static final String STATE_COMPLETE = "COMPLETE";
    public static final String STATE_ERROR = "ERROR";

    private BootstrapInstallContract() {}
}
