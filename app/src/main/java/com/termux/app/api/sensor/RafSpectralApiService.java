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

import com.termux.rafacodephi.R;
import com.termux.shared.logger.Logger;
import com.termux.shared.notification.NotificationUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Explicit, bounded sensor-window acquisition followed by local spectral analysis.
 * No microphone capture, file persistence or background scheduling is performed.
 */
public class RafSpectralApiService extends Service {

    private static final String LOG_TAG = "RafSpectralApiService";
    private static final String NOTIFICATION_CHANNEL_ID = "raf_sensor_runtime";
    private static final String NOTIFICATION_CHANNEL_NAME = "RAFAELIA Sensor Runtime";
    private static final int NOTIFICATION_ID = 0x52414650;

    private final Map<String, SpectrumRequest> activeRequests = new ConcurrentHashMap<>();
    private SensorManager sensorManager;
    private HandlerThread workerThread;
    private Handler workerHandler;

    @Override
    public void onCreate() {
        super.onCreate();
        sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        workerThread = new HandlerThread("raf-spectral-runtime");
        workerThread.start();
        workerHandler = new Handler(workerThread.getLooper());
        startAsForegroundService();
    }

    @Override
    public void onDestroy() {
        for (SpectrumRequest request : new ArrayList<>(activeRequests.values())) {
            request.cancelInternal("ERR_SERVICE_STOP", "Spectral service stopped");
        }
        activeRequests.clear();
        if (workerThread != null) {
            workerThread.quitSafely();
            workerThread = null;
        }
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        startAsForegroundService();
        if (intent == null || !RafSpectralContract.ACTION_SENSOR_SPECTRUM.equals(intent.getAction())) {
            Logger.logWarn(LOG_TAG, "Rejected null or unsupported action");
            stopIfIdle();
            return START_NOT_STICKY;
        }
        handleSpectrum(intent);
        return START_NOT_STICKY;
    }

    private void handleSpectrum(Intent intent) {
        int protocolVersion = intent.getIntExtra(RafSpectralContract.EXTRA_PROTOCOL_VERSION, -1);
        String requestId = intent.getStringExtra(RafSpectralContract.EXTRA_REQUEST_ID);
        String sensorName = intent.getStringExtra(RafSpectralContract.EXTRA_SENSOR_NAME);
        String axis = intent.getStringExtra(RafSpectralContract.EXTRA_AXIS);
        if (axis == null) axis = RafSpectralContract.AXIS_MAGNITUDE;
        int sampleCount = intent.getIntExtra(
            RafSpectralContract.EXTRA_SAMPLE_COUNT,
            RafSpectralContract.DEFAULT_SAMPLE_COUNT
        );
        int samplingPeriodUs = intent.getIntExtra(
            RafSpectralContract.EXTRA_SAMPLING_PERIOD_US,
            RafSpectralContract.DEFAULT_SAMPLING_PERIOD_US
        );
        int timeoutMs = intent.getIntExtra(
            RafSpectralContract.EXTRA_TIMEOUT_MS,
            RafSpectralContract.DEFAULT_TIMEOUT_MS
        );
        String window = intent.getStringExtra(RafSpectralContract.EXTRA_WINDOW);
        if (window == null) window = "hann";
        PendingIntent callback = intent.getParcelableExtra(RafSpectralContract.EXTRA_CALLBACK);
        String clientPackage = intent.getStringExtra(RafSpectralContract.EXTRA_CLIENT_PACKAGE);

        RafSpectralContract.ValidationResult validation = RafSpectralContract.validateRequest(
            protocolVersion,
            requestId,
            sensorName,
            axis,
            sampleCount,
            samplingPeriodUs,
            timeoutMs,
            window,
            callback != null,
            clientPackage
        );
        if (!validation.valid) {
            sendFailure(callback, requestId, validation.errorCode, validation.message);
            stopIfIdle();
            return;
        }
        if (!validateCaller(callback, clientPackage, requestId)) {
            stopIfIdle();
            return;
        }
        if (activeRequests.containsKey(requestId)) {
            sendFailure(callback, requestId, "ERR_DUPLICATE_REQUEST", "Request id is already active");
            stopIfIdle();
            return;
        }

        int sensorType = RafSensorAndroid.toSensorType(sensorName);
        Sensor sensor = sensorManager == null ? null : sensorManager.getDefaultSensor(sensorType);
        if (sensor == null) {
            sendFailure(callback, requestId, "ERR_SENSOR_UNAVAILABLE", "Requested sensor is not available");
            stopIfIdle();
            return;
        }

        SpectrumRequest request = new SpectrumRequest(
            requestId,
            clientPackage,
            callback,
            sensorName,
            sensor,
            axis,
            sampleCount,
            samplingPeriodUs,
            timeoutMs,
            window
        );
        activeRequests.put(requestId, request);
        request.start();
    }

