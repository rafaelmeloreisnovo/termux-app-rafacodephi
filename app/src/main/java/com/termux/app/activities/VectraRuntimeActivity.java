package com.termux.app.activities;

import android.content.Context;
import android.content.Intent;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Bundle;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;

import com.termux.app.api.sensor.RafSensorAndroid;
import com.termux.app.api.sensor.RafSensorContract;
import com.termux.app.benchmark.BenchmarkMenuActivity;
import com.termux.app.benchmark.IndustrialBenchmarkMethodology;
import com.termux.app.benchmark.PaBenchmarkReceipt;
import com.termux.lowlevel.BareMetal;
import com.termux.rafacodephi.R;
import com.termux.shared.activity.media.AppCompatActivityUtils;
import com.termux.shared.theme.NightMode;

import org.json.JSONObject;

import java.io.File;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Internal Termux RAFCODEΦ diagnostics surface historically named "Vectra".
 *
 * Deployment invariant: this Activity is bundled in the Termux RAFCODEΦ APK.
 * It does not require the separate Vectras-VM-Android app, package, repository
 * or CI to exist on the device.
 */
public class VectraRuntimeActivity extends AppCompatActivity {

    private LinearLayout contentLayout;
    private boolean lowLevelLibraryLoaded;
    private String lowLevelLibraryError = "";
    private final AtomicInteger renderEpoch = new AtomicInteger(0);

    private interface SectionBuilder {
        String build() throws Exception;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        AppCompatActivityUtils.setNightMode(this, NightMode.getAppNightMode().getName(), true);
        setContentView(R.layout.activity_vectra_runtime);

        contentLayout = findViewById(R.id.audit_content);
        AppCompatActivityUtils.setToolbar(this, com.termux.shared.R.id.toolbar);
        AppCompatActivityUtils.setShowBackButtonInActionBar(this, true);

        initializeLowLevelLibrary();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (contentLayout != null) refreshRuntimeReport();
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    private void initializeLowLevelLibrary() {
        try {
            System.loadLibrary("api_lowlevel");
            lowLevelLibraryLoaded = true;
            lowLevelLibraryError = "";
        } catch (Throwable error) {
            lowLevelLibraryLoaded = false;
            lowLevelLibraryError = error.getClass().getSimpleName() + ": " + String.valueOf(error.getMessage());
        }
    }

    private void refreshRuntimeReport() {
        int epoch = renderEpoch.incrementAndGet();
        contentLayout.removeAllViews();
        new Thread(() -> renderRuntimeReport(epoch), "vectra-runtime-report-" + epoch).start();
    }

    private void renderRuntimeReport(int epoch) {
        addCard(epoch, "Scope / Deployment Truth", buildScopeSummary());
        addCard(epoch, "Typed Sensor API", safeBuild("sensor API contract", this::buildApiContractSummary));
        addCard(epoch, "Vectra Sampling Presets", safeBuild("sampling presets", this::buildSamplingPresetSummary));
        addCard(epoch, "Bare-metal Hardware Profile", safeBuild("hardware profile", this::buildHardwareProfileSummary));
        addCard(epoch, "Supported Sensor Inventory", safeBuild("sensor inventory", this::buildSensorInventorySummary));
        addCard(epoch, "Deterministic Runtime Benchmark", safeBuild("runtime benchmark receipt", this::buildBenchmarkSummary));
        addCard(epoch, "Evidence Gaps / TOKEN_VAZIO", safeBuild("evidence gap ledger", this::buildGapSummary));
        addRuntimeActionsCard(epoch);
    }

    private String safeBuild(String section, SectionBuilder builder) {
        try {
            String value = builder.build();
            return value == null || value.trim().isEmpty()
                ? "• State: INVALIDATED\n• Reason: empty " + section + " output"
                : value;
        } catch (Throwable error) {
            return "• State: UNAVAILABLE\n"
                + "• Section: " + section + "\n"
                + "• Error: " + error.getClass().getSimpleName() + ": " + String.valueOf(error.getMessage()) + "\n"
                + "• Claim promotion: blocked until a successful observation exists";
        }
    }

