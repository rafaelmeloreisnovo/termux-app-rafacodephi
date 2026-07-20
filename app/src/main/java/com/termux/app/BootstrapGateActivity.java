package com.termux.app;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import com.termux.shared.logger.Logger;

/** Launcher gate that prevents races between external and embedded bootstrap paths. */
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
