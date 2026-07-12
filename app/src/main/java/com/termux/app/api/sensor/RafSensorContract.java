package com.termux.app.api.sensor;

import com.termux.rafacodephi.BuildConfig;

import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

public final class RafSensorContract {

    public static final int PROTOCOL_VERSION_1 = 1;

    public static final String PERMISSION = BuildConfig.APPLICATION_ID + ".permission.RAF_SENSOR_ACCESS";
    public static final String ACTION_SENSOR_SNAPSHOT = BuildConfig.APPLICATION_ID + ".action.RAF_SENSOR_SNAPSHOT";
    public static final String ACTION_CANCEL_SENSOR_REQUEST = BuildConfig.APPLICATION_ID + ".action.RAF_SENSOR_CANCEL";

    public static final String EXTRA_PROTOCOL_VERSION = "protocol_version";
    public static final String EXTRA_REQUEST_ID = "request_id";
    public static final String EXTRA_SENSOR_NAME = "sensor_name";
    public static final String EXTRA_SAMPLING_PERIOD_US = "sampling_period_us";
    public static final String EXTRA_MAX_REPORT_LATENCY_US = "max_report_latency_us";
    public static final String EXTRA_CALLBACK = "callback";
    public static final String EXTRA_CLIENT_PACKAGE = "client_package";

    public static final String RESULT_STATUS = "status";
    public static final String RESULT_ERROR_CODE = "error_code";
    public static final String RESULT_MESSAGE = "message";
    public static final String RESULT_PROTOCOL_VERSION = "protocol_version";
    public static final String RESULT_REQUEST_ID = "request_id";
    public static final String RESULT_SENSOR_NAME = "sensor_name";
    public static final String RESULT_SENSOR_TYPE = "sensor_type";
    public static final String RESULT_SENSOR_VENDOR = "sensor_vendor";
    public static final String RESULT_SENSOR_VERSION = "sensor_version";
    public static final String RESULT_TIMESTAMP_NS = "timestamp_ns";
    public static final String RESULT_ACCURACY = "accuracy";
    public static final String RESULT_VALUES = "values";
    public static final String RESULT_HARDWARE_ABI = "hardware_abi";
    public static final String RESULT_HARDWARE_FLAGS = "hardware_flags";
    public static final String RESULT_PAGE_SIZE = "page_size";
    public static final String RESULT_CACHE_LINE = "cache_line";
    public static final String RESULT_CPUS_ONLINE = "cpus_online";

    public static final String STATUS_ACCEPTED = "ACCEPTED";
    public static final String STATUS_SAMPLING = "SAMPLING";
    public static final String STATUS_COMPLETED = "COMPLETED";
    public static final String STATUS_CANCELLED = "CANCELLED";
    public static final String STATUS_FAILED = "FAILED";

    public static final String SENSOR_ACCELEROMETER = "accelerometer";
    public static final String SENSOR_GYROSCOPE = "gyroscope";
    public static final String SENSOR_MAGNETOMETER = "magnetometer";
    public static final String SENSOR_LIGHT = "light";
    public static final String SENSOR_PROXIMITY = "proximity";
    public static final String SENSOR_PRESSURE = "pressure";
    public static final String SENSOR_GRAVITY = "gravity";
    public static final String SENSOR_ROTATION_VECTOR = "rotation_vector";

    public static final int DEFAULT_SAMPLING_PERIOD_US = 5_000;
    public static final int DEFAULT_MAX_REPORT_LATENCY_US = 0;
    public static final int MAX_SAMPLING_PERIOD_US = 1_000_000;
    public static final int MAX_REPORT_LATENCY_US = 5_000_000;
    public static final int MAX_REQUEST_ID_LENGTH = 64;
    public static final int MAX_CLIENT_PACKAGE_LENGTH = 200;

    private static final Set<String> ALLOWED_SENSOR_NAMES = Collections.unmodifiableSet(
        new LinkedHashSet<>(Arrays.asList(
            SENSOR_ACCELEROMETER,
            SENSOR_GYROSCOPE,
            SENSOR_MAGNETOMETER,
            SENSOR_LIGHT,
            SENSOR_PROXIMITY,
            SENSOR_PRESSURE,
            SENSOR_GRAVITY,
            SENSOR_ROTATION_VECTOR
        )));

    private static final Map<String, Integer> SAMPLING_PRESETS_US;