    private String buildScopeSummary() {
        return "• Component: INTERNAL_TERMUX_RAFCODEPHI_SCREEN\n"
            + "• APK owner: " + getPackageName() + "\n"
            + "• External Vectras app installed: NOT_REQUIRED\n"
            + "• External Vectras repository/CI: NOT_REQUIRED\n"
            + "• Purpose: local sensor, hardware, low-level and PA benchmark diagnostics\n"
            + "• Invariant: no claim may exceed evidence observed by this installed APK";
    }

    private void addCard(int epoch, String title, String content) {
        runOnUiThread(() -> {
            if (epoch != renderEpoch.get() || isFinishing() || isDestroyed()) return;

            CardView card = new CardView(this);
            LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            );
            cardParams.setMargins(16, 16, 16, 16);
            card.setLayoutParams(cardParams);
            card.setCardElevation(8);
            card.setRadius(16);

            LinearLayout cardContent = new LinearLayout(this);
            cardContent.setOrientation(LinearLayout.VERTICAL);
            cardContent.setPadding(24, 24, 24, 24);

            TextView titleView = new TextView(this);
            titleView.setText(title);
            titleView.setTextSize(18);
            titleView.setTextColor(getResources().getColor(R.color.termux_text_color_primary, getTheme()));
            titleView.setPadding(0, 0, 0, 16);

            TextView contentView = new TextView(this);
            contentView.setText(content);
            contentView.setTextSize(14);
            contentView.setTextIsSelectable(true);
            contentView.setLineSpacing(0, 1.2f);

            cardContent.addView(titleView);
            cardContent.addView(contentView);
            card.addView(cardContent);
            contentLayout.addView(card);
        });
    }

