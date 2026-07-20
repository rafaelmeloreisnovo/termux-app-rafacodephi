package com.termux.rafacodephi.loader;

import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Locale;

/**
 * Verifies that a bootstrap ZIP matches its expected SHA-256 before extraction.
 * Fail-closed: any I/O or digest error returns false.
 */
final class BootstrapChecksumValidator {

    private static final String TAG = "BootstrapChecksumValidator";
    private static final int BUFFER_SIZE = 65536;

    private BootstrapChecksumValidator() {}

    /**
     * @param file         bootstrap ZIP on disk
     * @param expectedHex  64-char lowercase SHA-256 hex
     * @return true only if the file exists and its digest matches expectedHex
     */
    static boolean validate(File file, String expectedHex) {
        if (file == null || !file.isFile()) {
            Log.e(TAG, "validate: file not found: " + file);
            return false;
        }
        if (expectedHex == null || expectedHex.length() != 64) {
            Log.e(TAG, "validate: invalid expectedHex length");
            return false;
        }
        String actual = sha256Hex(file);
        if (actual == null) return false;
        boolean match = actual.equals(expectedHex.toLowerCase(Locale.ROOT));
        if (!match) {
            Log.e(TAG, "validate: digest mismatch — expected=" + expectedHex + " actual=" + actual);
        }
        return match;
    }

    private static String sha256Hex(File file) {
        MessageDigest digest;
        try {
            digest = MessageDigest.getInstance("SHA-256");
        } catch (NoSuchAlgorithmException e) {
            Log.e(TAG, "SHA-256 not available", e);
            return null;
        }
        byte[] buf = new byte[BUFFER_SIZE];
        try (FileInputStream in = new FileInputStream(file)) {
            int read;
            while ((read = in.read(buf)) > 0) {
                digest.update(buf, 0, read);
            }
        } catch (IOException e) {
            Log.e(TAG, "I/O error while hashing " + file, e);
            return null;
        }
        byte[] raw = digest.digest();
        StringBuilder sb = new StringBuilder(raw.length * 2);
        for (byte b : raw) sb.append(String.format(Locale.ROOT, "%02x", b & 0xff));
        return sb.toString();
    }
}
