package com.termux.app.benchmark;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.os.Build;
import android.os.PowerManager;
import android.os.SystemClock;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.Locale;

/**
 * Best-effort user-space environment snapshot for benchmark observations.
 *
 * Missing kernel/sysfs fields remain UNAVAILABLE. Battery temperature is
 * explicitly not promoted to CPU/SoC temperature, and framework current/power
 * metadata is never promoted to measured energy.
 */
public final class BenchmarkEnvironmentSnapshot {

    public static final String SCHEMA = "rafcodephi.benchmark-environment-snapshot/v1";
    private static final int MAX_CPUS = 64;

    private BenchmarkEnvironmentSnapshot() {}

    public static JSONObject capture(Context context) {
        JSONObject out = new JSONObject();
        try {
            out.put("schema", SCHEMA);
            out.put("elapsed_realtime_ns", SystemClock.elapsedRealtimeNanos());
            out.put("uptime_ms", SystemClock.uptimeMillis());
            out.put("available_processors", Runtime.getRuntime().availableProcessors());
            captureThermal(context, out);
            captureBattery(context, out);
            captureMemory(context, out);
            captureCpuFreq(out);
            out.put("claim_allowed_cpu_temperature", false);
            out.put("claim_allowed_energy_measurement", false);
            out.put("claim_allowed_pmu", false);
        } catch (Throwable error) {
            try {
                out.put("snapshot_state", "INVALIDATED");
                out.put("snapshot_error", error.getClass().getSimpleName() + ": " + String.valueOf(error.getMessage()));
            } catch (Throwable ignored) {
            }
            return out;
        }
        try {
            out.put("snapshot_state", "OBSERVED_LIMITED");
        } catch (Throwable ignored) {
        }
        return out;
    }

    public static boolean hasSevereThermalInterference(JSONObject snapshot) {
        if (snapshot == null) return false;
        int status = snapshot.optInt("android_thermal_status", -1);
        return status >= 3; // PowerManager THERMAL_STATUS_SEVERE and above.
    }

    private static void captureThermal(Context context, JSONObject out) throws Exception {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            out.put("android_thermal_status", JSONObject.NULL);
            out.put("android_thermal_state", "UNAVAILABLE_API_LT_29");
            return;
        }
        PowerManager power = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        if (power == null) {
            out.put("android_thermal_status", JSONObject.NULL);
            out.put("android_thermal_state", "UNAVAILABLE_POWER_MANAGER");
            return;
        }
        int status = power.getCurrentThermalStatus();
        out.put("android_thermal_status", status);
        out.put("android_thermal_state", thermalName(status));
    }

    private static void captureBattery(Context context, JSONObject out) throws Exception {
        Intent battery = context.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        if (battery == null) {
            out.put("battery_state", "UNAVAILABLE");
            return;
        }
        int level = battery.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
        int scale = battery.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
        int temperatureTenthsC = battery.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, Integer.MIN_VALUE);
        int voltageMv = battery.getIntExtra(BatteryManager.EXTRA_VOLTAGE, -1);
        int plugged = battery.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
        int status = battery.getIntExtra(BatteryManager.EXTRA_STATUS, -1);
        out.put("battery_state", "FRAMEWORK_OBSERVED");
        out.put("battery_level", level);
        out.put("battery_scale", scale);
        out.put("battery_fraction", level >= 0 && scale > 0 ? ((double) level / (double) scale) : JSONObject.NULL);
        out.put("battery_temperature_c", temperatureTenthsC == Integer.MIN_VALUE
            ? JSONObject.NULL : temperatureTenthsC / 10.0);
        out.put("battery_temperature_scope", "BATTERY_NOT_CPU_SOC");
        out.put("battery_voltage_mv", voltageMv >= 0 ? voltageMv : JSONObject.NULL);
        out.put("battery_plugged", plugged);
        out.put("battery_status", status);
    }

    private static void captureMemory(Context context, JSONObject out) throws Exception {
        ActivityManager manager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        if (manager == null) {
            out.put("memory_state", "UNAVAILABLE");
            return;
        }
        ActivityManager.MemoryInfo info = new ActivityManager.MemoryInfo();
        manager.getMemoryInfo(info);
        out.put("memory_state", "FRAMEWORK_OBSERVED");
        out.put("memory_avail_bytes", info.availMem);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) out.put("memory_total_bytes", info.totalMem);
        out.put("memory_threshold_bytes", info.threshold);
        out.put("memory_low", info.lowMemory);
    }

    private static void captureCpuFreq(JSONObject out) throws Exception {
        JSONArray cpus = new JSONArray();
        int seen = 0;
        int readable = 0;
        for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
            File cpuDir = new File("/sys/devices/system/cpu/cpu" + cpu);
            if (!cpuDir.exists()) {
                if (cpu > 15 && seen == 0) break;
                continue;
            }
            seen++;
            JSONObject row = new JSONObject();
            row.put("cpu", cpu);
            String cur = firstReadable(
                "/sys/devices/system/cpu/cpu" + cpu + "/cpufreq/scaling_cur_freq",
                "/sys/devices/system/cpu/cpu" + cpu + "/cpufreq/cpuinfo_cur_freq");
            String min = readOneLine("/sys/devices/system/cpu/cpu" + cpu + "/cpufreq/scaling_min_freq");
            String max = readOneLine("/sys/devices/system/cpu/cpu" + cpu + "/cpufreq/scaling_max_freq");
            String governor = readOneLine("/sys/devices/system/cpu/cpu" + cpu + "/cpufreq/scaling_governor");
            if (cur != null) {
                readable++;
                row.put("current_khz", parseLongOrString(cur));
            } else row.put("current_khz", JSONObject.NULL);
            row.put("min_khz", min == null ? JSONObject.NULL : parseLongOrString(min));
            row.put("max_khz", max == null ? JSONObject.NULL : parseLongOrString(max));
            row.put("governor", governor == null ? JSONObject.NULL : governor);
            cpus.put(row);
        }
        out.put("cpu_frequency_samples", cpus);
        out.put("cpu_frequency_visible_cpus", seen);
        out.put("cpu_frequency_readable_cpus", readable);
        out.put("cpu_frequency_state", readable == 0 ? "UNAVAILABLE" : readable == seen ? "AVAILABLE" : "PARTIAL");
        out.put("cpu_frequency_scope", "SYSFS_OBSERVATION_NOT_FREQUENCY_LOCK");
    }

    private static Object parseLongOrString(String value) {
        try {
            return Long.parseLong(value.trim());
        } catch (Throwable ignored) {
            return value.trim();
        }
    }

    private static String firstReadable(String... paths) {
        for (String path : paths) {
            String value = readOneLine(path);
            if (value != null) return value;
        }
        return null;
    }

    private static String readOneLine(String path) {
        File file = new File(path);
        if (!file.isFile() || !file.canRead()) return null;
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line = reader.readLine();
            return line == null || line.trim().isEmpty() ? null : line.trim();
        } catch (Throwable ignored) {
            return null;
        }
    }

    private static String thermalName(int status) {
        switch (status) {
            case 0: return "NONE";
            case 1: return "LIGHT";
            case 2: return "MODERATE";
            case 3: return "SEVERE";
            case 4: return "CRITICAL";
            case 5: return "EMERGENCY";
            case 6: return "SHUTDOWN";
            default: return String.format(Locale.US, "UNKNOWN_%d", status);
        }
    }
}
