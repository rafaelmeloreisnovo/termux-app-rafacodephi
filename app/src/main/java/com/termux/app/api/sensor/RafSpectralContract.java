package com.termux.app.api.sensor;

import com.termux.rafacodephi.BuildConfig;

public final class RafSpectralContract {

    public static final int PROTOCOL_VERSION = 3;

    public static final String ACTION_SENSOR_SPECTRUM =
        BuildConfig.APPLICATION_ID + ".action.RAF_SENSOR_SPECTRUM";

    public static final String EXTRA_PROTOCOL_VERSION = "protocol_version";
    public static final String EXTRA_REQUEST_ID = "request_id";
    public static final String EXTRA_SENSOR_NAME = "sensor_name";
    public static final String EXTRA_AXIS = "spectral_axis";
    public static final String EXTRA_SAMPLE_COUNT = "sample_count";
    public static final String EXTRA_SAMPLING_PERIOD_US = "sampling_period_us";
    public static final String EXTRA_TIMEOUT_MS = "timeout_ms";
    public static final String EXTRA_WINDOW = "window";
    public static final String EXTRA_CALLBACK = "callback";
    public static final String EXTRA_CLIENT_PACKAGE = "client_package";

    public static final String RESULT_STATUS = "status";
    public static final String RESULT_ERROR_CODE = "error_code";
    public static final String RESULT_MESSAGE = "message";
    public static final String RESULT_REQUEST_ID = "request_id";
    public static final String RESULT_SPECTRUM_JSON = "spectrum_json";

    public static final String STATUS_ACCEPTED = "ACCEPTED";
    public static final String STATUS_SAMPLING = "SAMPLING";
    public static final String STATUS_COMPLETED = "COMPLETED";
    public static final String STATUS_CANCELLED = "CANCELLED";
    public static final String STATUS_FAILED = "FAILED";

    public static final String AXIS_MAGNITUDE = "magnitude";
    public static final String AXIS_X = "x";
    public static final String AXIS_Y = "y";
    public static final String AXIS_Z = "z";
    public static final String AXIS_W = "w";

    public static final int DEFAULT_SAMPLE_COUNT = 128;
    public static final int DEFAULT_SAMPLING_PERIOD_US = 20_000;
    public static final int MIN_SAMPLING_PERIOD_US = 5_000;
    public static final int MAX_SAMPLING_PERIOD_US = 200_000;
    public static final int DEFAULT_TIMEOUT_MS = 5_000;
    public static final int MIN_TIMEOUT_MS = 1_000;
    public static final int MAX_TIMEOUT_MS = 30_000;
    public static final int MAX_REQUEST_ID_LENGTH = 64;
    public static final int MAX_CLIENT_PACKAGE_LENGTH = 200;

    private RafSpectralContract() {}

    public static ValidationResult validateRequest(int protocolVersion,
                                                   String requestId,
                                                   String sensorName,
                                                   String axis,
                                                   int sampleCount,
                                                   int samplingPeriodUs,
                                                   int timeoutMs,
                                                   String window,
                                                   boolean hasCallback,
                                                   String clientPackage) {
        if (protocolVersion != PROTOCOL_VERSION) {
            return ValidationResult.error("ERR_PROTOCOL_VERSION", "Spectral analysis requires protocol version 3");
        }
        if (!isSafeToken(requestId, MAX_REQUEST_ID_LENGTH)) {
            return ValidationResult.error("ERR_REQUEST_ID", "Request id must be 1-64 chars of [A-Za-z0-9._:-]");
        }
        if (!RafSensorContract.isAllowedSensorName(sensorName)) {
            return ValidationResult.error("ERR_SENSOR_NAME", "Unsupported sensor name");
        }
        if (!isAllowedAxis(axis)) {
            return ValidationResult.error("ERR_AXIS", "Axis must be magnitude, x, y, z or w");
        }
        if (sampleCount < RafSpectralAnalyzer.MIN_SAMPLES || sampleCount > RafSpectralAnalyzer.MAX_SAMPLES) {
            return ValidationResult.error("ERR_SAMPLE_COUNT", "Sample count must be between 16 and 512");
        }
        if (samplingPeriodUs < MIN_SAMPLING_PERIOD_US || samplingPeriodUs > MAX_SAMPLING_PERIOD_US) {
            return ValidationResult.error("ERR_SAMPLING_PERIOD", "Sampling period is out of range");
        }
        if (timeoutMs < MIN_TIMEOUT_MS || timeoutMs > MAX_TIMEOUT_MS) {
            return ValidationResult.error("ERR_TIMEOUT", "Timeout is out of range");
        }
        long nominalDurationMs = ((long) (sampleCount - 1) * samplingPeriodUs) / 1_000L;
        if (nominalDurationMs > timeoutMs) {
            return ValidationResult.error("ERR_TIMEOUT_BUDGET", "Timeout is shorter than nominal sample window");
        }
        try {
            RafSpectralAnalyzer.normalizeWindow(window);
        } catch (IllegalArgumentException error) {
            return ValidationResult.error("ERR_WINDOW", error.getMessage());
        }
        if (!hasCallback) {
            return ValidationResult.error("ERR_CALLBACK", "PendingIntent callback is required");
        }
        if (!isPackageNameLike(clientPackage)) {
            return ValidationResult.error("ERR_CLIENT_PACKAGE", "Client package is missing or invalid");
        }
        return ValidationResult.ok();
    }

    public static boolean callbackBelongsToClient(String callbackCreatorPackage, String clientPackage) {
        return callbackCreatorPackage != null && callbackCreatorPackage.equals(clientPackage);
    }

    public static boolean isAllowedAxis(String axis) {
        return AXIS_MAGNITUDE.equals(axis) || AXIS_X.equals(axis) || AXIS_Y.equals(axis) ||
            AXIS_Z.equals(axis) || AXIS_W.equals(axis);
    }

    public static int axisIndex(String axis) {
        if (AXIS_X.equals(axis)) return 0;
        if (AXIS_Y.equals(axis)) return 1;
        if (AXIS_Z.equals(axis)) return 2;
        if (AXIS_W.equals(axis)) return 3;
        return -1;
    }

    private static boolean isSafeToken(String value, int maxLength) {
        if (value == null || value.isEmpty() || value.length() > maxLength) return false;
        for (int index = 0; index < value.length(); index++) {
            char c = value.charAt(index);
            boolean safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '.' || c == '_' || c == ':' || c == '-';
            if (!safe) return false;
        }
        return true;
    }

    private static boolean isPackageNameLike(String value) {
        if (value == null || value.isEmpty() || value.length() > MAX_CLIENT_PACKAGE_LENGTH) return false;
        boolean hasDot = false;
        for (int index = 0; index < value.length(); index++) {
            char c = value.charAt(index);
            if (c == '.') {
                hasDot = true;
                continue;
            }
            boolean safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_' || c == '-';
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

        public static ValidationResult error(String code, String message) {
            return new ValidationResult(false, code, message);
        }
    }
}
