package com.termux.app.activities;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Bundle;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;

import com.termux.app.api.sensor.RafSensorAndroid;
import com.termux.app.api.sensor.RafSensorContract;
import com.termux.lowlevel.BareMetal;
import com.termux.rafacodephi.R;
import com.termux.shared.activity.media.AppCompatActivityUtils;
import com.termux.shared.theme.NightMode;

import java.util.List;
import java.util.Locale;
import java.util.Map;

public class VectraRuntimeActivity extends AppCompatActivity {

    private LinearLayout contentLayout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        AppCompatActivityUtils.setNightMode(this, NightMode.getAppNightMode().getName(), true);
        setContentView(R.layout.activity_vectra_runtime);

        contentLayout = findViewById(R.id.audit_content);
        AppCompatActivityUtils.setToolbar(this, com.termux.shared.R.id.toolbar);
        AppCompatActivityUtils.setShowBackButtonInActionBar(this, true);

        new Thread(this::renderRuntimeReport).start();
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
    }

    private void addCard(String title, String content) {
        runOnUiThread(() -> {
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
            contentView.setLineSpacing(0, 1.2f);

            cardContent.addView(titleView);
            cardContent.addView(contentView);
            card.addView(cardContent);
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
        sb.append("• Allowed sensors: ").append(String.join(", ", RafSensorContract.allowedSensorNames())).append("\n");
        sb.append("• Request bounds: requestId<=64, sampling<=1,000,000us, latency<=5,000,000us");
        return sb.toString();
    }

    private String buildSamplingPresetSummary() {
        StringBuilder sb = new StringBuilder();
        for (Map.Entry<String, Integer> entry : RafSensorContract.samplingPresetsUs().entrySet()) {
            sb.append("• ").append(entry.getKey()).append(": ")
                .append(entry.getValue()).append("us");
            if ("FASTEST".equals(entry.getKey())) sb.append(" — near-real-time snapshots");
            if ("GAME".equals(entry.getKey())) sb.append(" — motion-heavy interaction");
            if ("UI".equals(entry.getKey())) sb.append(" — balanced UI refresh");
            if ("NORMAL".equals(entry.getKey())) sb.append(" — low-power monitoring");
            sb.append("\n");
        }
        return sb.toString().trim();
    }

    private String buildHardwareProfileSummary() {
        BareMetal.HardwareProfile profile = BareMetal.readHardwareProfile();
        BareMetal.CapabilitiesDetail caps = BareMetal.getCapabilitiesDetailParsed();
        StringBuilder sb = new StringBuilder();
        sb.append("• Native library loaded: ").append(BareMetal.isLoaded() ? "yes" : "no").append("\n");
        sb.append("• ABI: ").append(profile.abi).append("\n");
        sb.append("• Runtime capabilities: 0x").append(Integer.toHexString(caps.runtime)).append("\n");
        sb.append("• Effective capabilities: 0x").append(Integer.toHexString(caps.effective)).append("\n");
        sb.append("• Access flags: 0x").append(Integer.toHexString(profile.accessFlags)).append("\n");
        sb.append("• CPUs online: ").append(profile.cpusOnline).append("\n");
        sb.append("• Page size: ").append(profile.pageSize).append(" bytes\n");
        sb.append("• Cache line: ").append(profile.cacheLine).append(" bytes\n");
        sb.append("• CPU clusters: ").append(profile.cpuClusters).append("\n");
        sb.append("• Device ABI list: ").append(String.join(", ", Build.SUPPORTED_ABIS));
        return sb.toString();
    }

    private String buildSensorInventorySummary() {
        SensorManager manager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        if (manager == null) {
            return "SensorManager unavailable";
        }
        StringBuilder sb = new StringBuilder();
        for (String sensorName : RafSensorContract.allowedSensorNames()) {
            Sensor sensor = manager.getDefaultSensor(RafSensorAndroid.toSensorType(sensorName));
            if (sensor == null) {
                sb.append("• ").append(sensorName).append(": unavailable\n");
                continue;
            }
            sb.append("• ").append(sensorName)
                .append(": ").append(sensor.getName())
                .append(" / ").append(sensor.getVendor())
                .append(" / mode=").append(RafSensorAndroid.reportingModeToString(sensor))
                .append(" / minDelay=").append(sensor.getMinDelay()).append("us")
                .append(" / power=").append(String.format(Locale.US, "%.3f", sensor.getPower())).append("mA")
                .append("\n");
        }
        List<Sensor> allSensors = manager.getSensorList(Sensor.TYPE_ALL);
        sb.append("\n• Total sensors reported by framework: ").append(allSensors.size());
        return sb.toString();
    }

    private String buildBenchmarkSummary() {
        final int q16Steps = 200_000;
        int state = 65_536;
        long q16Start = System.nanoTime();
        for (int i = 0; i < q16Steps; i++) {
            state = (int) ((((long) state * 56_756L) >> 16) + 203_280L);
            state ^= (i & 0x3f);
        }
        long q16Elapsed = System.nanoTime() - q16Start;

        byte[] src = new byte[4_096];
        byte[] dst = new byte[4_096];
        for (int i = 0; i < src.length; i++) {
            src[i] = (byte) ((i * 31) ^ (i >>> 3));
        }

        long checksum = 0xcbf29ce484222325L;
        final int rounds = 256;
        long bufferStart = System.nanoTime();
        for (int round = 0; round < rounds; round++) {
            System.arraycopy(src, 0, dst, 0, src.length);
            for (byte value : dst) {
                checksum ^= (value & 0xffL);
                checksum *= 0x100000001b3L;
            }
        }
        long bufferElapsed = System.nanoTime() - bufferStart;
        double seconds = bufferElapsed / 1_000_000_000.0d;
        double mib = (src.length * rounds) / (1024.0d * 1024.0d);

        StringBuilder sb = new StringBuilder();
        sb.append("• Q16 recurrence final state: ").append(state).append("\n");
        sb.append("• Q16 elapsed: ").append(String.format(Locale.US, "%.3f", q16Elapsed / 1_000_000.0d)).append(" ms\n");
        sb.append("• Buffer bytes: ").append(src.length * rounds).append("\n");
        sb.append("• Buffer elapsed: ").append(String.format(Locale.US, "%.3f", bufferElapsed / 1_000_000.0d)).append(" ms\n");
        sb.append("• FNV checksum: 0x").append(Long.toHexString(checksum)).append("\n");
        sb.append("• Throughput: ").append(seconds > 0 ? String.format(Locale.US, "%.2f", mib / seconds) : "unknown").append(" MiB/s\n");
        sb.append("• Sensor runtime policy: typed requests only, no generic command channel");
        return sb.toString();
    }
}
