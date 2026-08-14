package com.termux.app.orchestration;

import android.content.Context;

import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.HashSet;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/** Creates one portable evidence bundle without exporting unrelated Termux user files. */
public final class ControlCenterEvidenceBundle {

    private ControlCenterEvidenceBundle() {}

    public static int write(Context context, OutputStream destination, String bootstrapSnapshot,
                            String runtimeSnapshot, JSONObject latestReceipt) throws Exception {
        int[] entries = {0};
        Set<String> emitted = new HashSet<>();
        try (ZipOutputStream zip = new ZipOutputStream(destination)) {
            putText(zip, emitted, "00-control-center/README.txt",
                "RAFCODEPHI CONTROL CENTER EVIDENCE BUNDLE V1\n"
                    + "scope=bootstrap+package-runtime+vectra+beta-orchestrator\n"
                    + "unrelated_termux_home_files=NOT_EXPORTED\n"
                    + "claim_allowed_release=false\n"
                    + "TOKEN_VAZIO remains a valid evidence state.\n", entries);
            putText(zip, emitted, "00-control-center/bootstrap-readiness.txt", bootstrapSnapshot, entries);
            putText(zip, emitted, "00-control-center/runtime-vectra-snapshot.txt", runtimeSnapshot, entries);
            if (latestReceipt != null) {
                putText(zip, emitted, "00-control-center/latest-orchestrator-receipt.json",
                    latestReceipt.toString() + "\n", entries);
            }

            File canonical = new File(context.getFilesDir(), "rafcodephi-beta-orchestrator");
            addTree(zip, emitted, canonical, "beta-orchestrator-private", entries);

            File external = context.getExternalFilesDir("beta-evidence");
            addTree(zip, emitted, external, "beta-evidence-app-specific", entries);

            putText(zip, emitted, "00-control-center/MANIFEST.txt",
                "schema=rafcodephi.control-center-export/v1\n"
                    + "entries_before_manifest=" + entries[0] + "\n"
                    + "canonical_private_root=" + canonical.getAbsolutePath() + "\n"
                    + "app_specific_external_root=" + (external == null ? "UNAVAILABLE" : external.getAbsolutePath()) + "\n"
                    + "export_authority=copy_only\n"
                    + "claim_allowed_release=false\n", entries);
            zip.finish();
        }
        return entries[0];
    }

    private static void addTree(ZipOutputStream zip, Set<String> emitted, File root,
                                String prefix, int[] entries) throws Exception {
        if (root == null || !root.exists()) return;
        String rootCanonical = root.getCanonicalPath();
        addTreeRecursive(zip, emitted, root, rootCanonical, prefix, entries);
    }

    private static void addTreeRecursive(ZipOutputStream zip, Set<String> emitted, File file,
                                         String rootCanonical, String prefix, int[] entries) throws Exception {
        String canonical = file.getCanonicalPath();
        if (!canonical.equals(rootCanonical) && !canonical.startsWith(rootCanonical + File.separator)) {
            throw new SecurityException("evidence_path_escape");
        }
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children == null) return;
            for (File child : children) addTreeRecursive(zip, emitted, child, rootCanonical, prefix, entries);
            return;
        }
        if (!file.isFile()) return;
        String relative = canonical.equals(rootCanonical) ? file.getName()
            : canonical.substring(rootCanonical.length() + 1).replace(File.separatorChar, '/');
        String name = sanitizeEntry(prefix + "/" + relative);
        if (!emitted.add(name)) return;
        zip.putNextEntry(new ZipEntry(name));
        try (BufferedInputStream in = new BufferedInputStream(new FileInputStream(file))) {
            byte[] buffer = new byte[32768];
            int read;
            while ((read = in.read(buffer)) >= 0) {
                if (read > 0) zip.write(buffer, 0, read);
            }
        }
        zip.closeEntry();
        entries[0]++;
    }

    private static void putText(ZipOutputStream zip, Set<String> emitted, String name,
                                String value, int[] entries) throws Exception {
        name = sanitizeEntry(name);
        if (!emitted.add(name)) return;
        zip.putNextEntry(new ZipEntry(name));
        byte[] data = (value == null ? "TOKEN_VAZIO\n" : value).getBytes(StandardCharsets.UTF_8);
        zip.write(data);
        zip.closeEntry();
        entries[0]++;
    }

    private static String sanitizeEntry(String name) {
        String value = name.replace('\\', '/');
        while (value.startsWith("/")) value = value.substring(1);
        if (value.contains("../") || value.equals("..")) throw new IllegalArgumentException("unsafe_zip_entry");
        return value;
    }
}