    static {
        LinkedHashMap<String, Integer> presets = new LinkedHashMap<>();
        presets.put("FASTEST", 5_000);
        presets.put("GAME", 20_000);
        presets.put("UI", 66_667);
        presets.put("NORMAL", 200_000);
        SAMPLING_PRESETS_US = Collections.unmodifiableMap(presets);
    }

    private RafSensorContract() {}

    public static Set<String> allowedSensorNames() {
        return ALLOWED_SENSOR_NAMES;
    }

    public static Map<String, Integer> samplingPresetsUs() {
        return SAMPLING_PRESETS_US;
    }

    public static int normalizeSamplingPeriodUs(int samplingPeriodUs) {
        if (samplingPeriodUs <= 0) {
            return DEFAULT_SAMPLING_PERIOD_US;
        }
        return samplingPeriodUs;
    }

    public static int normalizeMaxReportLatencyUs(int maxReportLatencyUs) {
        if (maxReportLatencyUs < 0) {
            return DEFAULT_MAX_REPORT_LATENCY_US;
        }
        return maxReportLatencyUs;
    }

    public static boolean isAllowedSensorName(String sensorName) {
        return sensorName != null && ALLOWED_SENSOR_NAMES.contains(sensorName);
    }

    public static ValidationResult validateSnapshotRequest(int protocolVersion,
                                                           String requestId,
                                                           String sensorName,
                                                           int samplingPeriodUs,
                                                           int maxReportLatencyUs,
                                                           boolean hasCallback,
                                                           String clientPackage) {
        if (protocolVersion != PROTOCOL_VERSION_1) {
            return ValidationResult.error("ERR_PROTOCOL_VERSION", "Unsupported protocol version");
        }

        if (!isSafeToken(requestId, MAX_REQUEST_ID_LENGTH)) {
            return ValidationResult.error("ERR_REQUEST_ID", "Request id must be 1-64 chars of [A-Za-z0-9._:-]");
        }

        if (!isAllowedSensorName(sensorName)) {
            return ValidationResult.error("ERR_SENSOR_NAME", "Unsupported sensor name");
        }

        int normalizedSampling = normalizeSamplingPeriodUs(samplingPeriodUs);
        if (normalizedSampling <= 0 || normalizedSampling > MAX_SAMPLING_PERIOD_US) {
            return ValidationResult.error("ERR_SAMPLING_PERIOD", "Sampling period is out of range");
        }

        int normalizedLatency = normalizeMaxReportLatencyUs(maxReportLatencyUs);
        if (normalizedLatency < 0 || normalizedLatency > MAX_REPORT_LATENCY_US) {
            return ValidationResult.error("ERR_REPORT_LATENCY", "Max report latency is out of range");
        }

        if (!hasCallback) {
            return ValidationResult.error("ERR_CALLBACK", "PendingIntent callback is required");
        }

        if (!isPackageNameLike(clientPackage)) {
            return ValidationResult.error("ERR_CLIENT_PACKAGE", "Client package is missing or invalid");
        }

        return ValidationResult.ok();
    }

    private static boolean isSafeToken(String value, int maxLength) {
        if (value == null || value.isEmpty() || value.length() > maxLength) return false;
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            boolean safe = (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '.' || c == '_' || c == ':' || c == '-';
            if (!safe) return false;
        }
        return true;
    }

    private static boolean isPackageNameLike(String clientPackage) {
        if (clientPackage == null || clientPackage.isEmpty() || clientPackage.length() > MAX_CLIENT_PACKAGE_LENGTH) {
            return false;
        }
        boolean hasDot = false;
        for (int i = 0; i < clientPackage.length(); i++) {
            char c = clientPackage.charAt(i);
            if (c == '.') {
                hasDot = true;
                continue;
            }
            boolean safe = (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '_' || c == '-';
            if (!safe) return false;
        }
        return hasDot;
    }

    public static final class ValidationResult {
        public final boolean valid;
        public final String errorCode;
        public final String message;

        private ValidationResult(boolean valid, String errorCode, String message) {
            this.valid = valid;
            this.errorCode = errorCode;
            this.message = message;
        }

        public static ValidationResult ok() {
            return new ValidationResult(true, null, null);
        }

        public static ValidationResult error(String errorCode, String message) {
            return new ValidationResult(false, errorCode, message);
        }
    }
}
