package com.termux.app.rafaelia;

import android.app.Activity;
import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemClock;
import android.system.Os;
import android.system.OsConstants;
import android.util.Log;

import com.termux.rafacodephi.BuildConfig;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

/**
 * Debug-only physical-device probe for the RAFAELIA ZERO RFZ1 runtime.
 *
 * The activity has no UI. It runs one deterministic native ingest, checks the
 * process ABI against the compiled kernel, writes an atomic JSON receipt to
 * files/rafaelia-zero/latest.json and exits. It is guarded by android.permission.DUMP
 * in the debug manifest, so adb shell can invoke it while ordinary apps cannot.
 */
public final class RafaeliaZeroProbeActivity extends Activity {
    private static final String TAG = "RafaeliaZeroProbe";
    private static final byte[] PAYLOAD = new byte[] {
        0x52, 0x46, 0x5A, 0x31, 0x2D, 0x44, 0x45, 0x56,
        0x49, 0x43, 0x45, 0x2D, 0x50, 0x52, 0x4F, 0x42,
        0x45, 0x2D, 0x56, 0x31
    };
    private static final long SOURCE = 0x52465A3150524F42L; // RFZ1PROB
    private static final int FLAGS = 0x44565031; // DVP1

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        String result;
        try {
            result = runProbe();
        } catch (Throwable t) {
            Log.e(TAG, "probe crashed", t);
            result = failureReceipt(t);
        }

