/*
 * RAFCODEPHI dpkg adapter for the freestanding bootstrap controller.
 *
 * dpkg is a platform payload, not a freestanding artifact.  This unit proves
 * the expected ELF ABI, creates only the minimum database skeleton when absent,
 * and executes a real --version probe without fabricating package state.
 */

#include "freestanding.h"
#include "syscall_arm.h"

#ifndef RAFCODEPHI_PREFIX
#define RAFCODEPHI_PREFIX "/data/data/com.termux.rafacodephi/files/usr"
#endif

#define DPKG_BINARY_PATH RAFCODEPHI_PREFIX "/bin/dpkg"
#define DPKG_VAR_DIR     RAFCODEPHI_PREFIX "/var"
#define DPKG_LIB_VAR_DIR RAFCODEPHI_PREFIX "/var/lib"
#define DPKG_DB_DIR      RAFCODEPHI_PREFIX "/var/lib/dpkg"
#define DPKG_STATUS_FILE RAFCODEPHI_PREFIX "/var/lib/dpkg/status"
#define RAF_WNOHANG 1

static int mkdir_existing_ok(const char *path, int mode) {
    int64_t rc = syscall_mkdir(path, mode);
    if (rc == 0) return 0;
    return SYSCALL_ERR_VAL(rc) == 17 ? 0 : -1;
}

static int check_dpkg_binary(void) {
    return syscall_access(DPKG_BINARY_PATH, 1) == 0 ? 0 : -1;
}

static int verify_dpkg_elf_abi(void) {
    uint8_t h[64];
    int64_t fd = syscall_open(DPKG_BINARY_PATH, RAF_O_RDONLY | RAF_O_CLOEXEC, 0);
    if (fd < 0) return -1;
    int64_t n = syscall_read((int)fd, h, sizeof(h));
    (void)syscall_close((int)fd);
    if (n < 20) return -2;
    if (h[0] != 0x7f || h[1] != 'E' || h[2] != 'L' || h[3] != 'F') return -3;
    if (h[5] != 1) return -4; /* little endian */
    uint32_t machine = (uint32_t)h[18] | ((uint32_t)h[19] << 8);
#if defined(__aarch64__)
    if (h[4] != 2 || machine != 183u) return -5; /* ELF64 / EM_AARCH64 */
#elif defined(__arm__)
    if (h[4] != 1 || machine != 40u) return -5;  /* ELF32 / EM_ARM */
#endif
    return 0;
}

static int init_dpkg_status_db(void) {
    if (mkdir_existing_ok(DPKG_VAR_DIR, 0700) != 0) return -1;
    if (mkdir_existing_ok(DPKG_LIB_VAR_DIR, 0700) != 0) return -2;
    if (mkdir_existing_ok(DPKG_DB_DIR, 0700) != 0) return -3;

    if (syscall_access(DPKG_STATUS_FILE, 0) == 0) return 0;

    int64_t fd = syscall_open(DPKG_STATUS_FILE,
                              RAF_O_WRONLY | RAF_O_CREAT | RAF_O_EXCL | RAF_O_CLOEXEC,
                              0600);
    if (fd < 0) {
        /* A concurrent/previous initializer may have created it. */
        return syscall_access(DPKG_STATUS_FILE, 0) == 0 ? 0 : -4;
    }
    if (syscall_fsync((int)fd) < 0) {
        (void)syscall_close((int)fd);
        return -5;
    }
    return syscall_close((int)fd) == 0 ? 0 : -6;
}

static int run_dpkg_version(void) {
    char *argv[] = {
        (char *)DPKG_BINARY_PATH,
        (char *)"--version",
        (char *)0
    };
    char *envp[] = {
        (char *)"PREFIX=" RAFCODEPHI_PREFIX,
        (char *)"PATH=" RAFCODEPHI_PREFIX "/bin:/system/bin",
        (char *)"TMPDIR=" RAFCODEPHI_PREFIX "/tmp",
        (char *)"HOME=/data/data/com.termux.rafacodephi/files/home",
        (char *)"LANG=C.UTF-8",
        (char *)0
    };

    int64_t pid = syscall_fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        (void)syscall_execve(DPKG_BINARY_PATH, argv, envp);
        syscall_exit(127);
    }

    int status = 0;
    int64_t waited = syscall_wait4((int)pid, &status, 0, (void *)0);
    if (waited != pid) return -2;
    return status == 0 ? 0 : -3;
}

int dpkg_install_real(void) {
    if (check_dpkg_binary() != 0) return -1;
    if (verify_dpkg_elf_abi() != 0) return -2;
    if (init_dpkg_status_db() != 0) return -3;
    if (run_dpkg_version() != 0) return -4;
    return 0;
}

int dpkg_verify_installation(void) {
    if (check_dpkg_binary() != 0) return -1;
    if (verify_dpkg_elf_abi() != 0) return -2;
    if (syscall_access(DPKG_STATUS_FILE, 0) != 0) return -3;
    return 0;
}

int dpkg_run_command(const char *cmd_arg) {
    if (!cmd_arg) return -1;
    char *argv[] = {
        (char *)DPKG_BINARY_PATH,
        (char *)cmd_arg,
        (char *)0
    };
    char *envp[] = {
        (char *)"PREFIX=" RAFCODEPHI_PREFIX,
        (char *)"PATH=" RAFCODEPHI_PREFIX "/bin:/system/bin",
        (char *)"TMPDIR=" RAFCODEPHI_PREFIX "/tmp",
        (char *)"HOME=/data/data/com.termux.rafacodephi/files/home",
        (char *)"LANG=C.UTF-8",
        (char *)0
    };
    int64_t pid = syscall_fork();
    if (pid < 0) return -2;
    if (pid == 0) {
        (void)syscall_execve(DPKG_BINARY_PATH, argv, envp);
        syscall_exit(127);
    }
    int status = 0;
    int64_t waited = syscall_wait4((int)pid, &status, 0, (void *)0);
    if (waited != pid) return -3;
    return status == 0 ? 0 : -4;
}

typedef struct {
    int installed;
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
} DpkgStatus;

int dpkg_get_status(DpkgStatus *status) {
    if (!status) return -1;
    status->installed = 0;
    status->version_major = 0;
    status->version_minor = 0;
    status->version_patch = 0;
    if (dpkg_verify_installation() != 0) return -2;
    status->installed = 1;
    /* Version numbers are intentionally not invented.  The executable probe is
     * the authority until a bounded parser is added. */
    return 0;
}
