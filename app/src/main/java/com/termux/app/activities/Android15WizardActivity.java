package com.termux.app.activities;

/**
 * Compatibility entry point retained for manifest/settings callers.
 * The implementation now lives in BetaBootstrapWizardActivity so Wizard and
 * beta orchestration consume the same bootstrap-readiness invariant.
 */
public final class Android15WizardActivity extends BetaBootstrapWizardActivity {
}
