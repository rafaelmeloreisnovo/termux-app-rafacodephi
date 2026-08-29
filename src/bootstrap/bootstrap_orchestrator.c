/* Bootstrap orchestrator — P0.1-P0.4 integration */

#include "freestanding.h"
#include "syscall_arm64.h"

/* External functions from modules */
extern int bootstrap_main(void);
extern int extract_bootstrap_payload(void);
extern int dpkg_install_real(void);
extern int seal_receipt_complete(struct Receipt *receipt, const char *json_data, uint32_t json_len);
extern int verify_receipt_sha256(struct Receipt *receipt, const char *json_data, uint32_t json_len);

#define LOG_TAG "BOOTSTRAP-ORCH"

static int64_t log_message(const char *msg, uint32_t len) {
    return syscall_write(2, msg, len);
}

/* P0.4: Validate receipt — block fake "success" on skip paths */
typedef struct {
    int extracted;
    int dpkg_installed;
    int apt_configured;
    int restart_count;
    int skip_count;
} BootstrapProgress;

static int validate_receipt_state(struct Receipt *receipt, BootstrapProgress *progress) {
    /* If restart_count > 2, receipt is invalid */
    if (progress->restart_count > 2) {
        log_message("ERROR: restart count exceeded\n", 31);
        return -1;
    }

    /* If skip_count > 0 but marked as "success", reject */
    if (progress->skip_count > 0 && receipt->exit_code == 0) {
        log_message("ERROR: receipt marked success but stages skipped\n", 51);
        return -2;
    }

    /* Validate phi_fst coherence range [0, 1] in Q16 */
    if (receipt->phi_fst > 0x10000) {
        log_message("ERROR: phi_fst out of range\n", 30);
        return -3;
    }

    /* Validate attractor range [0, 40] — 41-state toroid (BUG-02 resolved) */
    if (receipt->attractor > 40) {
        log_message("ERROR: attractor out of range [0..40]\n", 39);
        return -4;
    }

    return 0;
}

/* Main bootstrap orchestration */
int bootstrap_orchestrator_main(void) {
    struct Receipt receipt;
    struct BootstrapProgress progress;

    /* Initialize */
    receipt.magic = 0;
    receipt.stage = 0;
    receipt.exit_code = 1;  /* Assume failure until proven otherwise */
    progress.extracted = 0;
    progress.dpkg_installed = 0;
    progress.apt_configured = 0;
    progress.restart_count = 0;
    progress.skip_count = 0;

    log_message("[ORCH] Starting bootstrap orchestration\n", 40);

    /* P0.1: Initialize proot */
    log_message("[ORCH] P0.1: Bootstrap main\n", 29);
    int ret = bootstrap_main();
    if (ret != 0) {
        log_message("[ORCH] P0.1 failed\n", 19);
        progress.skip_count++;
    }

    /* P0.3.1: Extract payload */
    log_message("[ORCH] P0.3.1: Extract payload\n", 32);
    ret = extract_bootstrap_payload();
    if (ret == 0) {
        progress.extracted = 1;
        log_message("[ORCH] Payload extracted\n", 26);
    } else if (ret == -3) {
        log_message("[ORCH] Gzip decompression not implemented, skipping\n", 52);
        progress.skip_count++;
    } else {
        log_message("[ORCH] Extract failed\n", 22);
        progress.skip_count++;
    }

    /* P0.3.2: Install dpkg */
    log_message("[ORCH] P0.3.2: Install dpkg\n", 29);
    ret = dpkg_install_real();
    if (ret == 0) {
        progress.dpkg_installed = 1;
        log_message("[ORCH] dpkg installed\n", 22);
    } else {
        log_message("[ORCH] dpkg install failed\n", 28);
        progress.skip_count++;
    }

    /* P0.4: Validate receipt state */
    log_message("[ORCH] P0.4: Validate receipt state\n", 36);
    ret = validate_receipt_state(&receipt, &progress);
    if (ret != 0) {
        log_message("[ORCH] Receipt validation failed\n", 33);
        receipt.exit_code = 1;
        return ret;
    }

    /* Mark success if critical paths completed */
    if (progress.dpkg_installed && progress.skip_count == 0) {
        receipt.exit_code = 0;
        log_message("[ORCH] Bootstrap successful\n", 28);
    } else {
        log_message("[ORCH] Bootstrap incomplete (skip_count > 0)\n", 45);
    }

    return receipt.exit_code;
}

/* Entry point */
int main(void) {
    int ret = bootstrap_orchestrator_main();
    syscall_exit(ret);
    return ret;  /* unreachable */
}
