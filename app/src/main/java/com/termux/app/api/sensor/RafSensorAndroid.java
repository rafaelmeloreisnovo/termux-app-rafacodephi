package com.termux.app.api.sensor;

import android.hardware.Sensor;

public final class RafSensorAndroid {

    private RafSensorAndroid() {}

    public static int toSensorType(String sensorName) {
        if (RafSensorContract.SENSOR_ACCELEROMETER.equals(sensorName)) return Sensor.TYPE_ACCELEROMETER;
        if (RafSensorContract.SENSOR_GYROSCOPE.equals(sensorName)) return Sensor.TYPE_GYROSCOPE;
        if (RafSensorContract.SENSOR_MAGNETOMETER.equals(sensorName)) return Sensor.TYPE_MAGNETIC_FIELD;
        if (RafSensorContract.SENSOR_LIGHT.equals(sensorName)) return Sensor.TYPE_LIGHT;
        if (RafSensorContract.SENSOR_PROXIMITY.equals(sensorName)) return Sensor.TYPE_PROXIMITY;
        if (RafSensorContract.SENSOR_PRESSURE.equals(sensorName)) return Sensor.TYPE_PRESSURE;
        if (RafSensorContract.SENSOR_GRAVITY.equals(sensorName)) return Sensor.TYPE_GRAVITY;
        if (RafSensorContract.SENSOR_ROTATION_VECTOR.equals(sensorName)) return Sensor.TYPE_ROTATION_VECTOR;
        return -1;
    }

    public static String reportingModeToString(Sensor sensor) {
        if (sensor == null) return "unknown";
        switch (sensor.getReportingMode()) {
            case Sensor.REPORTING_MODE_CONTINUOUS:
                return "continuous";
            case Sensor.REPORTING_MODE_ON_CHANGE:
                return "on_change";
            case Sensor.REPORTING_MODE_ONE_SHOT:
                return "one_shot";
            case Sensor.REPORTING_MODE_SPECIAL_TRIGGER:
                return "special_trigger";
            default:
                return "unknown";
        }
    }
}
