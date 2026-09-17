package com.termux.app.activities;

import android.content.pm.PackageInfo;
import android.content.res.XmlResourceParser;
import android.os.Build;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import com.termux.rafacodephi.R;

import org.xmlpull.v1.XmlPullParser;

import java.io.File;
import java.io.FileInputStream;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Human-readable view over the machine-readable RAFCODEPhi capability contract.
 *
 * The contract deliberately separates:
 * SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM.
 *
 * This activity is primarily a reader. Its only runtime probe is a bounded,
 * read-only inspection of the APK currently executing this activity.
 */
public class CapabilityContractActivity extends AppCompatActivity {

    private static final String[] LEVEL_IDS = {
        "leigo", "basico", "medio", "avancado", "expert", "nerd"
    };

    private static final String[] LEVEL_LABELS = {
        "Leigo", "Básico", "Médio", "Avançado", "Expert", "Só para Nerds"
    };

    private final List<Capability> capabilities = new ArrayList<>();
    private String selectedLevel = "leigo";
    private LinearLayout capabilityContainer;
    private TextView runtimeEvidenceView;
    private RuntimeSnapshot runtimeSnapshot;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle("RAFCODEΦ · Conheça e Comprove");

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) actionBar.setDisplayHomeAsUpEnabled(true);

        capabilities.addAll(loadContract());
        setContentView(buildContentView());
        renderSelectedLevel();
        collectRuntimeEvidenceAsync();
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    @NonNull
    private View buildContentView() {
        ScrollView scrollView = new ScrollView(this);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(16);
        root.setPadding(pad, pad, pad, pad);
        scrollView.addView(root, new ScrollView.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT));

        root.addView(text(
            "Uma verdade técnica, várias profundidades de leitura. A linguagem muda; a evidência não.",
            18, true));
        root.addView(text(
            "Regra do contrato: SOURCE ≠ ARTIFACT ≠ EXECUTION ≠ EVIDENCE ≠ CLAIM. " +
                "IMPLEMENTED_UNTESTED ≠ PASS. TOKEN_VAZIO é estado válido.",
            14, false));

        TextView levelTitle = text("Nível de leitura", 16, true);
        setTopMargin(levelTitle, 18);
        root.addView(levelTitle);

        HorizontalScrollView levelScroller = new HorizontalScrollView(this);
        LinearLayout levelRow = new LinearLayout(this);
        levelRow.setOrientation(LinearLayout.HORIZONTAL);
        levelScroller.addView(levelRow);
        for (int i = 0; i < LEVEL_IDS.length; i++) {
            final String levelId = LEVEL_IDS[i];
            Button button = new Button(this);
            button.setText(LEVEL_LABELS[i]);
            button.setAllCaps(false);
            button.setOnClickListener(v -> {
                selectedLevel = levelId;
                renderSelectedLevel();
            });
            levelRow.addView(button);
        }
        root.addView(levelScroller);

        TextView runtimeTitle = text("Evidência desta instalação", 16, true);
        setTopMargin(runtimeTitle, 18);
        root.addView(runtimeTitle);

        runtimeEvidenceView = text("Coletando somente evidência local e de leitura do APK instalado…", 14, false);
        root.addView(runtimeEvidenceView);

        TextView catalogTitle = text("Capacidades / métodos", 16, true);
        setTopMargin(catalogTitle, 18);
        root.addView(catalogTitle);

        capabilityContainer = new LinearLayout(this);
        capabilityContainer.setOrientation(LinearLayout.VERTICAL);
        root.addView(capabilityContainer);

        return scrollView;
    }

    private void renderSelectedLevel() {
        if (capabilityContainer == null) return;
        capabilityContainer.removeAllViews();

        String levelLabel = labelForLevel(selectedLevel);
        TextView selected = text("Visão atual: " + levelLabel, 14, true);
        capabilityContainer.addView(selected);

        for (Capability capability : capabilities) {
            LinearLayout card = new LinearLayout(this);
            card.setOrientation(LinearLayout.VERTICAL);
            int inner = dp(12);
            card.setPadding(inner, inner, inner, inner);
            LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
            cardParams.topMargin = dp(10);
            card.setLayoutParams(cardParams);

            card.addView(text(capability.title, 17, true));
            card.addView(text("Estado: " + capability.status + "  ·  claim: " + capability.claimPolicy, 12, false));
            card.addView(text(capability.levels.getOrDefault(selectedLevel, "TOKEN_VAZIO"), 14, false));

            if (isTechnicalLevel(selectedLevel)) {
                card.addView(text("SOURCE: " + capability.source, 12, false));
                card.addView(text("EXECUTOR: " + capability.executor, 12, false));
                card.addView(text("EVIDENCE: " + capability.evidence, 12, false));
                card.addView(text("GAP: " + capability.gap, 12, false));
            } else if ("basico".equals(selectedLevel) || "medio".equals(selectedLevel)) {
                card.addView(text("Como conferir: " + capability.evidence, 12, false));
                card.addView(text("Ainda não prova: " + capability.gap, 12, false));
            }

            capabilityContainer.addView(card);
        }

        if (runtimeEvidenceView != null && runtimeSnapshot != null) {
            runtimeEvidenceView.setText(formatRuntimeSnapshot(runtimeSnapshot, selectedLevel));
        }
    }

    private boolean isTechnicalLevel(String level) {
        return "avancado".equals(level) || "expert".equals(level) || "nerd".equals(level);
    }

    private String labelForLevel(String level) {
        for (int i = 0; i < LEVEL_IDS.length; i++) {
            if (LEVEL_IDS[i].equals(level)) return LEVEL_LABELS[i];
        }
        return level;
    }

    private List<Capability> loadContract() {
        List<Capability> result = new ArrayList<>();
        XmlResourceParser parser = getResources().getXml(R.xml.rafcodephi_capability_contract);
        Capability current = null;
        try {
            int event = parser.getEventType();
            while (event != XmlPullParser.END_DOCUMENT) {
                if (event == XmlPullParser.START_TAG) {
                    String name = parser.getName();
                    if ("capability".equals(name)) {
                        current = new Capability();
                        current.id = value(parser.getAttributeValue(null, "id"));
                        current.title = value(parser.getAttributeValue(null, "title"));
                        current.status = value(parser.getAttributeValue(null, "status"));
                        current.claimPolicy = value(parser.getAttributeValue(null, "claimPolicy"));
                    } else if (current != null && isTextElement(name)) {
                        String text = value(parser.nextText()).trim();
                        if (isLevel(name)) {
                            current.levels.put(name, text);
                        } else if ("source".equals(name)) {
                            current.source = text;
                        } else if ("executor".equals(name)) {
                            current.executor = text;
                        } else if ("evidence".equals(name)) {
                            current.evidence = text;
                        } else if ("gap".equals(name)) {
                            current.gap = text;
                        }
                    }
                } else if (event == XmlPullParser.END_TAG && "capability".equals(parser.getName()) && current != null) {
                    result.add(current);
                    current = null;
                }
                event = parser.next();
            }
        } catch (Exception e) {
            Capability failure = new Capability();
            failure.id = "contract_parse_error";
            failure.title = "Contrato indisponível";
            failure.status = "TOKEN_VAZIO";
            failure.claimPolicy = "no_claim";
            failure.levels.put("leigo", "O contrato não pôde ser lido nesta instalação.");
            for (String level : LEVEL_IDS) failure.levels.put(level, "Falha de leitura: " + e.getClass().getSimpleName());
            failure.gap = "Corrigir o XML/empacotamento antes de promover qualquer claim.";
            result.add(failure);
        } finally {
            parser.close();
        }
        return result;
    }

    private boolean isTextElement(String name) {
        return isLevel(name) || "source".equals(name) || "executor".equals(name) ||
            "evidence".equals(name) || "gap".equals(name);
    }

    private boolean isLevel(String name) {
        for (String level : LEVEL_IDS) if (level.equals(name)) return true;
        return false;
    }

    private void collectRuntimeEvidenceAsync() {
        new Thread(() -> {
            RuntimeSnapshot snapshot = inspectInstalledApk();
            runOnUiThread(() -> {
                runtimeSnapshot = snapshot;
                if (runtimeEvidenceView != null) {
                    runtimeEvidenceView.setText(formatRuntimeSnapshot(snapshot, selectedLevel));
                }
            });
        }, "rafcodephi-contract-apk-inspection").start();
    }

    private RuntimeSnapshot inspectInstalledApk() {
        RuntimeSnapshot snapshot = new RuntimeSnapshot();
        snapshot.packageName = getPackageName();
        snapshot.supportedAbis = Arrays.asList(Build.SUPPORTED_ABIS);

        try {
            PackageInfo info = getPackageManager().getPackageInfo(getPackageName(), 0);
            snapshot.versionName = value(info.versionName);
        } catch (Exception e) {
            snapshot.versionName = "TOKEN_VAZIO";
            snapshot.errors.add("packageInfo=" + e.getClass().getSimpleName());
        }

        try {
            File apk = new File(getApplicationInfo().sourceDir);
            snapshot.sourcePath = apk.getAbsolutePath();
            snapshot.apkBytes = apk.length();
            snapshot.sha256 = sha256(apk);

            try (ZipFile zip = new ZipFile(apk)) {
                java.util.Enumeration<? extends ZipEntry> entries = zip.entries();
                while (entries.hasMoreElements()) {
                    String name = entries.nextElement().getName();
                    if (name.matches("classes(\\d*)?\\.dex")) snapshot.dexCount++;
                    if (name.startsWith("lib/") && name.endsWith(".so")) {
                        snapshot.nativeLibraryCount++;
                        String[] parts = name.split("/");
                        if (parts.length >= 3) snapshot.packagedAbis.add(parts[1]);
                    }
                }
            }
        } catch (Exception e) {
            snapshot.errors.add("apkInspection=" + e.getClass().getSimpleName());
        }

        return snapshot;
    }

    private String formatRuntimeSnapshot(RuntimeSnapshot s, String level) {
        StringBuilder out = new StringBuilder();
        out.append("Pacote: ").append(s.packageName)
            .append("\nVersão: ").append(s.versionName)
            .append("\nDEX encontrados: ").append(s.dexCount)
            .append("\nBibliotecas nativas: ").append(s.nativeLibraryCount)
            .append("\nABIs empacotadas: ").append(s.packagedAbis.isEmpty() ? "TOKEN_VAZIO" : s.packagedAbis);

        if (!"leigo".equals(level)) {
            out.append("\nABIs suportadas pelo dispositivo: ").append(s.supportedAbis);
        }
        if (isTechnicalLevel(level)) {
            out.append("\nAPK bytes: ").append(s.apkBytes)
                .append("\nSHA-256: ").append(s.sha256)
                .append("\nsourceDir: ").append(s.sourcePath);
        }
        if (!s.errors.isEmpty()) out.append("\nLacunas da coleta: ").append(s.errors);
        return out.toString();
    }

    private String sha256(File file) throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        byte[] buffer = new byte[64 * 1024];
        try (FileInputStream input = new FileInputStream(file)) {
            int read;
            while ((read = input.read(buffer)) != -1) digest.update(buffer, 0, read);
        }
        StringBuilder hex = new StringBuilder();
        for (byte b : digest.digest()) hex.append(String.format(Locale.US, "%02x", b));
        return hex.toString();
    }

    private TextView text(String value, float sp, boolean bold) {
        TextView view = new TextView(this);
        view.setText(value(value));
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        if (bold) view.setTypeface(view.getTypeface(), android.graphics.Typeface.BOLD);
        view.setPadding(0, dp(4), 0, dp(4));
        return view;
    }

    private void setTopMargin(View view, int marginDp) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);
        params.topMargin = dp(marginDp);
        view.setLayoutParams(params);
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private static String value(String value) {
        return value == null ? "TOKEN_VAZIO" : value;
    }

    private static final class Capability {
        String id = "TOKEN_VAZIO";
        String title = "TOKEN_VAZIO";
        String status = "TOKEN_VAZIO";
        String claimPolicy = "no_claim";
        String source = "TOKEN_VAZIO";
        String executor = "TOKEN_VAZIO";
        String evidence = "TOKEN_VAZIO";
        String gap = "TOKEN_VAZIO";
        final Map<String, String> levels = new LinkedHashMap<>();
    }

    private static final class RuntimeSnapshot {
        String packageName = "TOKEN_VAZIO";
        String versionName = "TOKEN_VAZIO";
        String sourcePath = "TOKEN_VAZIO";
        String sha256 = "TOKEN_VAZIO";
        long apkBytes = 0;
        int dexCount = 0;
        int nativeLibraryCount = 0;
        List<String> supportedAbis = new ArrayList<>();
        final Set<String> packagedAbis = new LinkedHashSet<>();
        final List<String> errors = new ArrayList<>();
    }
}
