package com.termux.app;

import android.system.Os;
import android.util.Base64;

import com.termux.shared.logger.Logger;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/** Installs the exact PEDRA_ANGULAR payload embedded in the RAFCODEPhi APK. */
final class PaPayloadInstaller {
    private static final String LOG_TAG = "PaPayloadInstaller";
    private static final Object INSTALL_LOCK = new Object();
    private static final String PAYLOAD_SHA256 = "1655dc886b549a006e28553ddf3e76dcee9d4838956aa81a1cac4f56b594a08f";
    private static final String EMBEDDED_DIR_NAME = "rafcodephi-pa-v1";
    private static final int MAX_ENTRIES = 512;
    private static final long MAX_EXTRACTED_BYTES = 16L * 1024L * 1024L;


    private PaPayloadInstaller() {}

    static void ensureInstalled(String prefixPath) {
        synchronized (INSTALL_LOCK) {
            try {
                install(prefixPath);
            } catch (Throwable t) {
                throw new RuntimeException("PA embedded payload installation failed", t);
            }
        }
    }

    private static void install(String prefixPath) throws Exception {
        if (prefixPath == null || prefixPath.trim().isEmpty())
            throw new IOException("empty prefix path");

        File prefix = new File(prefixPath).getCanonicalFile();
        File bash = new File(prefix, "bin/bash");
        if (!bash.isFile()) throw new IOException("missing bootstrap bash: " + bash);

        File libexec = new File(prefix, "libexec");
        ensureDirectory(libexec, 0700);
        File embeddedRoot = new File(libexec, EMBEDDED_DIR_NAME);
        File marker = new File(embeddedRoot, ".rafcodephi-pa.sha256");
        File embeddedEntry = new File(embeddedRoot, "bin/pa");
        File wrapper = new File(prefix, "bin/pa");

        if (embeddedEntry.isFile() && PAYLOAD_SHA256.equals(readSmallText(marker))) {
            writeWrapper(prefix, embeddedRoot, wrapper);
            Logger.logInfo(LOG_TAG, "state=present sha256=" + PAYLOAD_SHA256 + " root=" + embeddedRoot);
            return;
        }

        byte[] payload = decodePayload();
        String observed = sha256(payload);
        if (!PAYLOAD_SHA256.equals(observed))
            throw new IOException("embedded PA digest mismatch expected=" + PAYLOAD_SHA256 + " observed=" + observed);

        File staging = new File(libexec, EMBEDDED_DIR_NAME + ".staging");
        deleteOwnedTree(staging);
        ensureDirectory(staging, 0700);
        extractSafely(payload, staging, prefixPath);
        writeText(new File(staging, ".rafcodephi-pa.sha256"), PAYLOAD_SHA256 + "\n", 0600);

        deleteOwnedTree(embeddedRoot);
        if (!staging.renameTo(embeddedRoot))
            throw new IOException("could not promote PA staging directory to " + embeddedRoot);

        writeWrapper(prefix, embeddedRoot, wrapper);
        Logger.logInfo(LOG_TAG, "state=installed sha256=" + PAYLOAD_SHA256 +
            " bytes=" + payload.length + " root=" + embeddedRoot +
            " user_override=$HOME/PEDRA_ANGULAR");
    }

    private static String[] payloadChunks() {
        return new String[] {
            PaPayloadChunk00.VALUE,
            PaPayloadChunk01.VALUE,
            PaPayloadChunk02.VALUE,
            PaPayloadChunk03.VALUE,
            PaPayloadChunk04.VALUE,
            PaPayloadChunk05.VALUE,
            PaPayloadChunk06.VALUE,
            PaPayloadChunk07.VALUE,
            PaPayloadChunk08.VALUE,
            PaPayloadChunk09.VALUE,
            PaPayloadChunk10.VALUE
        };
    }

    private static byte[] decodePayload() throws IOException {
        StringBuilder encoded = new StringBuilder(65000);
        for (String chunk : payloadChunks()) encoded.append(chunk);
        byte[] decoded = Base64.decode(encoded.toString(), Base64.NO_WRAP);
        if (decoded.length != 48366)
            throw new IOException("unexpected PA payload size: " + decoded.length);
        return decoded;
    }