    private boolean validateCaller(PendingIntent callback, String clientPackage, String requestId) {
        String callbackCreatorPackage = callback == null ? null : callback.getCreatorPackage();
        if (!RafSpectralContract.callbackBelongsToClient(callbackCreatorPackage, clientPackage)) {
            sendFailure(callback, requestId, "ERR_CALLBACK_OWNER", "Callback creator does not match client package");
            return false;
        }
        if (getPackageManager().checkPermission(RafSensorContract.PERMISSION, clientPackage)
            != PackageManager.PERMISSION_GRANTED) {
            sendFailure(callback, requestId, "ERR_CALLER_PERMISSION", "Client does not hold RAFAELIA sensor permission");
            return false;
        }
        return true;
    }

    private final class SpectrumRequest implements SensorEventListener {
        private final String requestId;
        private final String clientPackage;
        private final PendingIntent callback;
        private final String sensorName;
        private final Sensor sensor;
        private final String axis;
        private final int requestedSampleCount;
        private final int samplingPeriodUs;
        private final int timeoutMs;
        private final String window;
        private final double[] values;
        private final long[] timestampsNs;
        private final AtomicBoolean finished = new AtomicBoolean(false);
        private int collected;

        private final Runnable timeoutRunnable = new Runnable() {
            @Override
            public void run() {
                if (collected >= RafSpectralAnalyzer.MIN_SAMPLES) {
                    complete(true);
                } else {
                    failInternal("ERR_SPECTRUM_TIMEOUT", "Too few samples before deadline: " + collected);
                }
            }
        };

        private SpectrumRequest(String requestId,
                                String clientPackage,
                                PendingIntent callback,
                                String sensorName,
                                Sensor sensor,
                                String axis,
                                int sampleCount,
                                int samplingPeriodUs,
                                int timeoutMs,
                                String window) {
            this.requestId = requestId;
            this.clientPackage = clientPackage;
            this.callback = callback;
            this.sensorName = sensorName;
            this.sensor = sensor;
            this.axis = axis;
            this.requestedSampleCount = sampleCount;
            this.samplingPeriodUs = samplingPeriodUs;
            this.timeoutMs = timeoutMs;
            this.window = window;
            this.values = new double[sampleCount];
            this.timestampsNs = new long[sampleCount];
        }

