package com.termux.rafaelia.runtime;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.work.Worker;
import androidx.work.WorkerParameters;

import org.json.JSONObject;

/**
 * Worker minimalista para emulador/dispositivo: processa batch e retorna auditoria JSON.
 */
public class RafaeliaBatchWorker extends Worker {

    public static final String KEY_PAYLOAD = "payload";
    public static final String KEY_ITERATIONS = "iterations";
    public static final String KEY_AUDIT_JSON = "audit_json";

    public RafaeliaBatchWorker(@NonNull Context context, @NonNull WorkerParameters params) {
        super(context, params);
    }

    @NonNull
    @Override
    public Result doWork() {
        try {
            String payload = getInputData().getString(KEY_PAYLOAD);
            int iterations = getInputData().getInt(KEY_ITERATIONS, 42);
            if (payload == null) payload = "rafaelia";
            if (iterations <= 0) iterations = 1;

            RafaeliaPipelineWorker pipeline = new RafaeliaPipelineWorker();
            byte[] bytes = payload.getBytes();
            for (int i = 0; i < iterations; i++) {
                pipeline.process(bytes, bytes.length);
            }

            String auditJson = pipeline.exportAuditJson();
            return Result.success(new androidx.work.Data.Builder().putString(KEY_AUDIT_JSON, auditJson).build());
        } catch (Exception e) {
            return Result.failure();
        }
    }
}
