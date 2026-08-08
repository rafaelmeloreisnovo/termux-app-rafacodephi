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
    private boolean apiLowLevelLibraryLoaded;
    private String apiLowLevelLibraryError = "";
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

        initializeApiLowLevelLibrary();
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

    private void renderRuntimeReport() {
        addCard("Typed Sensor API", buildApiContractSummary());
        addCard("Vectra Sampling Presets", buildSamplingPresetSummary());
        addCard("Bare-metal Hardware Profile", buildHardwareProfileSummary());
        addCard("Supported Sensor Inventory", buildSensorInventorySummary());
        addCard("Deterministic Runtime Benchmark", buildBenchmarkSummary());
        addIndustrialMethodologyCard();
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
            + "• External Vectras installation requirement: NOT_REQUIRED\n"
            + "• External Vectras repository/CI requirement: NOT_REQUIRED\n"
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

    private void addIndustrialMethodologyCard() {
        runOnUiThread(() -> {
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
            title.setText("Industrial Benchmark Methodology");
            title.setTextSize(18);
            title.setTextColor(getResources().getColor(R.color.termux_text_color_primary, getTheme()));
            body.addView(title);

            TextView description = new TextView(this);
            description.setText(
                "Generate an auditable methods file covering workload invariants, silicon-observation boundaries, " +
                "seven production domains, statistical rules, provenance, interference gates and release criteria.");
            description.setTextSize(14);
            description.setPadding(0, 12, 0, 16);
            body.addView(description);

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
        if (!BareMetal.isLoaded()) {
            return "• termux-baremetal library: UNAVAILABLE\n"
                + "• Hardware profile: UNAVAILABLE\n"
                + "• api_lowlevel status is reported separately and does not substitute for BareMetal capability evidence";
        }

        BareMetal.HardwareProfile profile = BareMetal.readHardwareProfile();
        BareMetal.CapabilitiesDetail caps = BareMetal.getCapabilitiesDetailParsed();
        if (profile == null || caps == null) {
            return "• State: INVALIDATED\n• Reason: termux-baremetal hardware profile returned null";
        }

        StringBuilder sb = new StringBuilder();
        sb.append("• termux-baremetal library loaded: yes\n");
        sb.append("• ABI: ").append(profile.abi == null ? "UNAVAILABLE" : profile.abi).append("\n");
        sb.append("• Runtime capabilities: 0x").append(Integer.toHexString(caps.runtime)).append("\n");
        sb.append("• Effective capabilities: 0x").append(Integer.toHexString(caps.effective)).append("\n");
        sb.append("• Runtime capabilities directly valid: ").append(caps.runtimeValid).append("\n");
        sb.append("• Access flags: 0x").append(Integer.toHexString(profile.accessFlags)).append("\n");
        sb.append("• CPUs online: ").append(profile.cpusOnline).append("\n");
        sb.append("• Page size: ").append(profile.pageSize).append(" bytes\n");
        if (profile.cacheLine > 0) {
            sb.append("• Cache line: ").append(profile.cacheLine).append(" bytes\n");
        } else {
            sb.append("• Cache line: UNAVAILABLE (runtime detector returned 0; zero is not treated as a physical cache-line size)\n");
        }
        sb.append("• CPU clusters: ").append(profile.cpuClusters).append("\n");
        sb.append("• Device ABI list: ").append(String.join(", ", Build.SUPPORTED_ABIS));
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
        sb.append("\n• Total sensors reported by framework: ").append(allSensors.size());
        sb.append("\n• Evidence boundary: inventory is observed; sampling latency/callback reproducibility requires a sampling receipt.");
        return sb.toString();
    }

    private String buildBenchmarkSummary() {
        StringBuilder sb = new StringBuilder();
        JSONObject receipt = PaBenchmarkReceipt.read(this);
        if (receipt == null) {
            sb.append("• Runtime evidence: NOT_MEASURED — no persisted PA device receipt\n");
            sb.append("• Proof route: BenchmarkMenuActivity → Android linker → packaged freestanding ELF\n");
            sb.append("• Next proof: execute the ELF benchmark once; the launcher now persists artifact hash, stdout markers and exit code atomically\n");
        } else {
            boolean pass = receipt.optBoolean("runtime_exec_pass", false);
            sb.append("• Runtime evidence: ").append(pass ? "PASS" : "FAIL/BLOCKED").append("\n");
            sb.append("• Receipt: ").append(PaBenchmarkReceipt.getReceiptFile(this).getAbsolutePath()).append("\n");
            sb.append("• Timestamp: ").append(receipt.optString("generated_at_utc", "UNAVAILABLE")).append("\n");
            sb.append("• Linker: ").append(receipt.optString("linker", "UNAVAILABLE")).append("\n");
            sb.append("• Exit code: ").append(receipt.optInt("exit_code", -1)).append("\n");
            sb.append("• ELF SHA-256: ").append(receipt.optString("elf_sha256", "UNAVAILABLE")).append("\n");
            sb.append("• Stdout SHA-256: ").append(receipt.optString("stdout_sha256", "UNAVAILABLE")).append("\n");
            JSONObject markers = receipt.optJSONObject("markers");
            if (markers != null) {
                int markerPass = 0;
                String[] names = {"header", "mode_contract_marker", "r0", "r1", "r2", "r3", "r4", "r5", "end"};
                for (String name : names) if (markers.optBoolean(name, false)) markerPass++;
                sb.append("• Required stdout markers: ").append(markerPass).append("/").append(names.length).append("\n");
            }
            sb.append("• Evidence scope: ").append(receipt.optString("evidence_scope", "UNAVAILABLE")).append("\n");
            sb.append("• Claim boundary: runtime execution proof is not promoted to isolated-silicon or reproducibility proof\n");
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
                "• Low-level state: phase=%d att=%d flags=0x%02x entropy=0x%02x events=%d",
                statePhase, stateAtt, stateFlags, stateEntropy, stateEvents));
        } catch (Throwable error) {
            sb.append("• Low-level state: UNAVAILABLE (")
                .append(error.getClass().getSimpleName())
                .append(")");
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
            .append(apiLowLevelLibraryLoaded ? "PASS" : "BLOCKED — " + apiLowLevelLibraryError).append("\n");
        sb.append("• termux-baremetal load: ")
            .append(BareMetal.isLoaded() ? "PASS" : "UNAVAILABLE — native library not loaded").append("\n");

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
