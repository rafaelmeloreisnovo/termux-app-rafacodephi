package com.termux.rafacodephi.loader;

import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Locale;

/** Fail-closed SHA-256 verification for loader-private bootstrap files. */
final class BootstrapChecksumValidator {

    private static final String TAG = "BootstrapChecksumValidator";
    private static final int BUFFER_SIZE = 65_536;

    private BootstrapChecksumValidator() {}

    static boolean validate(File file, String expectedHex) {
        if (file == null || !file.isFile()) {
            Log.e(TAG, "validate: file not found: " + file);
            return false;
        }
        if (expectedHex == null || !expectedHex.matches("^[0-9a-fA-F]{64}$")) {
            Log.e(TAG, "validate: invalid expected SHA-256");
            return false;
        }
        String actual = sha256Hex(file);
        if (actual == null) return false;
        boolean match = actual.equals(expectedHex.toLowerCase(Locale.US));
        if (!match) Log.e(TAG, "validate: digest mismatch");
        return match;
    }

    private static String sha256Hex(File file) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] buffer = new byte[BUFFER_SIZE];
            try (FileInputStream input = new FileInputStream(file)) {
                int read;
                while ((read = input.read(buffer)) != -1) digest.update(buffer, 0, read);
            }
            StringBuilder result = new StringBuilder(64);
            for (byte value : digest.digest()) {
                result.append(String.format(Locale.US, "%02x", value & 0xff));
            }
            return result.toString();
        } catch (NoSuchAlgorithmException | IOException e) {
            Log.e(TAG, "SHA-256 verification failed", e);
            return null;
        }
    }
}
