package com.termux.app.orchestration;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Build;

import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;

/** Read-only runtime snapshot used by the RAFCODEΦ control center and evidence export. */
public final class ControlCenterSnapshot {

    private ControlCenterSnapshot() {}

    public static String render(Context context) {
        File prefix = new File(context.getFilesDir(), "usr");
        StringBuilder out = new StringBuilder();
        out.append("RAFCODEPHI_CONTROL_CENTER_SNAPSHOT_V1\n");
        out.append("package=").append(context.getPackageName()).append('\n');
        out.append("supported_abis=").append(Arrays.toString(Build.SUPPORTED_ABIS)).append('\n');
        out.append("runtime_prefix=").append(prefix.getAbsolutePath()).append('\n');
        out.append("\n[PACKAGE_RUNTIME]\n");

        String[] tools = {"sh", "pkg", "apt", "apt-get", "dpkg", "bash", "busybox", "proot"};
        boolean realRuntime = true;
        for (String tool : tools) {
            File file = new File(prefix, "bin/" + tool);
            boolean present = file.isFile();
            boolean executable = present && file.canExecute();
            boolean requireElf = !"sh".equals(tool) && !"pkg".equals(tool);
            boolean elf = present && isElf(file);
            String state = present && executable && (!requireElf || elf) ? "PASS" : "BLOCKED";
            if ("BLOCKED".equals(state)) realRuntime = false;
            out.append(tool).append('=').append(state)
                .append(" present=").append(present)
                .append(" executable=").append(executable);
            if (requireElf) out.append(" elf=").append(elf);
            out.append('\n');
        }

        File dpkgStatus = new File(prefix, "var/lib/dpkg/status");
        boolean dpkgReady = dpkgStatus.isFile() && dpkgStatus.length() > 0;
        if (!dpkgReady) realRuntime = false;
        out.append("dpkg_status=").append(dpkgReady ? "PASS" : "BLOCKED")
            .append(" bytes=").append(dpkgStatus.isFile() ? dpkgStatus.length() : 0).append('\n');

        File sourcesD = new File(prefix, "etc/apt/sources.list.d");
        File sourceList = new File(prefix, "etc/apt/sources.list");
        boolean aptSource = sourceList.isFile() || hasRegularFile(sourcesD);
        if (!aptSource) realRuntime = false;
        out.append("apt_source_definition=").append(aptSource ? "PASS" : "BLOCKED").append('\n');

        File profileFile = new File(prefix, "BOOTSTRAP_PROFILE.json");
        String profileState = "TOKEN_VAZIO";
        String repoState = "TOKEN_VAZIO";
        String repoUrl = "TOKEN_VAZIO";
        if (profileFile.isFile()) {
            try {
                JSONObject profile = new JSONObject(readUtf8(profileFile));
                boolean realPkg = "real-pkg".equals(profile.optString("profile"))
                    && "real-pkg".equals(profile.optString("package_layer"));
                profileState = realPkg ? "PASS" : "BLOCKED";
                repoState = profile.optString("package_repo_runtime_state", "TOKEN_VAZIO");
                repoUrl = profile.optString("apt_repository_url", "TOKEN_VAZIO");
                if (!realPkg) realRuntime = false;
            } catch (Throwable error) {
                profileState = "BLOCKED:" + error.getClass().getSimpleName();
                realRuntime = false;
            }
        } else {
            realRuntime = false;
        }
        out.append("bootstrap_profile=").append(profileState).append('\n');
        out.append("package_repo_runtime_state=").append(repoState).append('\n');
        out.append("apt_repository_url=").append(repoUrl).append('\n');
        out.append("package_runtime_gate=").append(realRuntime ? "PASS" : "BLOCKED").append('\n');
        out.append("claim_allowed_release=false\n");
        out.append("device_runtime_proof=").append(realRuntime ? "OBSERVED_LOCAL_STRUCTURE_ONLY" : "TOKEN_VAZIO").append('\n');

        out.append("\n[VECTRA_RUNTIME]\n");
        SensorManager manager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        if (manager == null) {
            out.append("sensor_manager=UNAVAILABLE\n");
        } else {
            appendSensor(out, manager, "accelerometer", Sensor.TYPE_ACCELEROMETER);
            appendSensor(out, manager, "gyroscope", Sensor.TYPE_GYROSCOPE);
            appendSensor(out, manager, "magnetometer", Sensor.TYPE_MAGNETIC_FIELD);
            appendSensor(out, manager, "light", Sensor.TYPE_LIGHT);
            appendSensor(out, manager, "proximity", Sensor.TYPE_PROXIMITY);
            out.append("framework_sensor_count=").append(manager.getSensorList(Sensor.TYPE_ALL).size()).append('\n');
        }
        out.append("vectra_scope=INTERNAL_TERMUX_RAFCODEPHI\n");
        out.append("evidence_boundary=requested_sampling_parameters_are_not_measured_latency_or_throughput\n");
        return out.toString();
    }

    private static void appendSensor(StringBuilder out, SensorManager manager, String name, int type) {
        Sensor sensor = manager.getDefaultSensor(type);
        if (sensor == null) {
            out.append(name).append("=UNAVAILABLE\n");
            return;
        }
        out.append(name).append("=PASS name=").append(safe(sensor.getName()))
            .append(" vendor=").append(safe(sensor.getVendor()))
            .append(" minDelayUs=").append(sensor.getMinDelay()).append('\n');
    }

    private static boolean isElf(File file) {
        try (FileInputStream in = new FileInputStream(file)) {
            byte[] magic = new byte[4];
            return in.read(magic) == 4
                && magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F';
        } catch (Throwable ignored) {
            return false;
        }
    }

    private static boolean hasRegularFile(File dir) {
        if (!dir.isDirectory()) return false;
        File[] files = dir.listFiles();
        if (files == null) return false;
        for (File file : files) if (file.isFile() && file.length() > 0) return true;
        return false;
    }

    private static String readUtf8(File file) throws Exception {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            return new String(Files.readAllBytes(file.toPath()), StandardCharsets.UTF_8);
        }
        try (FileInputStream in = new FileInputStream(file)) {
            byte[] data = new byte[(int) file.length()];
            int offset = 0;
            while (offset < data.length) {
                int read = in.read(data, offset, data.length - offset);
                if (read < 0) break;
                offset += read;
            }
            return new String(data, 0, offset, StandardCharsets.UTF_8);
        }
    }

    private static String safe(String value) {
        return value == null ? "UNAVAILABLE" : value.replace('\n', ' ').replace('\r', ' ');
    }
}
