package com.termux.rafacodephi.loader;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

/**
 * Entry-point Activity for the loader APK.
 *
 * Can be launched two ways:
 *   1. ACTION_INSTALL_BOOTSTRAP intent → triggers bootstrap install service
 *   2. Direct launch (e.g. from launcher) → finishes immediately with
 *      RESULT_CANCELED, since the loader has no user-facing UI of its own.
 *
 * In both cases, results are broadcast via ACTION_INSTALL_RESULT.
 */
public class LoaderActivity extends Activity {

    private static final String TAG = "LoaderActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        String action = intent != null ? intent.getAction() : null;

        if (BootstrapInstallContract.ACTION_INSTALL_BOOTSTRAP.equals(action)) {
            handleInstallRequest(intent);
        } else {
            Log.i(TAG, "No install action; finishing without work");
            setResult(RESULT_CANCELED);
        }

        finish();
    }

    private void handleInstallRequest(Intent intent) {
        String abi       = intent.getStringExtra(BootstrapInstallContract.EXTRA_ABI);
        String sha256    = intent.getStringExtra(BootstrapInstallContract.EXTRA_SHA256);
        String sourceUrl = intent.getStringExtra(BootstrapInstallContract.EXTRA_SOURCE_URL);
        String targetDir = intent.getStringExtra(BootstrapInstallContract.EXTRA_TARGET_DIR);

        if (abi == null || sha256 == null || sourceUrl == null || targetDir == null) {
            Log.e(TAG, "handleInstallRequest: missing required extras");
            broadcastResult(false, "MISSING_EXTRAS", abi);
            return;
        }

        Log.i(TAG, "Delegating bootstrap install to BootstrapInstallService"
                + " abi=" + abi + " targetDir=" + targetDir);

        Intent serviceIntent = new Intent(this, BootstrapInstallService.class);
        serviceIntent.putExtra(BootstrapInstallContract.EXTRA_ABI, abi);
        serviceIntent.putExtra(BootstrapInstallContract.EXTRA_SHA256, sha256);
        serviceIntent.putExtra(BootstrapInstallContract.EXTRA_SOURCE_URL, sourceUrl);
        serviceIntent.putExtra(BootstrapInstallContract.EXTRA_TARGET_DIR, targetDir);
        startService(serviceIntent);
    }

    private void broadcastResult(boolean success, String reason, String abi) {
        Intent result = new Intent(BootstrapInstallContract.ACTION_INSTALL_RESULT);
        result.putExtra(BootstrapInstallContract.EXTRA_SUCCESS, success);
        result.putExtra(BootstrapInstallContract.EXTRA_FAILURE_REASON, reason != null ? reason : "");
        result.putExtra(BootstrapInstallContract.EXTRA_INSTALLED_ABI, abi != null ? abi : "");
        sendBroadcast(result);
    }
}
