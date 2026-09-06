/*
 * RAFCODEPHI freestanding bootstrap orchestrator v2.
 *
 * Order is intentionally causal:
 *   materialize payload -> validate dpkg -> probe apt/pkg -> probe proot.
 * A missing/skipped stage can never be promoted to success.
 */

#include "freestanding.h"
#include "syscall_arm.h"

extern int extract_bootstrap_payload(void);
extern int dpkg_install_real(void);
extern int package_stack_probe_real(void);
extern int package_stack_local_repo_present(void);
extern int bootstrap_main(void); /* RAF-NINJA proot probe adapter */

static int64_t log_message(const char *msg, uint32_t len) {
    return syscall_write(2, msg, len);
}

enum RafBootstrapStage {
    RAF_STAGE_BEGIN = 0,
    RAF_STAGE_PAYLOAD = 1,
    RAF_STAGE_DPKG = 2,
    RAF_STAGE_PKG = 3,
    RAF_STAGE_PROOT = 4,
    RAF_STAGE_READY_FOR_DEVICE_SMOKE = 5,
};

struct RafBootstrapReceipt {
    uint32_t stage;
    int32_t payload_preexisting;
    int32_t local_repo_present;
    int32_t failure_stage;
    int32_t failure_code;
    int32_t claim_allowed_pkg_runtime;
    int32_t claim_allowed_device_runtime;
};

static void receipt_init(struct RafBootstrapReceipt *r) {
    r->stage = RAF_STAGE_BEGIN;
    r->payload_preexisting = 0;
    r->local_repo_present = 0;
    r->failure_stage = 0;
    r->failure_code = 0;
    r->claim_allowed_pkg_runtime = 0;
    r->claim_allowed_device_runtime = 0;
}

static int fail(struct RafBootstrapReceipt *r, int stage, int code, const char *msg, uint32_t len) {
    r->failure_stage = stage;
    r->failure_code = code;
    (void)log_message(msg, len);
    return code ? code : -1;
}

int bootstrap_orchestrator_run(struct RafBootstrapReceipt *receipt) {
    if (!receipt) return -100;
    receipt_init(receipt);

    (void)log_message("RAF-BOOTSTRAP: materialize payload\n", 35);
    int rc = extract_bootstrap_payload();
    if (rc != 0 && rc != 1) {
        return fail(receipt, RAF_STAGE_PAYLOAD, rc,
                    "RAF-BOOTSTRAP: payload FAIL\n", 28);
    }
    receipt->payload_preexisting = (rc == 1);
    receipt->stage = RAF_STAGE_PAYLOAD;

    (void)log_message("RAF-BOOTSTRAP: validate dpkg\n", 29);
    rc = dpkg_install_real();
    if (rc != 0) {
        return fail(receipt, RAF_STAGE_DPKG, rc,
                    "RAF-BOOTSTRAP: dpkg FAIL\n", 25);
    }
    receipt->stage = RAF_STAGE_DPKG;

    (void)log_message("RAF-BOOTSTRAP: probe apt/pkg\n", 29);
    rc = package_stack_probe_real();
    if (rc != 0) {
        return fail(receipt, RAF_STAGE_PKG, rc,
                    "RAF-BOOTSTRAP: pkg stack FAIL\n", 30);
    }
    receipt->local_repo_present = package_stack_local_repo_present();
    receipt->stage = RAF_STAGE_PKG;

    (void)log_message("RAF-BOOTSTRAP: RAF-NINJA proot\n", 32);
    rc = bootstrap_main();
    if (rc != 0) {
        return fail(receipt, RAF_STAGE_PROOT, rc,
                    "RAF-BOOTSTRAP: proot FAIL\n", 26);
    }
    receipt->stage = RAF_STAGE_PROOT;

    /* Structural readiness is not pkg runtime proof.  The device smoke owns
     * pkg update/install promotion. */
    receipt->stage = RAF_STAGE_READY_FOR_DEVICE_SMOKE;
    receipt->claim_allowed_pkg_runtime = 0;
    receipt->claim_allowed_device_runtime = 0;
    (void)log_message("RAF-BOOTSTRAP: READY_FOR_DEVICE_SMOKE\n", 38);
    return 0;
}

int bootstrap_orchestrator_main(void) {
    struct RafBootstrapReceipt receipt;
    return bootstrap_orchestrator_run(&receipt);
}

int main(void) {
    int rc = bootstrap_orchestrator_main();
    syscall_exit(rc == 0 ? 0 : 1);
}
