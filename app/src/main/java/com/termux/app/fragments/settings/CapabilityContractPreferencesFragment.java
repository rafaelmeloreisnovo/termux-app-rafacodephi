package com.termux.app.fragments.settings;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.res.XmlResourceParser;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceScreen;

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
 * Read-only presentation of the RAFCODEPhi capability contract.
 *
 * One technical truth is rendered at multiple depths:
 * Leigo -> Basico -> Medio -> Avancado -> Expert -> So para Nerds.
 *
 * Contract invariant:
 * SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM.
 */
public class CapabilityContractPreferencesFragment extends PreferenceFragmentCompat {

    private static final String[] LEVEL_IDS = {
        "leigo", "basico", "medio", "avancado", "expert", "nerd"
    };

    private static final CharSequence[] LEVEL_LABELS = {
        "Leigo", "Básico", "Médio", "Avançado", "Expert", "Só para Nerds"
    };

    private final List<Capability> capabilities = new ArrayList<>();
    private final List<Preference> capabilityPreferences = new ArrayList<>();

    private String selectedLevel = "leigo";
    private Preference runtimeEvidencePreference;
    private RuntimeSnapshot runtimeSnapshot;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Context context = getContext();
        if (context == null) return;

        capabilities.clear();
        capabilities.addAll(loadContract(context));

        PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(context);
        setPreferenceScreen(screen);

        Preference intro = new Preference(context);
        intro.setTitle("RAFCODEΦ · Conheça e Comprove");
        intro.setSummary(
            "Uma verdade técnica, várias profundidades de leitura. A linguagem muda; a evidência não.\n\n" +
            "SOURCE ≠ ARTIFACT ≠ EXECUTION ≠ EVIDENCE ≠ CLAIM\n" +
            "IMPLEMENTED_UNTESTED ≠ PASS · TOKEN_VAZIO é estado válido.");
        intro.setSelectable(false);
        screen.addPreference(intro);

        ListPreference level = new ListPreference(context);
        level.setKey("rafcodephi_contract_level_session");
        level.setPersistent(false);
        level.setTitle("Nível de leitura");
        level.setEntries(LEVEL_LABELS);
        level.setEntryValues(LEVEL_IDS);
        level.setValue(selectedLevel);
        level.setSummary(labelForLevel(selectedLevel));
        level.setOnPreferenceChangeListener((preference, newValue) -> {
            selectedLevel = String.valueOf(newValue);
            level.setSummary(labelForLevel(selectedLevel));
            renderCapabilities();
            renderRuntimeEvidence();
            return true;
        });
        screen.addPreference(level);

        PreferenceCategory evidenceCategory = new PreferenceCategory(context);
        evidenceCategory.setTitle("Evidência desta instalação");
        screen.addPreference(evidenceCategory);

        runtimeEvidencePreference = new Preference(context);
        runtimeEvidencePreference.setTitle("Inspeção local do APK em execução");
        runtimeEvidencePreference.setSummary("Coletando somente evidência local e de leitura do APK instalado…");
        runtimeEvidencePreference.setSelectable(false);
        evidenceCategory.addPreference(runtimeEvidencePreference);

        PreferenceCategory capabilityCategory = new PreferenceCategory(context);
        capabilityCategory.setTitle("Capacidades / métodos");
        screen.addPreference(capabilityCategory);

        capabilityPreferences.clear();
        for (Capability capability : capabilities) {
            Preference preference = new Preference(context);
            preference.setKey("rafcodephi_contract_" + capability.id);
            preference.setTitle(capability.title);
            preference.setSelectable(false);
            capabilityCategory.addPreference(preference);
            capabilityPreferences.add(preference);
        }

