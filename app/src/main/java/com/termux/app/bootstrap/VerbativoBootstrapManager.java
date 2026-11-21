/**
 * VerbativoBootstrapManager.java — Convergence Receipt Generation & Bootstrap Integration
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Generates convergence receipts during app bootstrap initialization.
 * Ensures φ_fst validation before completing bootstrap.
 */
package com.termux.app.bootstrap;

import android.content.Context;
import android.util.Log;

import com.termux.rafaelia.VerbativoCore;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.Locale;

public class VerbativoBootstrapManager {
    private static final String TAG = "VerbativoBootstrap";
    private static final String RECEIPT_DIR = "verbovivo";
    private static final String RECEIPT_FILE = "convergence_receipt.bin";
    private static final int RECEIPT_BYTES = 26;

    private final Context context;
    private byte[] convergenceReceipt;
    private boolean initialized = false;

    public VerbativoBootstrapManager(Context context) {
        this.context = context;
    }

    /**
     * Execute bootstrap convergence validation.
     * This is called during app startup before loading main activities.
     *
     * @return true if bootstrap passed validation, false if failed (fail-closed)
     */
    public boolean executeBootstrap() {
        // A retry must never inherit the previous invocation's validated state.
        initialized = false;
        convergenceReceipt = null;
        Log.i(TAG, "Starting Verbovivo bootstrap validation");

        try {
            // Step 1: Verify Verbovivo is initialized
            if (!VerbativoCore.isInitialized()) {
                Log.e(TAG, "Verbovivo core not initialized");
                return false;
            }
            Log.d(TAG, "✓ Verbovivo core initialized");

            // Step 2: Generate convergence receipt
            byte[] candidateReceipt = VerbativoCore.generateConvergenceReceipt();
            if (candidateReceipt == null || candidateReceipt.length != RECEIPT_BYTES) {
                Log.e(TAG, "Failed to generate convergence receipt");
                return false;
            }
            Log.d(TAG, "✓ Convergence receipt generated (26 bytes)");

            // Step 3: Validate receipt structure
            if (VerbativoCore.validateConvergenceReceipt(candidateReceipt) != 0) {
                Log.e(TAG, "Convergence receipt validation failed");
                return false;
            }
            Log.d(TAG, "✓ Receipt validation passed");

            // Step 4: Extract and log metrics
            long h_norm = decodeUint64BE(candidateReceipt, 0);
            long c_norm = decodeUint64BE(candidateReceipt, 8);
            long phi_fst = decodeUint64BE(candidateReceipt, 16);
            int attractor_id = candidateReceipt[24] & 0xFF;
            int status = candidateReceipt[25] & 0xFF;

            double h_percent = (h_norm * 100.0) / 0x10000;
            double c_percent = (c_norm * 100.0) / 0x10000;
            double phi_percent = (phi_fst * 100.0) / 0x10000;

            Log.i(TAG, String.format(Locale.US,
                "✓ Bootstrap φ validation: H=%.2f%% C=%.2f%% φ=%.2f%% attractor=%d status=%d",
                h_percent, c_percent, phi_percent, attractor_id, status));

            // Step 5: Store receipt for audit/attestation
            if (!storeReceipt(candidateReceipt)) {
                Log.e(TAG, "Convergence receipt persistence/readback failed");
                return false;
            }
            Log.d(TAG, "✓ Receipt stored and read back");

            convergenceReceipt = candidateReceipt.clone();
            initialized = true;
            Log.i(TAG, "Verbovivo local diagnostic receipt validated and persisted; device E2E remains unverified");
            return true;

        } catch (Exception e) {
            Log.e(TAG, "Bootstrap validation exception", e);
            return false;
        } catch (LinkageError e) {
            // The convergence engine is an optional audit surface. Missing or ABI-incompatible
            // native symbols must be reported without blocking the terminal session startup.
            Log.e(TAG, "Bootstrap validation native linkage unavailable", e);
            return false;
        }
    }

    /**
     * Get the generated convergence receipt.
     * Only valid if executeBootstrap() returned true.
     *
     * @return convergence receipt (26 bytes) or null if not available
     */
    public byte[] getConvergenceReceipt() {
        return initialized && convergenceReceipt != null ? convergenceReceipt.clone() : null;
    }