        private void start() {
            Intent accepted = baseResult(RafSpectralContract.STATUS_ACCEPTED, requestId);
            accepted.putExtra(RafSpectralContract.RESULT_MESSAGE, "Spectral request accepted");
            dispatch(callback, accepted);

            boolean registered;
            try {
                registered = sensorManager != null && sensorManager.registerListener(
                    this,
                    sensor,
                    samplingPeriodUs,
                    0,
                    workerHandler
                );
            } catch (SecurityException error) {
                activeRequests.remove(requestId);
                failInternal("ERR_SENSOR_PERMISSION", "Android denied sensor access");
                return;
            }
            if (!registered) {
                activeRequests.remove(requestId);
                failInternal("ERR_REGISTER_LISTENER", "SensorManager rejected listener registration");
                return;
            }

            Intent sampling = baseResult(RafSpectralContract.STATUS_SAMPLING, requestId);
            sampling.putExtra(RafSpectralContract.RESULT_MESSAGE, "Collecting bounded sensor window");
            dispatch(callback, sampling);
            workerHandler.postDelayed(timeoutRunnable, timeoutMs);
        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            if (finished.get() || collected >= requestedSampleCount) return;
            double scalar;
            try {
                scalar = extractScalar(event.values, axis);
            } catch (IllegalArgumentException error) {
                failInternal("ERR_AXIS_VALUES", error.getMessage());
                return;
            }
            values[collected] = scalar;
            timestampsNs[collected] = event.timestamp;
            collected++;
            if (collected >= requestedSampleCount) complete(false);
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            // Accuracy is sensor metadata; it does not change the deterministic DFT.
        }

        private void complete(boolean timedOut) {
            if (!finished.compareAndSet(false, true)) return;
            cleanup();
            try {
                double[] actualValues = new double[collected];
                long[] actualTimestamps = new long[collected];
                System.arraycopy(values, 0, actualValues, 0, collected);
                System.arraycopy(timestampsNs, 0, actualTimestamps, 0, collected);
                RafSpectralAnalyzer.Result result = RafSpectralAnalyzer.analyze(actualValues, actualTimestamps, window);
                JSONObject payload = buildPayload(result, timedOut);
                Intent completed = baseResult(RafSpectralContract.STATUS_COMPLETED, requestId);
                completed.putExtra(RafSpectralContract.RESULT_SPECTRUM_JSON, payload.toString());
                completed.putExtra(RafSpectralContract.RESULT_MESSAGE,
                    timedOut ? "Partial spectral window analyzed" : "Spectral window analyzed");
                dispatch(callback, completed);
            } catch (Throwable error) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Spectral analysis failed", error);
                sendFailure(callback, requestId, "ERR_SPECTRAL_ANALYSIS",
                    error.getClass().getSimpleName() + ": " + error.getMessage());
            }
            activeRequests.remove(requestId);
            stopIfIdle();
        }

        private JSONObject buildPayload(RafSpectralAnalyzer.Result result, boolean timedOut) throws JSONException {
            JSONObject payload = new JSONObject();
            payload.put("schema", "raf-sensor-spectrum/v1");
            payload.put("protocol_version", RafSpectralContract.PROTOCOL_VERSION);
            payload.put("request_id", requestId);
            payload.put("client_package", clientPackage);
            payload.put("sensor_name", sensorName);
            payload.put("sensor_type", sensor.getType());
            payload.put("sensor_vendor", sensor.getVendor());
            payload.put("axis", axis);
            payload.put("window", result.window);
            payload.put("requested_sample_count", requestedSampleCount);
            payload.put("sample_count", result.sampleCount);
            payload.put("timed_out", timedOut);
            payload.put("requested_sampling_period_us", samplingPeriodUs);
            payload.put("first_timestamp_ns", result.firstTimestampNs);
            payload.put("last_timestamp_ns", result.lastTimestampNs);
            payload.put("effective_sample_rate_hz", result.sampleRateHz);
            payload.put("frequency_resolution_hz", result.frequencyResolutionHz);
            payload.put("timing_jitter_ratio", result.timingJitterRatio);
            payload.put("mean", result.mean);
            payload.put("rms_detrended", result.rms);
            payload.put("dominant_frequency_hz", result.dominantFrequencyHz);
            payload.put("dominant_power", result.dominantPower);
            payload.put("spectral_centroid_hz", result.spectralCentroidHz);
            payload.put("total_non_dc_power", result.totalNonDcPower);
            payload.put("flat_signal", result.flatSignal);
            payload.put("quality_state", result.qualityState);
            payload.put("uniform_sampling_assumption",
                result.timingJitterRatio <= RafSpectralAnalyzer.DEFAULT_MAX_JITTER_RATIO);
            payload.put("raw_samples_included", false);
            payload.put("claim_allowed", false);
            payload.put("interpretation",
                "Computational periodogram only; dominant frequency does not establish physical causality.");

            JSONArray frequencies = new JSONArray();
            JSONArray powers = new JSONArray();
            for (int index = 0; index < result.frequenciesHz.length; index++) {
                frequencies.put(result.frequenciesHz[index]);
                powers.put(result.powers[index]);
            }
            payload.put("frequencies_hz", frequencies);
            payload.put("power", powers);
            return payload;
        }

