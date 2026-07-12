package com.termux.app.api.sensor;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;

import com.termux.lowlevel.BareMetal;
import com.termux.rafacodephi.R;
import com.termux.shared.logger.Logger;
import com.termux.shared.notification.NotificationUtils;

import java.util.Arrays;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;

public class RafSensorApiService extends Service {

    private static final String LOG_TAG = "RafSensorApiService";
    private static final String NOTIFICATION_CHANNEL_ID = "raf_sensor_runtime";
    private static final String NOTIFICATION_CHANNEL_NAME = "RAFAELIA Sensor Runtime";
    private static final int NOTIFICATION_ID = 0x52414653;
    private static final long REQUEST_TIMEOUT_MS = 3_000L;

    private final Map<String, ActiveRequest> activeRequests = new ConcurrentHashMap<>();

    private SensorManager sensorManager;
    private HandlerThread sensorThread;
    private Handler sensorHandler;

    @Override
    public void onCreate() {
        super.onCreate();
        sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        sensorThread = new HandlerThread("raf-sensor-runtime");
        sensorThread.start();
        sensorHandler = new Handler(sensorThread.getLooper());
        runStartForeground();
    }

    @Override
    public void onDestroy() {
        for (ActiveRequest request : activeRequests.values()) {
            request.cancelInternal("ERR_SERVICE_STOP", "Service stopped");
        }
        activeRequests.clear();
        if (sensorThread != null) {
            sensorThread.quitSafely();
            sensorThread = null;
        }
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        runStartForeground();
        if (intent == null || intent.getAction() == null) {
            stopIfIdle();
            return START_NOT_STICKY;
        }

        String action = intent.getAction();
        if (RafSensorContract.ACTION_SENSOR_SNAPSHOT.equals(action)) {
            handleSnapshot(intent);
        } else if (RafSensorContract.ACTION_CANCEL_SENSOR_REQUEST.equals(action)) {
            handleCancel(intent);
        } else {
            Logger.logWarn(LOG_TAG, "Rejected unknown action: " + action);
        }

        return START_NOT_STICKY;
    }

    private void handleSnapshot(Intent intent) {
        int protocolVersion = intent.getIntExtra(RafSensorContract.EXTRA_PROTOCOL_VERSION, -1);
        String requestId = intent.getStringExtra(RafSensorContract.EXTRA_REQUEST_ID);
        String sensorName = intent.getStringExtra(RafSensorContract.EXTRA_SENSOR_NAME);
        int samplingPeriodUs = intent.getIntExtra(RafSensorContract.EXTRA_SAMPLING_PERIOD_US, RafSensorContract.DEFAULT_SAMPLING_PERIOD_US);
        int maxReportLatencyUs = intent.getIntExtra(RafSensorContract.EXTRA_MAX_REPORT_LATENCY_US, RafSensorContract.DEFAULT_MAX_REPORT_LATENCY_US);
        PendingIntent callback = intent.getParcelableExtra(RafSensorContract.EXTRA_CALLBACK);
        String clientPackage = intent.getStringExtra(RafSensorContract.EXTRA_CLIENT_PACKAGE);

        RafSensorContract.ValidationResult validation = RafSensorContract.validateSnapshotRequest(
            protocolVersion,
            requestId,
            sensorName,
            samplingPeriodUs,
            maxReportLatencyUs,
            callback != null,
            clientPackage
        );
        if (!validation.valid) {
            sendFailure(callback, requestId, sensorName, validation.errorCode, validation.message);
            stopIfIdle();
            return;
        }

        if (getPackageManager().checkPermission(RafSensorContract.PERMISSION, clientPackage) != PackageManager.PERMISSION_GRANTED) {
            sendFailure(callback, requestId, sensorName, "ERR_CALLER_PERMISSION", "Client package does not hold RAFAELIA sensor permission");
            stopIfIdle();
            return;
        }

        if (activeRequests.containsKey(requestId)) {
            sendFailure(callback, requestId, sensorName, "ERR_DUPLICATE_REQUEST", "Request id is already active");
            stopIfIdle();
            return;
        }

        int sensorType = RafSensorAndroid.toSensorType(sensorName);
        Sensor sensor = sensorManager == null ? null : sensorManager.getDefaultSensor(sensorType);
        if (sensor == null) {
            sendFailure(callback, requestId, sensorName, "ERR_SENSOR_UNAVAILABLE", "Requested sensor is not available on this device");
            stopIfIdle();
            return;
        }

        ActiveRequest request = new ActiveRequest(
            requestId,
            sensorName,
            clientPackage,
            callback,
            sensor,
            RafSensorContract.normalizeSamplingPeriodUs(samplingPeriodUs),
            RafSensorContract.normalizeMaxReportLatencyUs(maxReportLatencyUs)
        );
        activeRequests.put(requestId, request);
        request.start();
    }

