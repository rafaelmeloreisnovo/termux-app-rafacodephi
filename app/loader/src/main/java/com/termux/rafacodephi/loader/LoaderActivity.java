package com.termux.rafacodephi.loader;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

/** Signature-protected entry point for bounded bootstrap acquisition. */
public final class LoaderActivity extends Activity {

    private static final String TAG = "LoaderActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent request = getIntent();
        try {
            if (request == null || !BootstrapInstallContract.ACTION_INSTALL_BOOTSTRAP.equals(
                    request.getAction())) {
                throw new IllegalArgumentException("INVALID_ACTION");
            }
            String abi = BootstrapSourcePolicy.requireAbi(
                    request.getStringExtra(BootstrapInstallContract.EXTRA_ABI));
            String sha256 = BootstrapSourcePolicy.requireSha256(
                    request.getStringExtra(BootstrapInstallContract.EXTRA_SHA256));
            String sourceUrl = request.getStringExtra(BootstrapInstallContract.EXTRA_SOURCE_URL);
            BootstrapSourcePolicy.requireInitialUrl(sourceUrl);

            Intent service = new Intent(this, BootstrapInstallService.class);
            service.setAction(BootstrapInstallContract.ACTION_INSTALL_BOOTSTRAP);
            service.putExtra(BootstrapInstallContract.EXTRA_ABI, abi);
            service.putExtra(BootstrapInstallContract.EXTRA_SHA256, sha256);
            service.putExtra(BootstrapInstallContract.EXTRA_SOURCE_URL, sourceUrl);
            startService(service);
            setResult(RESULT_OK);
        } catch (Exception e) {
            Log.e(TAG, "Rejected bootstrap request", e);
            BootstrapInstallService.publishFailure(
                    this,
                    request == null ? null
                            : request.getStringExtra(BootstrapInstallContract.EXTRA_ABI),
                    e.getMessage() == null ? e.getClass().getSimpleName() : e.getMessage());
            setResult(RESULT_CANCELED);
        } finally {
            finish();
        }
    }
}
