package com.termux.app.benchmark;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Benchmark menu Activity — mirrors vectras-vm-android BenchmarkActivity
 * 6 categories × 4 execution profiles
 * All heavy computation delegated to native via JNI (api_lowlevel.so)
 * No GC-heavy allocations in measurement path
 */
public final class BenchmarkMenuActivity extends Activity {

    static {
        System.loadLibrary("api_lowlevel");
    }

    /* ── Native JNI (api_jni_bridge.c) ──────────────────────────────────── */
    /** Run one benchmark category. Returns packed: hi32=score lo32=cycles>>8 */
    public static native long nativeBenchRun(int profile, int category);
    /** Read ARM virtual cycle counter (cntvct_el0). */
    public static native long nativeCycleRead();
    /** Hardware capability bitmask: bit0=CNTVCT bit1=CRC32C bit2=NEON */
    public static native int  nativeHwCaps();

    /* ── Execution profiles ───────────────────────────────────────────────── */
    private static final int PROF_AUTO        = 0;
    private static final int PROF_DETERMINISTIC = 1;
    private static final int PROF_THROUGHPUT  = 2;
    private static final int PROF_LOW_LATENCY = 3;

    /* ── Category names ──────────────────────────────────────────────────── */
    private static final String[] CAT_NAMES = {
        "CPU Single", "CPU Multi", "Memory", "Storage", "Integrity", "Emulation"
    };
    private static final int CAT_COUNT = 6;

    /* ── UI refs ─────────────────────────────────────────────────────────── */
    private ProgressBar[] mBars;
    private TextView[]    mScores;
    private TextView      mTotalScore;
    private TextView      mHwCaps;
    private Button        mRunBtn;
    private RadioGroup    mProfileGroup;
    private final AtomicBoolean mRunning = new AtomicBoolean(false);
    private final Handler mUiHandler = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        setTitle("RafCodePhi Benchmark");
        setContentView(buildLayout());
        long baselineCycles = nativeCycleRead();
        mHwCaps.setText(formatHwCaps(nativeHwCaps())
            + "  cycle=0x" + Long.toHexString(baselineCycles));
    }

    /* ── Layout built programmatically (no XML dependency) ──────────────── */
    private View buildLayout() {
        ScrollView sv = new ScrollView(this);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(24, 24, 24, 24);

        /* Title */
        TextView title = new TextView(this);
        title.setText("RafCodePhi · Low-Level Benchmark");
        title.setTextSize(18f);
        title.setPadding(0, 0, 0, 16);
        root.addView(title);

        /* HW caps */
        mHwCaps = new TextView(this);
        mHwCaps.setTextSize(12f);
        root.addView(mHwCaps);

        /* Profile selector */
        TextView profLabel = new TextView(this);
        profLabel.setText("Execution Profile:");
        profLabel.setPadding(0, 16, 0, 4);
        root.addView(profLabel);

        mProfileGroup = new RadioGroup(this);
        mProfileGroup.setOrientation(RadioGroup.HORIZONTAL);
        String[] profNames = {"Auto", "Deterministic", "Throughput", "Low-Latency"};
        for (int i = 0; i < profNames.length; i++) {
            RadioButton rb = new RadioButton(this);
            rb.setText(profNames[i]);
            rb.setId(i);
            mProfileGroup.addView(rb);
        }
        ((RadioButton) mProfileGroup.getChildAt(0)).setChecked(true);
        root.addView(mProfileGroup);

        /* Category bars */
        mBars   = new ProgressBar[CAT_COUNT];
        mScores = new TextView[CAT_COUNT];
        for (int i = 0; i < CAT_COUNT; i++) {
            LinearLayout row = new LinearLayout(this);
            row.setOrientation(LinearLayout.HORIZONTAL);
            row.setPadding(0, 8, 0, 0);

            TextView label = new TextView(this);
            label.setText(CAT_NAMES[i]);
            label.setMinWidth(160);
            row.addView(label);

            mBars[i] = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
            mBars[i].setMax(1000);
            mBars[i].setProgress(0);
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
            mBars[i].setLayoutParams(lp);
            row.addView(mBars[i]);

            mScores[i] = new TextView(this);
            mScores[i].setText("  -");
            mScores[i].setMinWidth(80);
            row.addView(mScores[i]);

            root.addView(row);
        }

        /* Total score */
        LinearLayout totalRow = new LinearLayout(this);
        totalRow.setOrientation(LinearLayout.HORIZONTAL);
        totalRow.setPadding(0, 16, 0, 0);
        TextView totalLabel = new TextView(this);
        totalLabel.setText("Total Score: ");
        totalLabel.setTextSize(16f);
        totalRow.addView(totalLabel);
        mTotalScore = new TextView(this);
        mTotalScore.setText("—");
        mTotalScore.setTextSize(16f);
        totalRow.addView(mTotalScore);
        root.addView(totalRow);

        /* Run button */
        mRunBtn = new Button(this);
        mRunBtn.setText("Run All Benchmarks");
        mRunBtn.setPadding(0, 16, 0, 0);
        mRunBtn.setOnClickListener(v -> startBenchmarks());
        root.addView(mRunBtn);

        sv.addView(root);
        return sv;
    }

    /* ── Benchmark runner ────────────────────────────────────────────────── */
    private void startBenchmarks() {
        if (!mRunning.compareAndSet(false, true)) return;
        mRunBtn.setEnabled(false);
        mTotalScore.setText("Running…");

        /* selected profile — read RadioGroup */
        final int profile = mProfileGroup.getCheckedRadioButtonId();

        new Thread(() -> {
            long[] scores = new long[CAT_COUNT];
            long total = 0L;
            try {
                for (int cat = 0; cat < CAT_COUNT; cat++) {
                    final int c = cat;
                    long raw = nativeBenchRun(profile, cat);
                    /* hi32 = score, lo32 = cycles>>8 */
                    long score = (raw >>> 32) & 0xFFFFFFFFL;
                    scores[c] = score;
                    total += score;
                    final long fTotal = total;
                    final long fScore = score;
                    /* max score per category for progress scaling: 1M */
                    final int prog = (int) Math.min(1000L, (fScore * 1000L) / 1_000_000L);
                    mUiHandler.post(() -> {
                        mBars[c].setProgress(prog);
                        mScores[c].setText(String.format(Locale.US, " %,d", fScore));
                        mTotalScore.setText(String.format(Locale.US, "%,d", fTotal));
                    });
                }
            } finally {
                mUiHandler.post(() -> {
                    mRunBtn.setEnabled(true);
                    mRunning.set(false);
                });
            }
        }, "bench-runner").start();
    }

    /* ── Hardware capability display ──────────────────────────────────────── */
    private static String formatHwCaps(int caps) {
        StringBuilder sb = new StringBuilder("HW: ");
        if ((caps & 1) != 0) sb.append("CNTVCT ");
        if ((caps & 2) != 0) sb.append("CRC32C ");
        if ((caps & 4) != 0) sb.append("NEON ");
        if (sb.length() == 4) sb.append("none");
        return sb.toString();
    }
}
