package com.termux.rafaelia.runtime;

import android.content.Context;

import androidx.work.Constraints;
import androidx.work.Data;
import androidx.work.ExistingPeriodicWorkPolicy;
import androidx.work.NetworkType;
import androidx.work.PeriodicWorkRequest;
import androidx.work.WorkManager;

import java.util.concurrent.TimeUnit;

/** Registro de agendamento periódico para coleta histórica. */
public final class RafaeliaBatchScheduler {

    private RafaeliaBatchScheduler() {}

    public static final String UNIQUE_NAME = "rafaelia-periodic-batch";

    public static void schedulePeriodic(Context context, String payload, int iterations, long intervalMinutes) {
        if (intervalMinutes < 15) intervalMinutes = 15; // restrição do WorkManager

        Data input = new Data.Builder()
            .putString(RafaeliaBatchWorker.KEY_PAYLOAD, payload)
            .putInt(RafaeliaBatchWorker.KEY_ITERATIONS, iterations)
            .build();

        Constraints constraints = new Constraints.Builder()
            .setRequiredNetworkType(NetworkType.NOT_REQUIRED)
            .build();

        PeriodicWorkRequest req = new PeriodicWorkRequest.Builder(
            RafaeliaBatchWorker.class,
            intervalMinutes,
            TimeUnit.MINUTES)
            .setInputData(input)
            .setConstraints(constraints)
            .build();

        WorkManager.getInstance(context)
            .enqueueUniquePeriodicWork(UNIQUE_NAME, ExistingPeriodicWorkPolicy.UPDATE, req);
    }
}
