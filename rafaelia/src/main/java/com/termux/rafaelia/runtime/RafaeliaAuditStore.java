package com.termux.rafaelia.runtime;

import android.content.Context;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

/** Persistência local de audit_json para comparação entre builds. */
public final class RafaeliaAuditStore {

    private RafaeliaAuditStore() {}

    public static File saveAuditJson(Context context, String buildId, String auditJson) throws Exception {
        File dir = new File(context.getFilesDir(), "rafaelia-audit");
        if (!dir.exists() && !dir.mkdirs()) {
            throw new IllegalStateException("failed to create audit dir");
        }
        String safeBuild = (buildId == null || buildId.isEmpty()) ? "unknown" : buildId.replaceAll("[^a-zA-Z0-9._-]", "_");
        File out = new File(dir, "audit_" + safeBuild + ".json");
        try (FileOutputStream fos = new FileOutputStream(out, false)) {
            fos.write((auditJson == null ? "{}" : auditJson).getBytes(StandardCharsets.UTF_8));
        }
        return out;
    }
}
