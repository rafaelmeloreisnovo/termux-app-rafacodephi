package com.termux.app.event;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.data.IntentUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxUtils;
import com.termux.shared.termux.file.TermuxFileUtils;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;
import com.termux.shared.termux.shell.TermuxShellManager;

public class SystemEventReceiver extends BroadcastReceiver {

    private static SystemEventReceiver mInstance;

    private static final String LOG_TAG = "SystemEventReceiver";

    public static synchronized SystemEventReceiver getInstance() {
        if (mInstance == null) {
            mInstance = new SystemEventReceiver();
        }
        return mInstance;
    }

    @Override
    public void onReceive(@NonNull Context context, @Nullable Intent intent) {
        if (intent == null) {
            Logger.logError(LOG_TAG, "Received null intent");
            return;
        }
        
        try {
            Logger.logDebug(LOG_TAG, "Intent Received:\n" + IntentUtils.getIntentString(intent));

            String action = intent.getAction();
            if (action == null) {
                Logger.logError(LOG_TAG, "Received intent with null action");
                return;
            }

            switch (action) {
                case Intent.ACTION_BOOT_COMPLETED:
                    onActionBootCompleted(context, intent);
                    break;
                case Intent.ACTION_PACKAGE_ADDED:
                case Intent.ACTION_PACKAGE_REMOVED:
                case Intent.ACTION_PACKAGE_REPLACED:
                    onActionPackageUpdated(context, intent);
                    break;
                default:
                    Logger.logError(LOG_TAG, "Invalid action \"" + action + "\" passed to " + LOG_TAG);
            }
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error handling system event", e);
        }
    }

    public synchronized void onActionBootCompleted(@NonNull Context context, @NonNull Intent intent) {
        try {
            Logger.logInfo(LOG_TAG, "Device boot completed, initializing Termux environment");
            TermuxShellManager.onActionBootCompleted(context, intent);
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error handling boot completed event", e);
        }
    }

    public synchronized void onActionPackageUpdated(@NonNull Context context, @NonNull Intent intent) {
        try {
            Uri data = intent.getData();
            if (data != null && TermuxUtils.isUriDataForTermuxPluginPackage(data)) {
                String actionName = intent.getAction();
                String packageName = data.toString().replaceAll("^package:", "");
                
                if (actionName != null) {
                    Logger.logDebug(LOG_TAG, actionName.replaceAll("^android.intent.action.", "") +
                        " event received for \"" + packageName + "\"");
                }
                
                if (TermuxFileUtils.isTermuxFilesDirectoryAccessible(context, false, false) == null) {
                    TermuxShellEnvironment.writeEnvironmentToFile(context);
                }
            }
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error handling package update event", e);
        }
    }



    /**
     * Register {@link SystemEventReceiver} to listen to {@link Intent#ACTION_PACKAGE_ADDED},
     * {@link Intent#ACTION_PACKAGE_REMOVED} and {@link Intent#ACTION_PACKAGE_REPLACED} broadcasts.
     * They must be registered dynamically and cannot be registered implicitly in
     * the AndroidManifest.xml due to Android 8+ restrictions.
     *
     *  https://developer.android.com/guide/components/broadcast-exceptions
     */
    public synchronized static void registerPackageUpdateEvents(@NonNull Context context) {
        try {
            IntentFilter intentFilter = new IntentFilter();
            intentFilter.addAction(Intent.ACTION_PACKAGE_ADDED);
            intentFilter.addAction(Intent.ACTION_PACKAGE_REMOVED);
            intentFilter.addAction(Intent.ACTION_PACKAGE_REPLACED);
            intentFilter.addDataScheme("package");
            context.registerReceiver(getInstance(), intentFilter);
            Logger.logDebug(LOG_TAG, "Package update events registered successfully");
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error registering package update events", e);
        }
    }

    public synchronized static void unregisterPackageUpdateEvents(@NonNull Context context) {
        try {
            context.unregisterReceiver(getInstance());
            Logger.logDebug(LOG_TAG, "Package update events unregistered successfully");
        } catch (IllegalArgumentException e) {
            // Receiver was not registered, ignore
            Logger.logVerbose(LOG_TAG, "Receiver was not registered, ignoring unregister call");
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error unregistering package update events", e);
        }
    }

}
