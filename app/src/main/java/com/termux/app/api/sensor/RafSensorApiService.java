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
import android.os.SystemClock;

import com.termux.lowlevel.BareMetal;
import com.termux.rafacodephi.R;
import com.termux.shared.logger.Logger;
import com.termux.shared.notification.NotificationUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.IdentityHashMap;
import java.util.List;
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
    private final Map<String, BatchRequest> activeBatchRequests = new ConcurrentHashMap<>();

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
        for (ActiveRequest request : new ArrayList<>(activeRequests.values())) {
            request.cancelInternal("ERR_SERVICE_STOP", "Service stopped");
        }
        for (BatchRequest request : new ArrayList<>(activeBatchRequests.values())) {
            request.cancelInternal("ERR_SERVICE_STOP", "Service stopped");
        }
        activeRequests.clear();
        activeBatchRequests.clear();
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
        } else if (RafSensorContract.ACTION_SENSOR_CATALOG.equals(action)) {
            handleCatalog(intent);
        } else if (RafSensorContract.ACTION_SENSOR_SNAPSHOT_ALL.equals(action)) {
            handleSnapshotAll(intent);
        } else if (RafSensorContract.ACTION_CANCEL_SENSOR_REQUEST.equals(action)) {
            handleCancel(intent);
        } else {
            Logger.logWarn(LOG_TAG, "Rejected unknown action: " + action);
            stopIfIdle();
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
        if (!validateCaller(callback, clientPackage, requestId, sensorName)) {
            stopIfIdle();
            return;
        }
        if (requestIdInUse(requestId)) {
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
            protocolVersion,
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

    private void handleCatalog(Intent intent) {
        BridgeEnvelope envelope = validateBridgeEnvelope(intent, "catalog");
        if (envelope == null) {
            stopIfIdle();
            return;
        }

        try {
            JSONObject catalog = buildCatalogJson();
            Intent completed = buildBaseResult(RafSensorContract.STATUS_COMPLETED, envelope.requestId, "all");
            completed.putExtra(RafSensorContract.RESULT_SENSOR_CATALOG_JSON, catalog.toString());
            completed.putExtra(RafSensorContract.RESULT_MESSAGE, "Runtime sensor catalog captured");
            dispatchCallback(envelope.callback, completed);
        } catch (JSONException e) {
            sendFailure(envelope.callback, envelope.requestId, "all", "ERR_CATALOG_JSON", e.getMessage());
        }
        stopIfIdle();
    }

    private void handleSnapshotAll(Intent intent) {
        BridgeEnvelope envelope = validateBridgeEnvelope(intent, "all");
        if (envelope == null) {
            stopIfIdle();
            return;
        }
        if (requestIdInUse(envelope.requestId)) {
            sendFailure(envelope.callback, envelope.requestId, "all", "ERR_DUPLICATE_REQUEST", "Request id is already active");
            stopIfIdle();
            return;
        }

        BatchRequest request = new BatchRequest(
            envelope.requestId,
            envelope.clientPackage,
            envelope.callback,
            envelope.timeoutMs
        );
        activeBatchRequests.put(envelope.requestId, request);
        request.start();
    }

    private BridgeEnvelope validateBridgeEnvelope(Intent intent, String sensorName) {
        int protocolVersion = intent.getIntExtra(RafSensorContract.EXTRA_PROTOCOL_VERSION, -1);
        String requestId = intent.getStringExtra(RafSensorContract.EXTRA_REQUEST_ID);
        int timeoutMs = intent.getIntExtra(RafSensorContract.EXTRA_TIMEOUT_MS, RafSensorContract.DEFAULT_BATCH_TIMEOUT_MS);
        PendingIntent callback = intent.getParcelableExtra(RafSensorContract.EXTRA_CALLBACK);
        String clientPackage = intent.getStringExtra(RafSensorContract.EXTRA_CLIENT_PACKAGE);

        RafSensorContract.ValidationResult validation = RafSensorContract.validateBridgeRequest(
            protocolVersion,
            requestId,
            timeoutMs,
            callback != null,
            clientPackage
        );
        if (!validation.valid) {
            sendFailure(callback, requestId, sensorName, validation.errorCode, validation.message);
            return null;
        }
        if (!validateCaller(callback, clientPackage, requestId, sensorName)) return null;
        return new BridgeEnvelope(
            requestId,
            clientPackage,
            callback,
            RafSensorContract.normalizeBatchTimeoutMs(timeoutMs)
        );
    }

    private boolean validateCaller(PendingIntent callback, String clientPackage, String requestId, String sensorName) {
        String callbackCreatorPackage = callback == null ? null : callback.getCreatorPackage();
        if (!RafSensorContract.callbackBelongsToClient(callbackCreatorPackage, clientPackage)) {
            sendFailure(callback, requestId, sensorName, "ERR_CALLBACK_OWNER", "Callback creator package does not match client package");
            return false;
        }
        if (getPackageManager().checkPermission(RafSensorContract.PERMISSION, clientPackage) != PackageManager.PERMISSION_GRANTED) {
            sendFailure(callback, requestId, sensorName, "ERR_CALLER_PERMISSION", "Client package does not hold RAFAELIA sensor permission");
            return false;
        }
        return true;
    }

    private boolean requestIdInUse(String requestId) {
        return activeRequests.containsKey(requestId) || activeBatchRequests.containsKey(requestId);
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
            return;
        }
        BatchRequest batchRequest = activeBatchRequests.remove(requestId);
        if (batchRequest != null) {
            batchRequest.cancelByClient();
        } else {
            stopIfIdle();
        }
    }

    private void sendFailure(PendingIntent callback, String requestId, String sensorName, String errorCode, String message) {
        if (callback == null) return;
        Intent result = buildBaseResult(RafSensorContract.STATUS_FAILED, requestId, sensorName);
        result.putExtra(RafSensorContract.RESULT_ERROR_CODE, errorCode);
        result.putExtra(RafSensorContract.RESULT_MESSAGE, message == null ? "Unspecified sensor bridge failure" : message);
        dispatchCallback(callback, result);
    }

    private Intent buildBaseResult(String status, String requestId, String sensorName) {
        return buildBaseResult(RafSensorContract.PROTOCOL_VERSION_2, status, requestId, sensorName);
    }

    private Intent buildBaseResult(int protocolVersion, String status, String requestId, String sensorName) {
        Intent result = new Intent();
        result.putExtra(RafSensorContract.RESULT_STATUS, status);
        result.putExtra(RafSensorContract.RESULT_PROTOCOL_VERSION, protocolVersion);
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
        if (!activeRequests.isEmpty() || !activeBatchRequests.isEmpty()) return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) stopForeground(true);
        stopSelf();
    }

    private JSONObject buildCatalogJson() throws JSONException {
        JSONObject root = new JSONObject();
        root.put("schema", "raf-sensor-catalog/v1");
        root.put("protocol_version", RafSensorContract.PROTOCOL_VERSION_2);
        root.put("captured_elapsed_realtime_ns", SystemClock.elapsedRealtimeNanos());

        JSONArray names = new JSONArray();
        JSONArray catalog = new JSONArray();
        List<Sensor> sensors = sortedSensors();
        int count = Math.min(sensors.size(), RafSensorContract.MAX_CATALOG_SENSORS);
        for (int i = 0; i < count; i++) {
            Sensor sensor = sensors.get(i);
            names.put(sensor.getName());
            catalog.put(buildSensorMetadata(sensor));
        }
        root.put("sensor_count", count);
        root.put("truncated", sensors.size() > count);
        root.put("sensors", names);
        root.put("catalog", catalog);
        return root;
    }

    private List<Sensor> sortedSensors() {
        if (sensorManager == null) return Collections.emptyList();
        List<Sensor> sensors = new ArrayList<>(sensorManager.getSensorList(Sensor.TYPE_ALL));
        Collections.sort(sensors, new Comparator<Sensor>() {
            @Override
            public int compare(Sensor a, Sensor b) {
                int typeOrder = Integer.compare(a.getType(), b.getType());
                if (typeOrder != 0) return typeOrder;
                return a.getName().compareToIgnoreCase(b.getName());
            }
        });
        return sensors;
    }

    private JSONObject buildSensorMetadata(Sensor sensor) throws JSONException {
        JSONObject item = new JSONObject();
        item.put("name", sensor.getName());
        item.put("type", sensor.getType());
        item.put("vendor", sensor.getVendor());
        item.put("version", sensor.getVersion());
        item.put("max_range", sensor.getMaximumRange());
        item.put("resolution", sensor.getResolution());
        item.put("power_ma", sensor.getPower());
        item.put("min_delay_us", sensor.getMinDelay());
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH) {
            item.put("string_type", sensor.getStringType());
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            item.put("max_delay_us", sensor.getMaxDelay());
            item.put("reporting_mode", RafSensorAndroid.reportingModeToString(sensor));
            item.put("wake_up", sensor.isWakeUpSensor());
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            item.put("dynamic", sensor.isDynamicSensor());
            item.put("id", sensor.getId());
        }
        return item;
    }

    private final class ActiveRequest implements SensorEventListener {
        private final int protocolVersion;
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

        private ActiveRequest(int protocolVersion, String requestId, String sensorName, String clientPackage, PendingIntent callback,
                              Sensor sensor, int samplingPeriodUs, int maxReportLatencyUs) {
            this.protocolVersion = protocolVersion;
            this.requestId = requestId;
            this.sensorName = sensorName;
            this.clientPackage = clientPackage;
            this.callback = callback;
            this.sensor = sensor;
            this.samplingPeriodUs = samplingPeriodUs;
            this.maxReportLatencyUs = maxReportLatencyUs;
        }

        private void start() {
            Intent accepted = buildBaseResult(protocolVersion, RafSensorContract.STATUS_ACCEPTED, requestId, sensorName);
            accepted.putExtra(RafSensorContract.RESULT_MESSAGE, "Request accepted");
            accepted.putExtra(RafSensorContract.RESULT_SENSOR_TYPE, sensor.getType());
            accepted.putExtra(RafSensorContract.RESULT_SENSOR_VENDOR, sensor.getVendor());
            accepted.putExtra(RafSensorContract.RESULT_SENSOR_VERSION, sensor.getVersion());
            dispatchCallback(callback, accepted);

            boolean registered;
            try {
                registered = sensorManager != null && sensorManager.registerListener(
                    this, sensor, samplingPeriodUs, maxReportLatencyUs, sensorHandler);
            } catch (SecurityException e) {
                activeRequests.remove(requestId);
                failInternal("ERR_SENSOR_PERMISSION", "Android denied access to this sensor");
                return;
            }
            if (!registered) {
                activeRequests.remove(requestId);
                failInternal("ERR_REGISTER_LISTENER", "SensorManager rejected listener registration");
                return;
            }

            Intent sampling = buildBaseResult(protocolVersion, RafSensorContract.STATUS_SAMPLING, requestId, sensorName);
            sampling.putExtra(RafSensorContract.RESULT_MESSAGE, "Sampling started");
            sampling.putExtra(RafSensorContract.RESULT_SENSOR_TYPE, sensor.getType());
            dispatchCallback(callback, sampling);
            sensorHandler.postDelayed(timeoutRunnable, REQUEST_TIMEOUT_MS);
        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            if (!finished.compareAndSet(false, true)) return;
            cleanup();

            Intent completed = buildBaseResult(protocolVersion, RafSensorContract.STATUS_COMPLETED, requestId, sensorName);
            completed.putExtra(RafSensorContract.RESULT_SENSOR_TYPE, sensor.getType());
            completed.putExtra(RafSensorContract.RESULT_SENSOR_VENDOR, sensor.getVendor());
            completed.putExtra(RafSensorContract.RESULT_SENSOR_VERSION, sensor.getVersion());
            completed.putExtra(RafSensorContract.RESULT_TIMESTAMP_NS, event.timestamp);
            completed.putExtra(RafSensorContract.RESULT_ACCURACY, event.accuracy);
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
        public void onAccuracyChanged(Sensor changedSensor, int accuracy) {
            lastAccuracy = accuracy;
        }

        private void cancelByClient() {
            cancelInternal("ERR_CANCELLED", "Cancelled by client");
        }

        private void cancelInternal(String errorCode, String message) {
            if (!finished.compareAndSet(false, true)) return;
            cleanup();
            Intent cancelled = buildBaseResult(protocolVersion, RafSensorContract.STATUS_CANCELLED, requestId, sensorName);
            cancelled.putExtra(RafSensorContract.RESULT_ERROR_CODE, errorCode);
            cancelled.putExtra(RafSensorContract.RESULT_MESSAGE, message);
            dispatchCallback(callback, cancelled);
            stopIfIdle();
        }

        private void failInternal(String errorCode, String message) {
            if (!finished.compareAndSet(false, true)) return;
            cleanup();
            Intent failure = buildBaseResult(protocolVersion, RafSensorContract.STATUS_FAILED, requestId, sensorName);
            failure.putExtra(RafSensorContract.RESULT_ERROR_CODE, errorCode);
            failure.putExtra(RafSensorContract.RESULT_MESSAGE, message);
            dispatchCallback(callback, failure);
            stopIfIdle();
        }

        private void cleanup() {
            activeRequests.remove(requestId);
            if (sensorManager != null) sensorManager.unregisterListener(this);
            if (sensorHandler != null) sensorHandler.removeCallbacks(timeoutRunnable);
        }
    }

    private final class BatchRequest implements SensorEventListener {
        private final String requestId;
        private final String clientPackage;
        private final PendingIntent callback;
        private final int timeoutMs;
        private final AtomicBoolean finished = new AtomicBoolean(false);
        private final IdentityHashMap<Sensor, JSONObject> rows = new IdentityHashMap<>();
        private final List<Sensor> sensors = new ArrayList<>();
        private int pendingCount;

        private final Runnable timeoutRunnable = new Runnable() {
            @Override
            public void run() {
                complete(true);
            }
        };

        private BatchRequest(String requestId, String clientPackage, PendingIntent callback, int timeoutMs) {
            this.requestId = requestId;
            this.clientPackage = clientPackage;
            this.callback = callback;
            this.timeoutMs = timeoutMs;
        }

        private void start() {
            Intent accepted = buildBaseResult(RafSensorContract.STATUS_ACCEPTED, requestId, "all");
            accepted.putExtra(RafSensorContract.RESULT_MESSAGE, "All-sensor request accepted");
            dispatchCallback(callback, accepted);

            List<Sensor> available = sortedSensors();
            int count = Math.min(available.size(), RafSensorContract.MAX_CATALOG_SENSORS);
            for (int i = 0; i < count; i++) {
                Sensor sensor = available.get(i);
                sensors.add(sensor);
                try {
                    JSONObject row = buildSensorMetadata(sensor);
                    row.put("status", "PENDING");
                    rows.put(sensor, row);
                } catch (JSONException e) {
                    Logger.logStackTraceWithMessage(LOG_TAG, "Failed to build sensor row", e);
                    continue;
                }

                if (isTriggerOnly(sensor)) {
                    setStatus(sensor, "UNSUPPORTED_TRIGGER_MODE", "One-shot/special trigger sensors are inventoried but not auto-fired");
                    continue;
                }

                try {
                    boolean registered = sensorManager != null && sensorManager.registerListener(
                        this,
                        sensor,
                        SensorManager.SENSOR_DELAY_GAME,
                        0,
                        sensorHandler
                    );
                    if (registered) {
                        pendingCount++;
                    } else {
                        setStatus(sensor, "REGISTER_REJECTED", "SensorManager rejected listener registration");
                    }
                } catch (SecurityException e) {
                    setStatus(sensor, "PERMISSION_REQUIRED", e.getMessage());
                } catch (RuntimeException e) {
                    setStatus(sensor, "REGISTER_ERROR", e.getClass().getSimpleName() + ": " + e.getMessage());
                }
            }

            Intent sampling = buildBaseResult(RafSensorContract.STATUS_SAMPLING, requestId, "all");
            sampling.putExtra(RafSensorContract.RESULT_MESSAGE, "Sampling all observable SensorManager sensors");
            dispatchCallback(callback, sampling);

            if (pendingCount == 0) {
                complete(false);
            } else {
                sensorHandler.postDelayed(timeoutRunnable, timeoutMs);
            }
        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            if (finished.get()) return;
            JSONObject row = rows.get(event.sensor);
            if (row == null || !"PENDING".equals(row.optString("status"))) return;
            try {
                JSONArray values = new JSONArray();
                int valueCount = Math.min(event.values.length, RafSensorContract.MAX_SENSOR_VALUES);
                for (int i = 0; i < valueCount; i++) values.put(event.values[i]);
                row.put("status", "SAMPLED");
                row.put("timestamp_ns", event.timestamp);
                row.put("accuracy", event.accuracy);
                row.put("values", values);
                row.put("values_truncated", event.values.length > valueCount);
            } catch (JSONException e) {
                setStatus(event.sensor, "SERIALIZE_ERROR", e.getMessage());
            }
            if (sensorManager != null) sensorManager.unregisterListener(this, event.sensor);
            pendingCount--;
            if (pendingCount <= 0) complete(false);
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            JSONObject row = rows.get(sensor);
            if (row != null) {
                try {
                    row.put("last_accuracy", accuracy);
                } catch (JSONException ignored) {
                }
            }
        }

        private boolean isTriggerOnly(Sensor sensor) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return false;
            int mode = sensor.getReportingMode();
            return mode == Sensor.REPORTING_MODE_ONE_SHOT || mode == Sensor.REPORTING_MODE_SPECIAL_TRIGGER;
        }

        private void setStatus(Sensor sensor, String status, String detail) {
            JSONObject row = rows.get(sensor);
            if (row == null) return;
            try {
                row.put("status", status);
                if (detail != null && !detail.isEmpty()) row.put("detail", detail);
            } catch (JSONException ignored) {
            }
        }

        private void complete(boolean timedOut) {
            if (!finished.compareAndSet(false, true)) return;
            cleanupListeners();
            if (timedOut) {
                for (Map.Entry<Sensor, JSONObject> entry : rows.entrySet()) {
                    if ("PENDING".equals(entry.getValue().optString("status"))) {
                        setStatus(entry.getKey(), "TIMEOUT", "No sample before batch deadline");
                    }
                }
            }

            try {
                JSONObject payload = buildBatchPayload(timedOut);
                Intent completed = buildBaseResult(RafSensorContract.STATUS_COMPLETED, requestId, "all");
                completed.putExtra(RafSensorContract.RESULT_SENSOR_BATCH_JSON, payload.toString());
                completed.putExtra(RafSensorContract.RESULT_MESSAGE, timedOut
                    ? "Batch completed with timeout-marked sensors"
                    : "Batch completed");
                dispatchCallback(callback, completed);
            } catch (JSONException e) {
                sendFailure(callback, requestId, "all", "ERR_BATCH_JSON", e.getMessage());
            }
            activeBatchRequests.remove(requestId);
            stopIfIdle();
        }

        private JSONObject buildBatchPayload(boolean timedOut) throws JSONException {
            JSONObject root = new JSONObject();
            root.put("schema", "raf-sensor-batch/v1");
            root.put("protocol_version", RafSensorContract.PROTOCOL_VERSION_2);
            root.put("request_id", requestId);
            root.put("client_package", clientPackage);
            root.put("completed_elapsed_realtime_ns", SystemClock.elapsedRealtimeNanos());
            root.put("timeout_ms", timeoutMs);
            root.put("timed_out", timedOut);

            JSONArray samples = new JSONArray();
            JSONArray names = new JSONArray();
            JSONObject readings = new JSONObject();
            int sampled = 0;
            int blocked = 0;
            for (Sensor sensor : sensors) {
                JSONObject row = rows.get(sensor);
                if (row == null) continue;
                samples.put(row);
                names.put(sensor.getName());
                String status = row.optString("status");
                if ("SAMPLED".equals(status)) {
                    sampled++;
                    JSONObject legacy = new JSONObject();
                    legacy.put("values", row.optJSONArray("values"));
                    legacy.put("timestamp_ns", row.optLong("timestamp_ns"));
                    legacy.put("accuracy", row.optInt("accuracy"));
                    readings.put(sensor.getName(), legacy);
                } else {
                    blocked++;
                }
            }
            root.put("sensor_count", samples.length());
            root.put("sampled_count", sampled);
            root.put("non_sampled_count", blocked);
            root.put("sensors", names);
            root.put("readings", readings);
            root.put("samples", samples);

            BareMetal.HardwareProfile profile = BareMetal.readHardwareProfile();
            JSONObject hardware = new JSONObject();
            hardware.put("abi", profile.abi);
            hardware.put("access_flags", profile.accessFlags);
            hardware.put("page_size", profile.pageSize);
            hardware.put("cache_line", profile.cacheLine);
            hardware.put("cpus_online", profile.cpusOnline);
            root.put("hardware", hardware);
            return root;
        }

        private void cancelByClient() {
            cancelInternal("ERR_CANCELLED", "Cancelled by client");
        }

        private void cancelInternal(String errorCode, String message) {
            if (!finished.compareAndSet(false, true)) return;
            cleanupListeners();
            activeBatchRequests.remove(requestId);
            Intent cancelled = buildBaseResult(RafSensorContract.STATUS_CANCELLED, requestId, "all");
            cancelled.putExtra(RafSensorContract.RESULT_ERROR_CODE, errorCode);
            cancelled.putExtra(RafSensorContract.RESULT_MESSAGE, message);
            dispatchCallback(callback, cancelled);
            stopIfIdle();
        }

        private void cleanupListeners() {
            if (sensorManager != null) sensorManager.unregisterListener(this);
            if (sensorHandler != null) sensorHandler.removeCallbacks(timeoutRunnable);
            pendingCount = 0;
        }
    }

    private static final class BridgeEnvelope {
        final String requestId;
        final String clientPackage;
        final PendingIntent callback;
        final int timeoutMs;

        BridgeEnvelope(String requestId, String clientPackage, PendingIntent callback, int timeoutMs) {
            this.requestId = requestId;
            this.clientPackage = clientPackage;
            this.callback = callback;
            this.timeoutMs = timeoutMs;
        }
    }
}
