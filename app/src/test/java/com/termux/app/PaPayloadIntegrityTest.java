package com.termux.app;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.security.MessageDigest;
import java.util.Base64;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public final class PaPayloadIntegrityTest {
    private static final String EXPECTED_SHA256 = "1655dc886b549a006e28553ddf3e76dcee9d4838956aa81a1cac4f56b594a08f";

    @Test
    public void embeddedPayloadMatchesExactUploadedZip() throws Exception {
        String encoded = PaPayloadChunk00.VALUE + PaPayloadChunk01.VALUE + PaPayloadChunk02.VALUE +
            PaPayloadChunk03.VALUE + PaPayloadChunk04.VALUE + PaPayloadChunk05.VALUE +
            PaPayloadChunk06.VALUE + PaPayloadChunk07.VALUE + PaPayloadChunk08.VALUE +
            PaPayloadChunk09.VALUE + PaPayloadChunk10.VALUE;
        byte[] payload = Base64.getDecoder().decode(encoded);
        assertEquals(48366, payload.length);
        assertEquals(EXPECTED_SHA256, sha256(payload));

        int entries = 0;
        try (ZipInputStream zip = new ZipInputStream(new ByteArrayInputStream(payload))) {
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) entries++;
        }
        assertEquals(84, entries);
    }

    private static String sha256(byte[] data) throws Exception {
        byte[] digest = MessageDigest.getInstance("SHA-256").digest(data);
        StringBuilder value = new StringBuilder(64);
        for (byte item : digest) value.append(String.format(Locale.ROOT, "%02x", item & 0xff));
        return value.toString();
    }
}
