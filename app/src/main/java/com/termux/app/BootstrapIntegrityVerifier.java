package com.termux.app;

import android.os.Build;

import androidx.annotation.NonNull;

import com.termux.rafacodephi.BuildConfig;
import com.termux.shared.logger.Logger;

import org.bouncycastle.crypto.digests.Blake3Digest;

import java.util.Locale;

final class BootstrapIntegrityVerifier {

    private static final String LOG_TAG = "BootstrapIntegrity";

    private BootstrapIntegrityVerifier() {}

    @NonNull
    static String expectedHashForCurrentAbi() {
        String[] supportedAbis = Build.SUPPORTED_ABIS;
        if (supportedAbis.length == 0) return "";
        String abi = supportedAbis[0];
        switch (abi) {
            case "arm64-v8a": return BuildConfig.BOOTSTRAP_BLAKE3_AARCH64;
            case "armeabi-v7a": return BuildConfig.BOOTSTRAP_BLAKE3_ARM;
            case "x86": return BuildConfig.BOOTSTRAP_BLAKE3_I686;
            case "x86_64": return BuildConfig.BOOTSTRAP_BLAKE3_X86_64;
            default:
                Logger.logError(LOG_TAG, "Unsupported ABI for bootstrap hash verification: " + abi);
                return "";
        }
    }

    @NonNull
    static String blake3Hex(@NonNull byte[] bytes) {
        Blake3Digest digest = new Blake3Digest(256);
        digest.update(bytes, 0, bytes.length);
        byte[] out = new byte[32];
        digest.doFinal(out, 0);
        StringBuilder sb = new StringBuilder(out.length * 2);
        for (byte b : out) {
            sb.append(String.format(Locale.US, "%02x", b));
        }
        return sb.toString();
    }
}