    private static void extractSafely(byte[] payload, File staging, String prefixPath) throws Exception {
        String canonicalRoot = staging.getCanonicalPath() + File.separator;
        int entries = 0;
        long extracted = 0;
        byte[] buffer = new byte[8192];

        try (ZipInputStream zip = new ZipInputStream(new ByteArrayInputStream(payload))) {
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (++entries > MAX_ENTRIES) throw new IOException("PA zip entry limit exceeded");
                String name = validateEntryName(entry.getName());
                File target = new File(staging, name);
                String canonicalTarget = target.getCanonicalPath();
                if (!canonicalTarget.equals(staging.getCanonicalPath()) && !canonicalTarget.startsWith(canonicalRoot))
                    throw new IOException("PA zip entry escaped root: " + name);

                if (entry.isDirectory()) {
                    ensureDirectory(target, 0700);
                    continue;
                }
                ensureDirectory(target.getParentFile(), 0700);
                ByteArrayOutputStream bytes = new ByteArrayOutputStream();
                int read;
                while ((read = zip.read(buffer)) != -1) {
                    extracted += read;
                    if (extracted > MAX_EXTRACTED_BYTES)
                        throw new IOException("PA zip extracted byte limit exceeded");
                    bytes.write(buffer, 0, read);
                }
                byte[] fileBytes = patchPortableText(name, bytes.toByteArray(), prefixPath);
                try (FileOutputStream out = new FileOutputStream(target)) {
                    out.write(fileBytes);
                    out.getFD().sync();
                }
                Os.chmod(target.getAbsolutePath(), isExecutable(name) ? 0700 : 0600);
            }
        }
        if (entries != 84) throw new IOException("unexpected PA zip entry count: " + entries);
    }

    private static String validateEntryName(String value) throws IOException {
        if (value == null || value.isEmpty() || value.startsWith("/") || value.startsWith("\\") ||
            value.indexOf('\0') >= 0 || value.indexOf('\\') >= 0)
            throw new IOException("unsafe PA zip entry name");
        String[] parts = value.split("/");
        for (String part : parts) {
            if (part.equals("..") || part.equals("."))
                throw new IOException("unsafe PA zip path segment: " + value);
        }
        return value;
    }

    private static byte[] patchPortableText(String name, byte[] input, String prefixPath) {
        if (!isPortableScript(name)) return input;
        String text = new String(input, StandardCharsets.UTF_8);
        text = text.replace("#!/data/data/com.termux/files/usr/bin/bash", "#!" + prefixPath + "/bin/bash");
        text = text.replace("PA=\"$HOME/PEDRA_ANGULAR\"", "PA=\"${PA:-$HOME/PEDRA_ANGULAR}\"");
        text = text.replace("export PA=\"$HOME/PEDRA_ANGULAR\"", "export PA=\"${PA:-$HOME/PEDRA_ANGULAR}\"");
        return text.getBytes(StandardCharsets.UTF_8);
    }

    private static boolean isPortableScript(String name) {
        return name.equals(".env") || name.equals("bin/pa") || name.equals("scripts/pa") ||
            name.equals("asm/build.sh") || (name.startsWith("scripts/") && name.endsWith(".sh"));
    }

    private static boolean isExecutable(String name) {
        return name.equals("bin/pa") || name.equals("scripts/pa") || name.equals("asm/build.sh") ||
            (name.startsWith("scripts/") && name.endsWith(".sh")) ||
            name.startsWith("c/bin/") || name.startsWith("asm/bin/") ||
            name.equals("rust/projects/state_vec/vectra_neon");
    }

    private static void writeWrapper(File prefix, File embeddedRoot, File wrapper) throws Exception {
        File home = new File(prefix.getParentFile(), "home");
        String script = "#!" + new File(prefix, "bin/sh").getAbsolutePath() + "\n" +
            "USER_PA=\"" + new File(home, "PEDRA_ANGULAR").getAbsolutePath() + "\"\n" +
            "EMBEDDED_PA=\"" + embeddedRoot.getAbsolutePath() + "\"\n" +
            "if [ -f \"$USER_PA/bin/pa\" ]; then PA=\"$USER_PA\"; else PA=\"$EMBEDDED_PA\"; fi\n" +
            "export PA\n" +
            "exec \"" + new File(prefix, "bin/bash").getAbsolutePath() + "\" \"$PA/bin/pa\" \"$@\"\n";
        writeText(wrapper, script, 0700);
    }

    private static void ensureDirectory(File directory, int mode) throws Exception {
        if (directory == null) throw new IOException("null directory");
        if (directory.exists() && !directory.isDirectory())
            throw new IOException("path is not a directory: " + directory);
        if (!directory.exists() && !directory.mkdirs() && !directory.isDirectory())
            throw new IOException("could not create directory: " + directory);
        Os.chmod(directory.getAbsolutePath(), mode);
    }

    private static void writeText(File target, String text, int mode) throws Exception {
        ensureDirectory(target.getParentFile(), 0700);
        File temp = new File(target.getParentFile(), target.getName() + ".tmp");
        try (FileOutputStream out = new FileOutputStream(temp)) {
            out.write(text.getBytes(StandardCharsets.UTF_8));
            out.getFD().sync();
        }
        Os.chmod(temp.getAbsolutePath(), mode);
        if (target.exists() && !target.delete()) throw new IOException("could not replace " + target);
        if (!temp.renameTo(target)) throw new IOException("could not publish " + target);
        Os.chmod(target.getAbsolutePath(), mode);
    }

    private static String readSmallText(File file) {
        if (file == null || !file.isFile() || file.length() > 256) return "";
        byte[] data = new byte[(int) file.length()];
        try (FileInputStream in = new FileInputStream(file)) {
            int offset = 0;
            while (offset < data.length) {
                int read = in.read(data, offset, data.length - offset);
                if (read < 0) break;
                offset += read;
            }
            return new String(data, 0, offset, StandardCharsets.UTF_8).trim();
        } catch (IOException e) {
            return "";
        }
    }

    private static String sha256(byte[] data) throws Exception {
        byte[] digest = MessageDigest.getInstance("SHA-256").digest(data);
        StringBuilder out = new StringBuilder(64);
        for (byte value : digest) out.append(String.format(Locale.ROOT, "%02x", value & 0xff));
        return out.toString();
    }

    private static void deleteOwnedTree(File file) throws IOException {
        if (file == null || !file.exists()) return;
        File[] children = file.listFiles();
        if (children != null) for (File child : children) deleteOwnedTree(child);
        if (!file.delete()) throw new IOException("could not delete owned path: " + file);
    }
}
