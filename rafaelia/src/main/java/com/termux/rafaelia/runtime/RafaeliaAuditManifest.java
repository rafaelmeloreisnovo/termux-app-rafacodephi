package com.termux.rafaelia.runtime;

import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.security.MessageDigest;

/**
 * Gera manifesto assinado por hash (SHA-256) para artefatos de auditoria.
 */
public final class RafaeliaAuditManifest {

    private RafaeliaAuditManifest() {}

    public static JSONObject build(String buildId, File auditJson, File auditSvg) throws Exception {
        JSONObject root = new JSONObject();
        root.put("buildId", buildId == null ? "unknown" : buildId);
        root.put("auditJsonPath", auditJson == null ? "" : auditJson.getAbsolutePath());
        root.put("auditSvgPath", auditSvg == null ? "" : auditSvg.getAbsolutePath());
        root.put("auditJsonSha256", auditJson == null ? "" : sha256Hex(auditJson));
        root.put("auditSvgSha256", auditSvg == null ? "" : sha256Hex(auditSvg));
        root.put("merkleLikeRoot", sha256Text(
            root.optString("auditJsonSha256", "") + "|" + root.optString("auditSvgSha256", "")));
        return root;
    }

    private static String sha256Hex(File file) throws Exception {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        byte[] buf = new byte[8192];
        try (FileInputStream fis = new FileInputStream(file)) {
            int n;
            while ((n = fis.read(buf)) > 0) {
                md.update(buf, 0, n);
            }
        }
        return hex(md.digest());
    }

    private static String sha256Text(String text) throws Exception {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        md.update(text.getBytes(java.nio.charset.StandardCharsets.UTF_8));
        return hex(md.digest());
    }

    private static String hex(byte[] bytes) {
        StringBuilder sb = new StringBuilder(bytes.length * 2);
        for (byte b : bytes) sb.append(String.format("%02x", b));
        return sb.toString();
    }
}
