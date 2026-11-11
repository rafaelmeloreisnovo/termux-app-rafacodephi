package com.termux.app;

import com.termux.shared.termux.shell.command.runner.terminal.BootstrapElfApi;

import org.junit.Test;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.Assert.assertEquals;

/** Host-side regression for Android10 linker-incompatible login shell selection. */
public class BootstrapElfApiTest {

    private File elfWithAndroidNote(int api) throws Exception {
        byte[] bytes = new byte[1024];
        bytes[0] = 0x7f;
        bytes[1] = 'E'; bytes[2] = 'L'; bytes[3] = 'F';
        bytes[4] = 1; bytes[5] = 1;
        int p = 468;
        bytes[p - 12] = 8; bytes[p - 8] = (byte) 0x84;
        bytes[p - 4] = 1;
        byte[] owner = "Android\0".getBytes(StandardCharsets.UTF_8);
        System.arraycopy(owner, 0, bytes, p, owner.length);
        bytes[p + 8] = (byte) api;
        bytes[p + 9] = (byte) (api >>> 8);
        Path target = Files.createTempFile("rafcodephi-login-shell-api-", ".elf");
        Files.write(target, bytes);
        target.toFile().deleteOnExit();
        return target.toFile();
    }

    @Test public void api30IsDetectedBeforeAndroid10AttemptsToLinkBash() throws Exception {
        assertEquals(30, BootstrapElfApi.minimumAndroidApi(elfWithAndroidNote(30)));
    }

    @Test public void compatibleApi29RemainsSelectable() throws Exception {
        assertEquals(29, BootstrapElfApi.minimumAndroidApi(elfWithAndroidNote(29)));
    }

    @Test public void oldSystemShScriptWrapperRemainsSelectable() throws Exception {
        Path path = Files.createTempFile("rafcodephi-login-shell-", ".sh");
        Files.write(path, "#!/system/bin/sh\n".getBytes(StandardCharsets.UTF_8));
        assertEquals(0, BootstrapElfApi.minimumAndroidApi(path.toFile()));
    }

    @Test public void unknownElfNoteDoesNotPretendToBeCompatibleProof() throws Exception {
        byte[] bytes = new byte[64];
        bytes[0] = 0x7f;
        bytes[1] = 'E'; bytes[2] = 'L'; bytes[3] = 'F';
        bytes[5] = 1;
        Path path = Files.createTempFile("rafcodephi-login-shell-unknown-", ".elf");
        Files.write(path, bytes);
        assertEquals(-1, BootstrapElfApi.minimumAndroidApi(path.toFile()));
    }
}
