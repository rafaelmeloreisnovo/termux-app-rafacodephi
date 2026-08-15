package com.termux.app;

import org.junit.Test;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;

import static org.junit.Assert.assertTrue;

public class RafSensorIntegrationContractTest {

    @Test
    public void manifestDeclaresTypedSensorPermissionServiceAndActivity() throws Exception {
        String manifest = read("app/src/main/AndroidManifest.xml");

        assertTrue(manifest.contains(".permission.RAF_SENSOR_ACCESS"));
        assertTrue(manifest.contains("com.termux.app.api.sensor.RafSensorApiService"));
        assertTrue(manifest.contains("com.termux.app.activities.VectraRuntimeActivity"));
    }

    @Test
    public void settingsExposeControlCenterAndVectraRecoveryEntries() throws Exception {
        String rootPreferences = read("app/src/main/res/xml/root_preferences.xml");
        String settingsActivity = read("app/src/main/java/com/termux/app/activities/SettingsActivity.java");

        assertTrue(rootPreferences.contains("app:key=\"rafcodephi_control_center\""));
        assertTrue(rootPreferences.contains("BetaOrchestratorActivity"));
        assertTrue(settingsActivity.contains("findPreference(\"rafcodephi_control_center\")"));
        assertTrue(settingsActivity.contains("configureRafcodephiControlCenterPreference"));
        assertTrue(settingsActivity.contains("BetaOrchestratorActivity.class"));
        assertTrue(rootPreferences.contains("app:key=\"vectra_runtime\""));
        assertTrue(settingsActivity.contains("findPreference(\"vectra_runtime\")"));
        assertTrue(settingsActivity.contains("VectraRuntimeActivity.class"));
    }

    private static String read(String path) throws Exception {
        java.nio.file.Path candidate = Paths.get(path);
        if (!Files.exists(candidate)) {
            candidate = Paths.get("..").resolve(path);
        }
        return new String(Files.readAllBytes(candidate), StandardCharsets.UTF_8);
    }
}
