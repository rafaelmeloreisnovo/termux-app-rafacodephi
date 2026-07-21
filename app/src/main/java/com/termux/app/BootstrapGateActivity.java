package com.termux.app;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import com.termux.shared.logger.Logger;

/**
 * Launcher gate that prevents a race between external acquisition and embedded
 * bootstrap installation. It never downloads or extracts data itself.
 */
public final class BootstrapGateActivity extends Activity {

    private static final String LOG_TAG = "BootstrapGateActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            if (BootstrapLoaderClient.requestIfConfigured(this)) {
                finish();
                return;
            }
        } catch (Throwable t) {
            // A configured but invalid external route fails closed. Do not
            // silently downgrade to the embedded payload.
            Logger.logStackTraceWithMessage(
                    LOG_TAG,
                    "Configured external bootstrap route rejected",
                    t);
            finish();
            return;
        }

        Intent next = getIntent() == null ? new Intent() : new Intent(getIntent());
        next.setClass(this, TermuxActivity.class);
        startActivity(next);
        finish();
    }
}
