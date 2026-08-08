package com.termux.app.benchmark;

import com.termux.app.activities.BetaOrchestratorActivity;

/**
 * Compatibility entry point for existing Settings/Vectra links.
 *
 * Historical callers still target BenchmarkMenuActivity, but the user-facing
 * operation is now the unified bootstrap + evidence orchestrator. Low-level PA
 * execution remains implemented by PaBenchmarkRunner and the governed analyzer;
 * no benchmark contract is weakened by this routing layer.
 */
public final class BenchmarkMenuActivity extends BetaOrchestratorActivity {
}
