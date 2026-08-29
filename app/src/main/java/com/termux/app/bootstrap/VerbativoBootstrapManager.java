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
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.MessageDigest;
import java.util.Locale;

public class VerbativoBootstrapManager {
    private static final String TAG = "VerbativoBootstrap";
    private static final String RECEIPT_DIR = "verbovivo";
    private static final String RECEIPT_FILE = "convergence_receipt.bin";

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
        Log.i(TAG, "Starting Verbovivo bootstrap validation");

        try {
            // Step 1: Verify Verbovivo is initialized
            if (!VerbativoCore.isInitialized()) {
                Log.e(TAG, "Verbovivo core not initialized");
                return false;
            }
            Log.d(TAG, "✓ Verbovivo core initialized");

            // Step 2: Generate convergence receipt
            convergenceReceipt = VerbativoCore.generateConvergenceReceipt();
            if (convergenceReceipt == null || convergenceReceipt.length < 26) {
                Log.e(TAG, "Failed to generate convergence receipt");
                return false;
            }
            Log.d(TAG, "✓ Convergence receipt generated (26 bytes)");

            // Step 3: Validate receipt structure
            if (VerbativoCore.validateConvergenceReceipt(convergenceReceipt) != 0) {
                Log.e(TAG, "Convergence receipt validation failed");
                return false;
            }
            Log.d(TAG, "✓ Receipt validation passed");

            // Step 4: Extract and log metrics
            long h_norm = decodeUint64BE(convergenceReceipt, 0);
            long c_norm = decodeUint64BE(convergenceReceipt, 8);
            long phi_fst = decodeUint64BE(convergenceReceipt, 16);
            int attractor_id = convergenceReceipt[24] & 0xFF;
            int status = convergenceReceipt[25] & 0xFF;

            double h_percent = (h_norm * 100.0) / 0x10000;
            double c_percent = (c_norm * 100.0) / 0x10000;
            double phi_percent = (phi_fst * 100.0) / 0x10000;

            Log.i(TAG, String.format(Locale.US,
                "✓ Bootstrap φ validation: H=%.2f%% C=%.2f%% φ=%.2f%% attractor=%d status=%d",
                h_percent, c_percent, phi_percent, attractor_id, status));

            // Step 5: Store receipt for audit/attestation
            storeReceipt();
            Log.d(TAG, "✓ Receipt stored for audit trail");

            initialized = true;
            Log.i(TAG, "✅ Verbovivo bootstrap validation PASSED");
            return true;

        } catch (Exception e) {
            Log.e(TAG, "Bootstrap validation exception", e);
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
        return convergenceReceipt;
    }

    /**
     * Check if bootstrap completed successfully
     */
    public boolean isBootstrapComplete() {
        return initialized;
    }

    /**
     * Store receipt for audit trail and device attestation.
     * Receipt is stored in app's private data directory.
     */
    private void storeReceipt() {
        try {
            File receiptDir = new File(context.getFilesDir(), RECEIPT_DIR);
            if (!receiptDir.exists() && !receiptDir.mkdirs()) {
                Log.w(TAG, "Failed to create receipt directory");
                return;
            }

            File receiptFile = new File(receiptDir, RECEIPT_FILE);
            java.io.FileOutputStream fos = new java.io.FileOutputStream(receiptFile);
            fos.write(convergenceReceipt);
            fos.close();

            Log.d(TAG, "Receipt stored: " + receiptFile.getAbsolutePath());

        } catch (Exception e) {
            Log.w(TAG, "Failed to store receipt", e);
        }
    }

    /**
     * Retrieve stored convergence receipt for inspection/audit.
     *
     * @return receipt bytes or null if not available
     */
    public byte[] retrieveStoredReceipt() {
        try {
            File receiptFile = new File(context.getFilesDir(), RECEIPT_DIR + "/" + RECEIPT_FILE);
            if (!receiptFile.exists()) {
                return null;
            }

            java.io.FileInputStream fis = new java.io.FileInputStream(receiptFile);
            byte[] data = new byte[(int)receiptFile.length()];
            fis.read(data);
            fis.close();

            return data;

        } catch (Exception e) {
            Log.e(TAG, "Failed to retrieve receipt", e);
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
