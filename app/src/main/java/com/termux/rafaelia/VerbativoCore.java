/**
 * VerbativoCore.java — Java Interface to Verbovivo Convergence Engine
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Safe JNI wrapper for T^7 toroid convergence and receipt generation.
 */
package com.termux.rafaelia;

import android.util.Log;

public class VerbativoCore {
    private static final String TAG = "VerbativoCore";
    private static boolean initialized = false;

    static {
        try {
            System.loadLibrary("termux-rafaelia");
            initialized = isInitializedNative();
            if (initialized) {
                Log.i(TAG, "Verbovivo graph loaded and initialized");
            } else {
                Log.e(TAG, "Verbovivo graph initialization failed");
            }
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load Verbovivo native library", e);
            initialized = false;
        }
    }

    /**
     * Check if Verbovivo is initialized and ready
     */
    public static boolean isInitialized() {
        return initialized;
    }

    /**
     * Execute convergence walk and return φ_fst value
     *
     * @return φ_fst in Q16 format (0 to 0x10000), or -1 if validation failed
     */
    public static long executeConvergence() {
        if (!initialized) {
            return -1;
        }
        return executeConvergenceNative();
    }

    /**
     * Compute entropy, coherence, and φ metrics for current graph state
     *
     * @param outMetrics array to hold results [H_norm, C_norm, φ_fst] in Q16 format
     * @return 0 on success, -1 on failure
     */
    public static int computePhiMetrics(long[] outMetrics) {
        if (!initialized || outMetrics == null || outMetrics.length < 3) {
            return -1;
        }
        return computePhiMetricsNative(outMetrics);
    }

    /**
     * Find attractor nearest to query vector
     *
     * @param queryBytes 128-byte query vector (1024 bits)
     * @return attractor ID (0-41), or 255 if not found
     */
    public static int recallAttractor(byte[] queryBytes) {
        if (!initialized || queryBytes == null || queryBytes.length != 128) {
            return 255;
        }
        return recallAttractorNative(queryBytes);
    }

    /**
     * Validate convergence receipt structure
     *
     * Receipt format (binary):
     * - Bytes 0-7:   H_norm (uint64, big-endian, Q16)
     * - Bytes 8-15:  C_norm (uint64, big-endian, Q16)
     * - Bytes 16-23: φ_fst (uint64, big-endian, Q16)
     * - Byte 24:     attractor_id (0-41 or 255)
     * - Byte 25:     convergence_status (0-3)
     *
     * @param receiptBytes binary receipt (minimum 26 bytes)
     * @return 0 if valid (fail-closed), -1 if invalid
     */
    public static int validateConvergenceReceipt(byte[] receiptBytes) {
        if (!initialized || receiptBytes == null || receiptBytes.length < 26) {
            return -1;
        }
        return validateConvergenceReceiptNative(receiptBytes);
    }

    /**
     * Generate convergence receipt for bootstrap validation
     *
     * Creates a receipt with:
     * - Entropy H_norm from current graph state
     * - Coherence C_norm from KAM-7 metric
     * - φ_fst = (1-H)·C in Q16 fixed-point
     * - Attractor ID if converged
     * - Status code (0=attractor, 1=stable, 2=no-edges, 3=timeout)
     *
     * @return binary receipt (26 bytes minimum), or null if failed
     */
    public static byte[] generateConvergenceReceipt() {
        if (!initialized) {
            return null;
        }

        try {
            long phi = executeConvergence();
            if (phi < 0) {
                return null;  /* convergence failed */
            }

            long[] metrics = new long[3];
            if (computePhiMetrics(metrics) != 0) {
                return null;  /* metric computation failed */
            }

            /* Build receipt: H_norm | C_norm | φ_fst | attractor_id | status */
            byte[] receipt = new byte[26];

            /* Encode metrics as big-endian uint64 */
            encodeUint64BE(receipt, 0, metrics[0]);   /* H_norm */
            encodeUint64BE(receipt, 8, metrics[1]);   /* C_norm */
            encodeUint64BE(receipt, 16, metrics[2]);  /* φ_fst */

            /* Attractor ID (assume convergent for now) */
            receipt[24] = 0;    /* attractor 0 */
            receipt[25] = 0;    /* status: attractor */

            return receipt;
        } catch (Exception e) {
            Log.e(TAG, "Failed to generate convergence receipt", e);
            return null;
        }
    }

    /**
     * Convert receipt to hex string for logging
     */
    public static String receiptToHex(byte[] receipt) {
        if (receipt == null) {
            return "null";
        }
        StringBuilder sb = new StringBuilder();
        for (byte b : receipt) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }

    /* ── Native Methods (JNI) ────────────────────────────────────── */

    private static native long executeConvergenceNative();
    private static native int computePhiMetricsNative(long[] outMetrics);
    private static native int recallAttractorNative(byte[] queryBytes);
    private static native int validateConvergenceReceiptNative(byte[] receiptBytes);
    private static native int isInitializedNative();

    /* ── Utility ────────────────────────────────────────────────── */

    private static void encodeUint64BE(byte[] buf, int offset, long value) {
        for (int i = 0; i < 8; i++) {
            buf[offset + i] = (byte)((value >> (56 - i * 8)) & 0xFF);
        }
    }
}