    private void handleCancel(Intent intent) {
        String requestId = intent.getStringExtra(RafSensorContract.EXTRA_REQUEST_ID);
        if (requestId == null || requestId.isEmpty()) {
            stopIfIdle();
            return;
        }
        ActiveRequest request = activeRequests.remove(requestId);
        if (request != null) {
            request.cancelByClient();
        } else {
            stopIfIdle();
        }
    }

    private void sendFailure(PendingIntent callback, String requestId, String sensorName, String errorCode, String message) {
        if (callback == null) return;
        Intent result = buildBaseResult(RafSensorContract.STATUS_FAILED, requestId, sensorName);
        result.putExtra(RafSensorContract.RESULT_ERROR_CODE, errorCode);
        result.putExtra(RafSensorContract.RESULT_MESSAGE, message);
        dispatchCallback(callback, result);
    }

    private Intent buildBaseResult(String status, String requestId, String sensorName) {
        Intent result = new Intent();
        result.putExtra(RafSensorContract.RESULT_STATUS, status);
        result.putExtra(RafSensorContract.RESULT_PROTOCOL_VERSION, RafSensorContract.PROTOCOL_VERSION_1);
        result.putExtra(RafSensorContract.RESULT_REQUEST_ID, requestId);
        result.putExtra(RafSensorContract.RESULT_SENSOR_NAME, sensorName);
        return result;
    }

    private void dispatchCallback(PendingIntent callback, Intent result) {
        try {
            callback.send(this, 0, result);
        } catch (PendingIntent.CanceledException e) {
            Logger.logWarn(LOG_TAG, "Callback delivery failed: " + e.getMessage());
        }
    }