        renderCapabilities();
        collectRuntimeEvidenceAsync(context.getApplicationContext());
    }

    private void renderCapabilities() {
        for (int i = 0; i < capabilities.size() && i < capabilityPreferences.size(); i++) {
            Capability capability = capabilities.get(i);
            StringBuilder summary = new StringBuilder();
            summary.append("Estado: ").append(capability.status)
                .append(" · claim: ").append(capability.claimPolicy)
                .append("\n").append(capability.levels.getOrDefault(selectedLevel, "TOKEN_VAZIO"));

            if (isTechnicalLevel(selectedLevel)) {
                summary.append("\n\nSOURCE: ").append(capability.source)
                    .append("\nEXECUTOR: ").append(capability.executor)
                    .append("\nEVIDENCE: ").append(capability.evidence)
                    .append("\nGAP: ").append(capability.gap);
            } else if ("basico".equals(selectedLevel) || "medio".equals(selectedLevel)) {
                summary.append("\n\nComo conferir: ").append(capability.evidence)
                    .append("\nAinda não prova: ").append(capability.gap);
            }

            capabilityPreferences.get(i).setSummary(summary.toString());
        }
    }

    private boolean isTechnicalLevel(String level) {
        return "avancado".equals(level) || "expert".equals(level) || "nerd".equals(level);
    }

    private String labelForLevel(String level) {
        for (int i = 0; i < LEVEL_IDS.length; i++) {
            if (LEVEL_IDS[i].equals(level)) return LEVEL_LABELS[i].toString();
        }
        return level;
    }

    private List<Capability> loadContract(@NonNull Context context) {
        List<Capability> result = new ArrayList<>();
        XmlResourceParser parser = context.getResources().getXml(R.xml.rafcodephi_capability_contract);
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
            for (String level : LEVEL_IDS) {
                failure.levels.put(level, "Falha de leitura: " + e.getClass().getSimpleName());
            }
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

    private void collectRuntimeEvidenceAsync(@NonNull Context appContext) {
        new Thread(() -> {
            RuntimeSnapshot snapshot = inspectInstalledApk(appContext);
            if (!isAdded()) return;
            requireActivity().runOnUiThread(() -> {
                if (!isAdded()) return;
                runtimeSnapshot = snapshot;
                renderRuntimeEvidence();
            });
        }, "rafcodephi-contract-apk-inspection").start();
    }

    private RuntimeSnapshot inspectInstalledApk(@NonNull Context context) {
        RuntimeSnapshot snapshot = new RuntimeSnapshot();
        snapshot.packageName = context.getPackageName();
        snapshot.supportedAbis = Arrays.asList(Build.SUPPORTED_ABIS);

        try {
            PackageInfo info = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            snapshot.versionName = value(info.versionName);
        } catch (Exception e) {
            snapshot.versionName = "TOKEN_VAZIO";
            snapshot.errors.add("packageInfo=" + e.getClass().getSimpleName());
        }

        try {
            File apk = new File(context.getApplicationInfo().sourceDir);
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

    private void renderRuntimeEvidence() {
        if (runtimeEvidencePreference == null || runtimeSnapshot == null) return;
        RuntimeSnapshot s = runtimeSnapshot;
        StringBuilder out = new StringBuilder();
        out.append("Pacote: ").append(s.packageName)
            .append("\nVersão: ").append(s.versionName)
            .append("\nDEX encontrados: ").append(s.dexCount)
            .append("\nBibliotecas nativas: ").append(s.nativeLibraryCount)
            .append("\nABIs empacotadas: ").append(s.packagedAbis.isEmpty() ? "TOKEN_VAZIO" : s.packagedAbis);

        if (!"leigo".equals(selectedLevel)) {
            out.append("\nABIs suportadas pelo dispositivo: ").append(s.supportedAbis);
        }
        if (isTechnicalLevel(selectedLevel)) {
            out.append("\nAPK bytes: ").append(s.apkBytes)
                .append("\nSHA-256: ").append(s.sha256)
                .append("\nsourceDir: ").append(s.sourcePath);
        }
        if (!s.errors.isEmpty()) out.append("\nLacunas da coleta: ").append(s.errors);
        runtimeEvidencePreference.setSummary(out.toString());
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
