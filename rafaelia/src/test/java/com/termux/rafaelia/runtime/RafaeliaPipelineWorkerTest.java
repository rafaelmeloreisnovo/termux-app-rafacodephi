package com.termux.rafaelia.runtime;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class RafaeliaPipelineWorkerTest {

    @Test
    public void exportAuditJson_hasSchemaAndRates() throws Exception {
        RafaeliaPipelineWorker worker = new RafaeliaPipelineWorker();
        String json = worker.exportAuditJson();
        JSONObject root = new JSONObject(json);

        assertEquals(1, root.getInt("schemaVersion"));
        assertEquals(0.0, root.getDouble("commitRate"), 0.0);
        assertEquals(0.0, root.getDouble("rollbackRate"), 0.0);
        assertTrue(root.getJSONArray("phiWindow").length() == 42);
    }
}