    /**
     * Check if bootstrap completed successfully
     */
    public boolean isBootstrapComplete() {
        return initialized;
    }

    /**
     * Store a local diagnostic receipt for audit. This is not remote/device attestation.
     * Receipt is stored in app's private data directory.
     */
    private boolean storeReceipt(byte[] receipt) {
        if (receipt == null || receipt.length != RECEIPT_BYTES) return false;
        File temp = null;
        try {
            File receiptDir = new File(context.getFilesDir(), RECEIPT_DIR);
            if (!receiptDir.isDirectory() && !receiptDir.mkdirs()) {
                Log.w(TAG, "Failed to create receipt directory");
                return false;
            }

            // Unique temporary file in the same directory: no partial final receipt.
            temp = File.createTempFile("convergence_receipt.", ".tmp", receiptDir);
            try (FileOutputStream stream = new FileOutputStream(temp)) {
                stream.write(receipt);
                stream.getFD().sync();
            }

            File destination = new File(receiptDir, RECEIPT_FILE);
            if (!temp.renameTo(destination)) {
                Log.w(TAG, "Could not atomically replace receipt");
                return false;
            }

            // Readback is required before the audit layer reports success.
            byte[] persisted = retrieveStoredReceipt();
            return Arrays.equals(receipt, persisted);
        } catch (IOException | SecurityException e) {
            Log.w(TAG, "Failed to store/readback receipt", e);
            return false;
        } finally {
            if (temp != null && temp.exists() && !temp.delete()) {
                Log.w(TAG, "Could not clean temporary receipt");
            }
        }
    }

    /**
     * Read a bounded binary receipt; reject short, oversized or truncated files.
     * This is storage-integrity checking, not independent cryptographic attestation.
     */
    public byte[] retrieveStoredReceipt() {
        File receiptFile = new File(context.getFilesDir(), RECEIPT_DIR + "/" + RECEIPT_FILE);
        try {
            if (!receiptFile.isFile() || receiptFile.length() != RECEIPT_BYTES) {
                return null;
            }
            byte[] data = new byte[RECEIPT_BYTES];
            try (FileInputStream stream = new FileInputStream(receiptFile)) {
                int offset = 0;
                while (offset < data.length) {
                    int read = stream.read(data, offset, data.length - offset);
                    if (read <= 0) return null;
                    offset += read;
                }
                if (stream.read() != -1) return null;
            }
            return data;
        } catch (IOException | SecurityException e) {
            Log.w(TAG, "Failed to retrieve receipt", e);
            return null;
        }
    }

    /**
     * Get receipt as hex string for logging/display
     */
    public String getReceiptHex() {
        if (convergenceReceipt == null) {
            return "null";
        }
        return VerbativoCore.receiptToHex(convergenceReceipt);
    }

    /**
     * Get receipt hash for fingerprinting
     */
    public String getReceiptHash() {
        if (convergenceReceipt == null) {
            return null;
        }

        try {
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            byte[] hash = md.digest(convergenceReceipt);
            StringBuilder sb = new StringBuilder();
            for (byte b : hash) {
                sb.append(String.format("%02x", b));
            }
            return sb.toString();
        } catch (Exception e) {
            Log.e(TAG, "Failed to compute receipt hash", e);
            return null;
        }
    }

    /**
     * Format metrics for debug logging
     */
    public String formatMetrics() {
        if (convergenceReceipt == null || convergenceReceipt.length < 26) {
            return "null";
        }

        try {
            long h_norm = decodeUint64BE(convergenceReceipt, 0);
            long c_norm = decodeUint64BE(convergenceReceipt, 8);
            long phi_fst = decodeUint64BE(convergenceReceipt, 16);
            int attractor = convergenceReceipt[24] & 0xFF;
            int status = convergenceReceipt[25] & 0xFF;

            return String.format(Locale.US,
                "{H=0x%x C=0x%x φ=0x%x attractor=%d status=%d}",
                h_norm, c_norm, phi_fst, attractor, status);

        } catch (Exception e) {
            return "error";
        }
    }

    /* ── Utility ────────────────────────────────────────────────── */

    private static long decodeUint64BE(byte[] buf, int offset) {
        long value = 0;
        for (int i = 0; i < 8; i++) {
            value |= ((long)(buf[offset + i] & 0xFF)) << (56 - i * 8);
        }
        return value;
    }
}
