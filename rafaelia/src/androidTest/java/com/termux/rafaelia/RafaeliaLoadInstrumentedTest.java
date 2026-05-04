package com.termux.rafaelia;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;

import com.termux.rafaelia.runtime.RafaeliaPipelineWorker;

import org.json.JSONObject;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class RafaeliaLoadInstrumentedTest {

    @Test
    public void loadRunKeepsCycleAndProducesAuditJson() throws Exception {
        RafaeliaPipelineWorker w = new RafaeliaPipelineWorker();
        byte[] payload = new byte[]{7,11,13,17,19};
        int cycles = 84; // 2 períodos
        for (int i = 0; i < cycles; i++) {
            w.process(payload, payload.length);
        }

        String json = w.exportAuditJson();
        JSONObject o = new JSONObject(json);
        assertTrue(o.has("commitRate"));
        assertTrue(o.getJSONArray("phiWindow").length() == 42);
        assertTrue(RafaeliaCore.getCurrentCycle() >= 0 && RafaeliaCore.getCurrentCycle() < 42);
    }
}