    private void runStartForeground() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationUtils.setupNotificationChannel(this, NOTIFICATION_CHANNEL_ID, NOTIFICATION_CHANNEL_NAME, NotificationManager.IMPORTANCE_LOW);
            Notification.Builder builder = NotificationUtils.geNotificationBuilder(
                this,
                NOTIFICATION_CHANNEL_ID,
                Notification.PRIORITY_LOW,
                NOTIFICATION_CHANNEL_NAME,
                getString(R.string.raf_sensor_service_notification_title),
                getString(R.string.raf_sensor_service_notification_text),
                null,
                null,
                NotificationUtils.NOTIFICATION_MODE_SILENT
            );
            if (builder != null) {
                builder.setSmallIcon(R.drawable.ic_service_notification);
                builder.setShowWhen(false);
                builder.setColor(0xFF607D8B);
                startForeground(NOTIFICATION_ID, builder.build());
            }
        }
    }

    private void stopIfIdle() {
        if (!activeRequests.isEmpty()) return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            stopForeground(true);
        }
        stopSelf();
    }

    private final class ActiveRequest implements SensorEventListener {
        private final String requestId;
        private final String sensorName;
        private final String clientPackage;
        private final PendingIntent callback;
        private final Sensor sensor;
        private final int samplingPeriodUs;
        private final int maxReportLatencyUs;
        private final AtomicBoolean finished = new AtomicBoolean(false);

        private volatile int lastAccuracy = SensorManager.SENSOR_STATUS_UNRELIABLE;

        private final Runnable timeoutRunnable = new Runnable() {
            @Override
            public void run() {
                failInternal("ERR_TIMEOUT", "Sensor sample timed out");
            }
        };

        private ActiveRequest(String requestId, String sensorName, String clientPackage, PendingIntent callback,
                              Sensor sensor, int samplingPeriodUs, int maxReportLatencyUs) {
            this.requestId = requestId;
            this.sensorName = sensorName;
            this.clientPackage = clientPackage;
            this.callback = callback;
            this.sensor = sensor;
            this.samplingPeriodUs = samplingPeriodUs;
            this.maxReportLatencyUs = maxReportLatencyUs;
        }

        private void start() {
            Intent accepted = buildBaseResult(RafSensorContract.STATUS_ACCEPTED, requestId, sensorName);
            accepted.putExtra(RafSensorContract.RESULT_MESSAGE, "Request accepted");
            accepted.putExtra(RafSensorContract.RESULT_SENSOR_TYPE, sensor.getType());
            accepted.putExtra(RafSensorContract.RESULT_SENSOR_VENDOR, sensor.getVendor());
            accepted.putExtra(RafSensorContract.RESULT_SENSOR_VERSION, sensor.getVersion());
            dispatchCallback(callback, accepted);

            boolean registered = sensorManager != null && sensorManager.registerListener(
                this,
                sensor,
                samplingPeriodUs,
                maxReportLatencyUs,
                sensorHandler
            );
            if (!registered) {
                activeRequests.remove(requestId);
                failInternal("ERR_REGISTER_LISTENER", "SensorManager rejected listener registration");
                return;
            }

            Intent sampling = buildBaseResult(RafSensorContract.STATUS_SAMPLING, requestId, sensorName);
            sampling.putExtra(RafSensorContract.RESULT_MESSAGE, "Sampling started");
            sampling.putExtra(RafSensorContract.RESULT_SENSOR_TYPE, sensor.getType());
            dispatchCallback(callback, sampling);

            sensorHandler.postDelayed(timeoutRunnable, REQUEST_TIMEOUT_MS);
        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            if (!finished.compareAndSet(false, true)) return;
            cleanup();

            Intent completed = buildBaseResult(RafSensorContract.STATUS_COMPLETED, requestId, sensorName);
            completed.putExtra(RafSensorContract.RESULT_SENSOR_TYPE, sensor.getType());
            completed.putExtra(RafSensorContract.RESULT_SENSOR_VENDOR, sensor.getVendor());
            completed.putExtra(RafSensorContract.RESULT_SENSOR_VERSION, sensor.getVersion());
            completed.putExtra(RafSensorContract.RESULT_TIMESTAMP_NS, event.timestamp);
            completed.putExtra(RafSensorContract.RESULT_ACCURACY, lastAccuracy);
            completed.putExtra(RafSensorContract.RESULT_VALUES, Arrays.copyOf(event.values, event.values.length));

            BareMetal.HardwareProfile profile = BareMetal.readHardwareProfile();
            completed.putExtra(RafSensorContract.RESULT_HARDWARE_ABI, profile.abi);
            completed.putExtra(RafSensorContract.RESULT_HARDWARE_FLAGS, profile.accessFlags);
            completed.putExtra(RafSensorContract.RESULT_PAGE_SIZE, profile.pageSize);
            completed.putExtra(RafSensorContract.RESULT_CACHE_LINE, profile.cacheLine);
            completed.putExtra(RafSensorContract.RESULT_CPUS_ONLINE, profile.cpusOnline);
            completed.putExtra(RafSensorContract.RESULT_MESSAGE, "Sample captured for " + clientPackage);

            dispatchCallback(callback, completed);
            stopIfIdle();
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            lastAccuracy = accuracy;
        }

        private void cancelByClient() {
            cancelInternal("ERR_CANCELLED", "Cancelled by client");
        }

        private void cancelInternal(String errorCode, String message) {
            if (!finished.compareAndSet(false, true)) return;
            cleanup();
            Intent cancelled = buildBaseResult(RafSensorContract.STATUS_CANCELLED, requestId, sensorName);
            cancelled.putExtra(RafSensorContract.RESULT_ERROR_CODE, errorCode);
            cancelled.putExtra(RafSensorContract.RESULT_MESSAGE, message);
            dispatchCallback(callback, cancelled);
            stopIfIdle();
        }

        private void failInternal(String errorCode, String message) {
            if (!finished.compareAndSet(false, true)) return;
            cleanup();
            Intent failure = buildBaseResult(RafSensorContract.STATUS_FAILED, requestId, sensorName);
            failure.putExtra(RafSensorContract.RESULT_ERROR_CODE, errorCode);
            failure.putExtra(RafSensorContract.RESULT_MESSAGE, message);
            dispatchCallback(callback, failure);
            stopIfIdle();
        }

        private void cleanup() {
            activeRequests.remove(requestId);
            if (sensorManager != null) {
                sensorManager.unregisterListener(this);
            }
            if (sensorHandler != null) {
                sensorHandler.removeCallbacks(timeoutRunnable);
            }
        }
    }
}
