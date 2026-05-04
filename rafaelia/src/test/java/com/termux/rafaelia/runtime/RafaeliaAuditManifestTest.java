package com.termux.rafaelia.runtime;

import org.json.JSONObject;
import org.junit.Test;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class RafaeliaAuditManifestTest {

    @Test
    public void build_createsHashesAndRoot() throws Exception {
        File json = File.createTempFile("audit", ".json");
        File svg = File.createTempFile("audit", ".svg");
        try (FileOutputStream fos = new FileOutputStream(json)) {
            fos.write("{\"a\":1}".getBytes(StandardCharsets.UTF_8));
        }
        try (FileOutputStream fos = new FileOutputStream(svg)) {
            fos.write("<svg/>".getBytes(StandardCharsets.UTF_8));
        }

        JSONObject root = RafaeliaAuditManifest.build("b1", json, svg);
        assertTrue(root.getString("auditJsonSha256").length() == 64);
        assertTrue(root.getString("auditSvgSha256").length() == 64);
        assertFalse(root.getString("merkleLikeRoot").isEmpty());
    }
}