        try {
            File receipt = writeReceipt(result);
            Log.i(TAG, "RAFAELIA_ZERO_DEVICE_PROBE path=" + receipt.getAbsolutePath());
        } catch (Throwable t) {
            Log.e(TAG, "receipt write failed", t);
        }
        finish();
    }

    private String runProbe() throws Exception {
        final long timestampMs = System.currentTimeMillis();
        final long sequence = SystemClock.elapsedRealtimeNanos();
        final String processArch = String.valueOf(System.getProperty("os.arch", "unknown"));
        final int expectedArch = expectedArchitecture(processArch);
        final long pageSize = Os.sysconf(OsConstants._SC_PAGESIZE);
        final boolean debuggable = BuildConfig.DEBUG
            && (getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;

        final int initStatus = RafaeliaZeroRuntime.init();
        final boolean available = RafaeliaZeroRuntime.isAvailable();
        final int nativeArch = RafaeliaZeroRuntime.architectureId();
        final int maxPayload = RafaeliaZeroRuntime.MAX_PAYLOAD;
        final int acceptedBefore = RafaeliaZeroRuntime.acceptedCount();
        final int rejectedBefore = RafaeliaZeroRuntime.rejectedCount();
        final int digestBefore = RafaeliaZeroRuntime.stateDigest();

        ByteBuffer direct = ByteBuffer.allocateDirect(PAYLOAD.length);
        direct.put(PAYLOAD);
        direct.flip();

        final int ingestStatus = RafaeliaZeroRuntime.ingestDirect(
            direct, PAYLOAD.length, SOURCE, sequence, FLAGS);
        final int nullGuardStatus = RafaeliaZeroRuntime.ingestDirect(
            null, 0, SOURCE, sequence, FLAGS);
        final int rangeGuardStatus = RafaeliaZeroRuntime.ingestDirect(
            direct, RafaeliaZeroRuntime.MAX_PAYLOAD + 1, SOURCE, sequence, FLAGS);

        final int acceptedAfter = RafaeliaZeroRuntime.acceptedCount();
        final int rejectedAfter = RafaeliaZeroRuntime.rejectedCount();
        final int digestAfter = RafaeliaZeroRuntime.stateDigest();

        final boolean initOk = initStatus == RafaeliaZeroRuntime.OK;
        final boolean archMatch = expectedArch != 0 && nativeArch == expectedArch;
        final boolean maxPayloadMatch = maxPayload == 1024;
        final boolean ingestOk = ingestStatus == RafaeliaZeroRuntime.OK;
        final boolean acceptedIncrement = acceptedAfter == acceptedBefore + 1;
        final boolean rejectedStable = rejectedAfter == rejectedBefore;
        final boolean digestChanged = digestAfter != digestBefore;
        final boolean digestNonzero = digestAfter != 0;
        final boolean nullGuard = nullGuardStatus == RafaeliaZeroRuntime.E_NULL;
        final boolean rangeGuard = rangeGuardStatus == RafaeliaZeroRuntime.E_RANGE;
        final boolean pageSizeValid = pageSize > 0 && (pageSize & (pageSize - 1)) == 0;

        final boolean pass = debuggable && available && initOk && archMatch
            && maxPayloadMatch && ingestOk && acceptedIncrement && rejectedStable
            && digestChanged && digestNonzero && nullGuard && rangeGuard
            && pageSizeValid;

        StringBuilder json = new StringBuilder(2048);
        json.append("{\n");
        field(json, "schema", "rafaelia.zero.device.probe.v1", true);
        field(json, "result", pass ? "PASS" : "FAIL", true);
        boolField(json, "claim_allowed_device", pass, true);
        numberField(json, "timestamp_unix_ms", timestampMs, true);
        field(json, "package", getPackageName(), true);
        boolField(json, "debuggable", debuggable, true);
        json.append("  \"device\": {\n");
        field(json, "manufacturer", Build.MANUFACTURER, true, 4);
        field(json, "model", Build.MODEL, true, 4);
        field(json, "device", Build.DEVICE, true, 4);
        field(json, "fingerprint", Build.FINGERPRINT, true, 4);
        numberField(json, "sdk_int", Build.VERSION.SDK_INT, true, 4);
        field(json, "process_arch", processArch, true, 4);
        field(json, "supported_abis", join(Build.SUPPORTED_ABIS), true, 4);
        numberField(json, "page_size", pageSize, false, 4);
        json.append("  },\n");
        json.append("  \"native\": {\n");
        numberField(json, "init_status", initStatus, true, 4);
        boolField(json, "available", available, true, 4);
        numberField(json, "architecture_id", nativeArch, true, 4);
        numberField(json, "expected_architecture_id", expectedArch, true, 4);
        numberField(json, "max_payload", maxPayload, true, 4);
        numberField(json, "ingest_status", ingestStatus, true, 4);
        numberField(json, "null_guard_status", nullGuardStatus, true, 4);
        numberField(json, "range_guard_status", rangeGuardStatus, false, 4);
        json.append("  },\n");
        json.append("  \"observed\": {\n");
        numberField(json, "payload_bytes", PAYLOAD.length, true, 4);
        numberField(json, "source", SOURCE, true, 4);
        numberField(json, "sequence", sequence, true, 4);
        numberField(json, "accepted_before", acceptedBefore, true, 4);
        numberField(json, "accepted_after", acceptedAfter, true, 4);
        numberField(json, "rejected_before", rejectedBefore, true, 4);
        numberField(json, "rejected_after", rejectedAfter, true, 4);
        numberField(json, "digest_before", Integer.toUnsignedLong(digestBefore), true, 4);
        numberField(json, "digest_after", Integer.toUnsignedLong(digestAfter), false, 4);
        json.append("  },\n");
        json.append("  \"checks\": {\n");
        boolField(json, "debuggable", debuggable, true, 4);
        boolField(json, "library_available", available, true, 4);
        boolField(json, "init_ok", initOk, true, 4);
        boolField(json, "architecture_match", archMatch, true, 4);
        boolField(json, "max_payload_match", maxPayloadMatch, true, 4);
        boolField(json, "ingest_ok", ingestOk, true, 4);
        boolField(json, "accepted_increment", acceptedIncrement, true, 4);
        boolField(json, "rejected_stable", rejectedStable, true, 4);
        boolField(json, "digest_changed", digestChanged, true, 4);
        boolField(json, "digest_nonzero", digestNonzero, true, 4);
        boolField(json, "null_guard", nullGuard, true, 4);
        boolField(json, "range_guard", rangeGuard, true, 4);
        boolField(json, "page_size_valid", pageSizeValid, false, 4);
        json.append("  }\n");
        json.append("}\n");
        return json.toString();
    }

    private File writeReceipt(String json) throws Exception {
        File directory = new File(getFilesDir(), "rafaelia-zero");
        if (!directory.exists() && !directory.mkdirs()) {
            throw new IllegalStateException("cannot create " + directory);
        }
        File temporary = new File(directory, "latest.json.tmp");
        File target = new File(directory, "latest.json");
        try (FileOutputStream stream = new FileOutputStream(temporary, false);
             OutputStreamWriter writer = new OutputStreamWriter(stream, StandardCharsets.UTF_8)) {
            writer.write(json);
            writer.flush();
            stream.getFD().sync();
        }
        if (target.exists() && !target.delete()) {
            throw new IllegalStateException("cannot replace " + target);
        }
        if (!temporary.renameTo(target)) {
            throw new IllegalStateException("cannot atomically rename receipt");
        }
        return target;
    }

    private String failureReceipt(Throwable t) {
        StringBuilder json = new StringBuilder(512);
        json.append("{\n");
        field(json, "schema", "rafaelia.zero.device.probe.v1", true);
        field(json, "result", "FAIL", true);
        boolField(json, "claim_allowed_device", false, true);
        numberField(json, "timestamp_unix_ms", System.currentTimeMillis(), true);
        field(json, "package", getPackageName(), true);
        field(json, "error_class", t.getClass().getName(), true);
        field(json, "error_message", String.valueOf(t.getMessage()), false);
        json.append("}\n");
        return json.toString();
    }

    private static int expectedArchitecture(String arch) {
        String normalized = arch == null ? "" : arch.toLowerCase();
        if (normalized.contains("aarch64") || normalized.contains("arm64")) return 2;
        if (normalized.startsWith("arm") || normalized.contains("armv7")) return 1;
        if (normalized.contains("x86_64") || normalized.contains("amd64")) return 3;
        if (normalized.contains("x86") || normalized.contains("i686")) return 4;
        return 0;
    }

    private static String join(String[] values) {
        if (values == null || values.length == 0) return "";
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < values.length; i++) {
            if (i != 0) result.append(',');
            result.append(values[i]);
        }
        return result.toString();
    }

    private static void field(StringBuilder out, String key, String value, boolean comma) {
        field(out, key, value, comma, 2);
    }

    private static void field(StringBuilder out, String key, String value, boolean comma, int indent) {
        indent(out, indent);
        out.append('"').append(escape(key)).append("\": \"")
            .append(escape(value)).append('"');
        if (comma) out.append(',');
        out.append('\n');
    }

    private static void boolField(StringBuilder out, String key, boolean value, boolean comma) {
        boolField(out, key, value, comma, 2);
    }

    private static void boolField(StringBuilder out, String key, boolean value, boolean comma, int indent) {
        indent(out, indent);
        out.append('"').append(escape(key)).append("\": ").append(value);
        if (comma) out.append(',');
        out.append('\n');
    }

    private static void numberField(StringBuilder out, String key, long value, boolean comma) {
        numberField(out, key, value, comma, 2);
    }

    private static void numberField(StringBuilder out, String key, long value, boolean comma, int indent) {
        indent(out, indent);
        out.append('"').append(escape(key)).append("\": ").append(value);
        if (comma) out.append(',');
        out.append('\n');
    }

    private static void indent(StringBuilder out, int count) {
        for (int i = 0; i < count; i++) out.append(' ');
    }

    private static String escape(String value) {
        if (value == null) return "";
        StringBuilder escaped = new StringBuilder(value.length() + 16);
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            switch (c) {
                case '\\': escaped.append("\\\\"); break;
                case '"': escaped.append("\\\""); break;
                case '\n': escaped.append("\\n"); break;
                case '\r': escaped.append("\\r"); break;
                case '\t': escaped.append("\\t"); break;
                default:
                    if (c < 0x20) {
                        escaped.append(String.format("\\u%04x", (int) c));
                    } else {
                        escaped.append(c);
                    }
            }
        }
        return escaped.toString();
    }
}