        private void cancelInternal(String code, String message) {
            if (!finished.compareAndSet(false, true)) return;
            cleanup();
            Intent cancelled = baseResult(RafSpectralContract.STATUS_CANCELLED, requestId);
            cancelled.putExtra(RafSpectralContract.RESULT_ERROR_CODE, code);
            cancelled.putExtra(RafSpectralContract.RESULT_MESSAGE, message);
            dispatch(callback, cancelled);
            activeRequests.remove(requestId);
            stopIfIdle();
        }

        private void failInternal(String code, String message) {
            if (!finished.compareAndSet(false, true)) return;
            cleanup();
            sendFailure(callback, requestId, code, message);
            activeRequests.remove(requestId);
            stopIfIdle();
        }

        private void cleanup() {
            if (sensorManager != null) sensorManager.unregisterListener(this);
            if (workerHandler != null) workerHandler.removeCallbacks(timeoutRunnable);
        }
    }

    static double extractScalar(float[] sensorValues, String axis) {
        if (sensorValues == null || sensorValues.length == 0) {
            throw new IllegalArgumentException("Sensor event has no values");
        }
        int axisIndex = RafSpectralContract.axisIndex(axis);
        if (axisIndex >= 0) {
            if (axisIndex >= sensorValues.length) {
                throw new IllegalArgumentException("Requested axis is not available for this sensor");
            }
            return sensorValues[axisIndex];
        }
        double sumSquares = 0.0d;
        for (float value : sensorValues) {
            if (!Float.isFinite(value)) {
                throw new IllegalArgumentException("Sensor value is not finite");
            }
            sumSquares += ((double) value) * value;
        }
        return Math.sqrt(sumSquares);
    }

    private Intent baseResult(String status, String requestId) {
        Intent result = new Intent();
        result.putExtra(RafSpectralContract.RESULT_STATUS, status);
        result.putExtra(RafSpectralContract.RESULT_REQUEST_ID, requestId);
        return result;
    }

    private void sendFailure(PendingIntent callback, String requestId, String code, String message) {
        if (callback == null) return;
        Intent failure = baseResult(RafSpectralContract.STATUS_FAILED, requestId);
        failure.putExtra(RafSpectralContract.RESULT_ERROR_CODE, code == null ? "ERR_UNKNOWN" : code);
        failure.putExtra(RafSpectralContract.RESULT_MESSAGE, message == null ? "Unknown spectral failure" : message);
        dispatch(callback, failure);
    }

    private void dispatch(PendingIntent callback, Intent result) {
        if (callback == null) return;
        try {
            callback.send(this, 0, result);
        } catch (PendingIntent.CanceledException error) {
            Logger.logWarn(LOG_TAG, "Callback delivery failed: " + error.getMessage());
        }
    }

    private void startAsForegroundService() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;
        NotificationUtils.setupNotificationChannel(
            this,
            NOTIFICATION_CHANNEL_ID,
            NOTIFICATION_CHANNEL_NAME,
            NotificationManager.IMPORTANCE_LOW
        );
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

    private void stopIfIdle() {
        if (!activeRequests.isEmpty()) return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) stopForeground(true);
        stopSelf();
    }
}