    private void addRuntimeActionsCard(int epoch) {
        runOnUiThread(() -> {
            if (epoch != renderEpoch.get() || isFinishing() || isDestroyed()) return;

            CardView card = new CardView(this);
            LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            );
            cardParams.setMargins(16, 16, 16, 32);
            card.setLayoutParams(cardParams);
            card.setCardElevation(8);
            card.setRadius(16);

            LinearLayout body = new LinearLayout(this);
            body.setOrientation(LinearLayout.VERTICAL);
            body.setPadding(24, 24, 24, 24);

            TextView title = new TextView(this);
            title.setText("Internal Benchmark Actions");
            title.setTextSize(18);
            title.setTextColor(getResources().getColor(R.color.termux_text_color_primary, getTheme()));
            body.addView(title);

            TextView description = new TextView(this);
            description.setText(
                "These actions operate only inside Termux RAFCODEΦ. Run the packaged PA ELF to fill runtime evidence, " +
                "then generate the industrial methods/gap file from the same installed APK.");
            description.setTextSize(14);
            description.setPadding(0, 12, 0, 16);
            body.addView(description);

            Button execute = new Button(this);
            execute.setText("RUN PA ELF BENCHMARK");
            execute.setOnClickListener(view -> startActivity(new Intent(this, BenchmarkMenuActivity.class)));
            body.addView(execute);

            Button generate = new Button(this);
            generate.setText("GENERATE INDUSTRIAL METHODS FILE");
            generate.setOnClickListener(view -> {
                generate.setEnabled(false);
                new Thread(() -> {
                    try {
                        File file = IndustrialBenchmarkMethodology.write(this);
                        runOnUiThread(() -> {
                            generate.setEnabled(true);
                            Toast.makeText(this,
                                "Industrial methods file generated: " + file.getAbsolutePath(),
                                Toast.LENGTH_LONG).show();
                        });
                    } catch (Throwable error) {
                        runOnUiThread(() -> {
                            generate.setEnabled(true);
                            Toast.makeText(this,
                                "Method file generation failed: " + error.getClass().getSimpleName(),
                                Toast.LENGTH_LONG).show();
                        });
                    }
                }, "industrial-method-export").start();
            });
            body.addView(generate);

            Button refresh = new Button(this);
            refresh.setText("REFRESH RUNTIME EVIDENCE");
            refresh.setOnClickListener(view -> refreshRuntimeReport());
            body.addView(refresh);

            card.addView(body);
            contentLayout.addView(card);
        });
    }

    private String buildApiContractSummary() {
        StringBuilder sb = new StringBuilder();
        sb.append("• Explicit Android component only\n");
        sb.append("• Permission gate: ").append(RafSensorContract.PERMISSION).append("\n");
        sb.append("• Snapshot action: ").append(RafSensorContract.ACTION_SENSOR_SNAPSHOT).append("\n");
        sb.append("• Cancel action: ").append(RafSensorContract.ACTION_CANCEL_SENSOR_REQUEST).append("\n");
        sb.append("• Protocol version: ").append(RafSensorContract.PROTOCOL_VERSION_1).append("\n");
        sb.append("• Callback statuses: ")
            .append(RafSensorContract.STATUS_ACCEPTED).append(", ")
            .append(RafSensorContract.STATUS_SAMPLING).append(", ")
            .append(RafSensorContract.STATUS_COMPLETED).append(", ")
            .append(RafSensorContract.STATUS_CANCELLED).append(", ")
            .append(RafSensorContract.STATUS_FAILED).append("\n");
        sb.append("• Allowed sensors: ").append(joinStrings(RafSensorContract.allowedSensorNames())).append("\n");
        sb.append("• Request bounds: requestId<=64, sampling<=1,000,000us, latency<=5,000,000us");
        return sb.toString();
    }

    private String buildSamplingPresetSummary() {
        StringBuilder sb = new StringBuilder();
        for (Map.Entry<String, Integer> entry : RafSensorContract.samplingPresetsUs().entrySet()) {
            sb.append("• ").append(entry.getKey()).append(": ")
                .append(entry.getValue()).append("us");
            if ("FASTEST".equals(entry.getKey())) sb.append(" — requested fastest preset; observed callback rate requires measurement");
            if ("GAME".equals(entry.getKey())) sb.append(" — motion-oriented request preset");
            if ("UI".equals(entry.getKey())) sb.append(" — UI-oriented request preset");
            if ("NORMAL".equals(entry.getKey())) sb.append(" — normal request preset");
            sb.append("\n");
        }
        sb.append("• Evidence boundary: presets are requested sampling parameters, not measured latency/throughput.");
        return sb.toString().trim();
    }

    private String buildHardwareProfileSummary() {
        if (!lowLevelLibraryLoaded) {
            return "• Native library: BLOCKED\n"
                + "• Error: " + lowLevelLibraryError + "\n"
                + "• Hardware profile claim: not promoted";
        }

        BareMetal.HardwareProfile profile = BareMetal.readHardwareProfile();
        BareMetal.CapabilitiesDetail caps = BareMetal.getCapabilitiesDetailParsed();
        if (profile == null || caps == null) {
            return "• State: INVALIDATED\n• Reason: native hardware profile returned null";
        }

        StringBuilder sb = new StringBuilder();
        sb.append("• Native library loaded: yes\n");
        sb.append("• ABI: ").append(profile.abi == null ? "UNAVAILABLE" : profile.abi).append("\n");
        sb.append("• Runtime capabilities: 0x").append(Integer.toHexString(caps.runtime)).append("\n");
        sb.append("• Effective capabilities: 0x").append(Integer.toHexString(caps.effective)).append("\n");
        sb.append("• Access flags: 0x").append(Integer.toHexString(profile.accessFlags)).append("\n");
        if (profile.cpusOnline > 0) sb.append("• CPUs online: ").append(profile.cpusOnline).append("\n");
        else sb.append("• CPUs online: UNAVAILABLE\n");
        if (profile.pageSize > 0) sb.append("• Page size: ").append(profile.pageSize).append(" bytes\n");
        else sb.append("• Page size: UNAVAILABLE\n");
        if (profile.cacheLine > 0) sb.append("• Cache line: ").append(profile.cacheLine).append(" bytes\n");
        else sb.append("• Cache line: UNAVAILABLE (detector returned non-positive value)\n");
        String clusters = profile.cpuClusters == null ? "" : profile.cpuClusters.trim();
        if (!clusters.isEmpty() && !"n/a".equalsIgnoreCase(clusters) && !"unknown".equalsIgnoreCase(clusters)) {
            sb.append("• CPU clusters: ").append(clusters).append("\n");
        } else {
            sb.append("• CPU clusters: UNAVAILABLE\n");
        }
        sb.append("• Device ABI list: ").append(joinStrings(Build.SUPPORTED_ABIS));
        return sb.toString();
    }

    private String buildSensorInventorySummary() {
        SensorManager manager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        if (manager == null) return "• State: UNAVAILABLE\n• Reason: SensorManager unavailable";

        StringBuilder sb = new StringBuilder();
        for (String sensorName : RafSensorContract.allowedSensorNames()) {
            try {
                int sensorType = RafSensorAndroid.toSensorType(sensorName);
                Sensor sensor = manager.getDefaultSensor(sensorType);
                if (sensor == null) {
                    sb.append("• ").append(sensorName).append(": UNAVAILABLE\n");
                    continue;
                }
                sb.append("• ").append(sensorName)
                    .append(": ").append(sensor.getName())
                    .append(" / ").append(sensor.getVendor())
                    .append(" / mode=").append(RafSensorAndroid.reportingModeToString(sensor))
                    .append(" / minDelay=").append(sensor.getMinDelay()).append("us")
                    .append(" / frameworkPower=").append(String.format(Locale.US, "%.3f", sensor.getPower())).append("mA")
                    .append("\n");
            } catch (Throwable error) {
                sb.append("• ").append(sensorName).append(": INVALIDATED (")
                    .append(error.getClass().getSimpleName()).append(")\n");
            }
        }
        List<Sensor> allSensors = manager.getSensorList(Sensor.TYPE_ALL);
        sb.append("\n• Total sensors reported by framework: ").append(allSensors == null ? "UNAVAILABLE" : String.valueOf(allSensors.size()));
        sb.append("\n• Evidence boundary: inventory is observed; framework power metadata is not measured energy; callback timing requires a sampling receipt.");
        return sb.toString();
    }

    private String buildBenchmarkSummary() {
        StringBuilder sb = new StringBuilder();
        String readState = PaBenchmarkReceipt.getReadState(this);
        JSONObject receipt = PaBenchmarkReceipt.read(this);
        if ("NOT_MEASURED".equals(readState)) {
            sb.append("• Runtime evidence: NOT_MEASURED — no PA receipt from this build/install\n");
            sb.append("• Proof route: internal Vectra screen → BenchmarkMenuActivity → Android linker → packaged freestanding ELF\n");
            sb.append("• External Vectras app/CI: NOT_REQUIRED\n");
            sb.append("• Next proof: tap RUN PA ELF BENCHMARK below");
        } else if (receipt == null) {
            sb.append("• Runtime evidence: INVALIDATED\n");
            sb.append("• Reason: latest PA receipt exists but cannot be parsed within the receipt contract\n");
            sb.append("• Claim promotion: false\n");
            sb.append("• Recovery: run PA ELF again; history directory is not deleted");
        } else {
            String state = receipt.optString("evidence_state", "INVALIDATED");
            sb.append("• Runtime evidence: ").append(state).append("\n");
            sb.append("• Reason: ").append(receipt.optString("evidence_reason", "UNKNOWN")).append("\n");
            sb.append("• Receipt: ").append(PaBenchmarkReceipt.getReceiptFile(this).getAbsolutePath()).append("\n");
            sb.append("• History: ").append(PaBenchmarkReceipt.getHistoryDirectory(this).getAbsolutePath()).append("\n");
            sb.append("• Timestamp: ").append(receipt.optString("generated_at_utc", "UNAVAILABLE")).append("\n");
            sb.append("• Linker: ").append(emptyToUnavailable(receipt.optString("linker", ""))).append("\n");
            sb.append("• Exit code: ").append(receipt.optInt("exit_code", -1)).append("\n");
            sb.append("• Timed out: ").append(receipt.optBoolean("timed_out", false)).append("\n");
            sb.append("• Wall time: ").append(receipt.optLong("wall_time_ms", -1L)).append(" ms\n");
            sb.append("• Stdout truncated: ").append(receipt.optBoolean("stdout_truncated", false)).append("\n");
            sb.append("• ELF SHA-256: ").append(emptyToUnavailable(receipt.optString("elf_sha256", ""))).append("\n");
            sb.append("• Stdout SHA-256: ").append(emptyToUnavailable(receipt.optString("stdout_sha256", ""))).append("\n");
            JSONObject markers = receipt.optJSONObject("markers");
            if (markers != null) {
                int markerPass = 0;
                String[] names = {"header", "mode_contract_marker", "r0", "r1", "r2", "r3", "r4", "r5", "end"};
                for (String name : names) if (markers.optBoolean(name, false)) markerPass++;
                sb.append("• Required stdout markers: ").append(markerPass).append("/").append(names.length).append("\n");
            } else {
                sb.append("• Required stdout markers: INVALIDATED — marker object absent\n");
            }
            sb.append("• Runtime execution claim allowed: ")
                .append(receipt.optBoolean("claim_allowed_runtime_execution", false)).append("\n");
            sb.append("• Isolated-silicon claim allowed: false\n");
            sb.append("• Reproducibility claim allowed: false until homogeneous repeated trials exist\n");
        }

        if (!lowLevelLibraryLoaded) {
            sb.append("\n• Low-level state: BLOCKED (").append(lowLevelLibraryError).append(")");
            return sb.toString();
        }

        try {
            long state = com.termux.app.api.ApiLowLevelBridge.nativeStateQuery();
            int hiWord = (int)(state >>> 32);
            int statePhase = (hiWord >>> 24) & 0xFF;
            int stateAtt = (hiWord >>> 16) & 0xFF;
            int stateFlags = (hiWord >>> 8) & 0xFF;
            int stateEntropy = hiWord & 0xFF;
            int stateEvents = (int)(state & 0xFFFFFFFFL);
            sb.append(String.format(Locale.US,
                "\n• Low-level state: phase=%d att=%d flags=0x%02x entropy=0x%02x events=%d",
                statePhase, stateAtt, stateFlags, stateEntropy, stateEvents));
        } catch (Throwable error) {
            sb.append("\n• Low-level state: UNAVAILABLE (")
                .append(error.getClass().getSimpleName()).append(")");
        }
        return sb.toString();
    }

    private String buildGapSummary() {
        StringBuilder sb = new StringBuilder();
        String readState = PaBenchmarkReceipt.getReadState(this);
        JSONObject receipt = PaBenchmarkReceipt.read(this);

        sb.append("• External Vectras installation: NOT_REQUIRED — closed by scope invariant\n");
        sb.append("• External Vectras CI: NOT_REQUIRED — closed by scope invariant\n");
        sb.append("• api_lowlevel load: ")
            .append(lowLevelLibraryLoaded ? "PASS" : "BLOCKED — " + lowLevelLibraryError).append("\n");

        if ("NOT_MEASURED".equals(readState)) {
            sb.append("• PA physical execution receipt: TOKEN_VAZIO / NOT_MEASURED\n");
        } else if (receipt == null) {
            sb.append("• PA physical execution receipt: INVALIDATED — latest receipt unreadable\n");
        } else {
            sb.append("• PA physical execution receipt: ")
                .append(receipt.optString("evidence_state", "INVALIDATED")).append("\n");
        }

        sb.append("• Repeated homogeneous PA series (n>1): TOKEN_VAZIO — not yet recorded by receipt v2\n");
        sb.append("• Timer overhead/calibration receipt: TOKEN_VAZIO — not yet measured\n");
        sb.append("• CPU frequency/DVFS pre-run and post-run: TOKEN_VAZIO — not yet captured\n");
        sb.append("• Thermal pre-run/post-run evidence: TOKEN_VAZIO — not yet captured\n");
        sb.append("• PMU counters (cycles/instructions/cache/branch): TOKEN_VAZIO or UNAVAILABLE — capability not yet probed\n");
        sb.append("• Sensor callback timing series: TOKEN_VAZIO — inventory exists, acquisition timing receipt absent\n");
        sb.append("• Industrial composite score: BLOCKED BY DESIGN until normalization + uncertainty contract exists\n");
        sb.append("• Next invariant action: fill measurable TOKEN_VAZIO fields with direct device receipts; leave inaccessible counters UNAVAILABLE rather than inventing zero.");
        return sb.toString();
    }

    private static String emptyToUnavailable(String value) {
        return value == null || value.trim().isEmpty() ? "UNAVAILABLE" : value;
    }

    private static String joinStrings(String[] values) {
        if (values == null || values.length == 0) return "UNAVAILABLE";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < values.length; i++) {
            if (i > 0) sb.append(", ");
            sb.append(values[i] == null ? "UNAVAILABLE" : values[i]);
        }
        return sb.toString();
    }

    private static String joinStrings(Iterable<String> values) {
        if (values == null) return "UNAVAILABLE";
        StringBuilder sb = new StringBuilder();
        boolean first = true;
        for (String value : values) {
            if (!first) sb.append(", ");
            sb.append(value == null ? "UNAVAILABLE" : value);
            first = false;
        }
        return first ? "UNAVAILABLE" : sb.toString();
    }
}
