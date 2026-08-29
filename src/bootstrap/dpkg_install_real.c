/* P0.3.2: dpkg installation — real implementation */

#include "freestanding.h"
#include "syscall_arm64.h"

#define DPKG_BINARY_PATH "/data/data/com.termux.rafacodephi/bin/dpkg"
#define DPKG_LIB_DIR "/data/data/com.termux.rafacodephi/lib"
#define DPKG_STATUS_FILE "/data/data/com.termux.rafacodephi/var/lib/dpkg/status"

/* Check if dpkg binary is accessible */
static int check_dpkg_binary(void) {
    int64_t ret = syscall_open(DPKG_BINARY_PATH, 0, 0);  /* O_RDONLY */
    if (SYSCALL_ERR(ret)) {
        return -1;  /* dpkg not found */
    }
    syscall_close((int)ret);
    return 0;  /* dpkg found */
}

/* Check if binary is statically linked (no dynamic section) */
static int verify_static_binary(const char *binary_path) {
    uint8_t elf_header[64];

    int64_t fd = syscall_open(binary_path, 0, 0);
    if (SYSCALL_ERR(fd)) {
        return -1;
    }

    int64_t nread = syscall_read((int)fd, elf_header, sizeof(elf_header));
    syscall_close((int)fd);

    if (nread < 64) {
        return -2;  /* Too short */
    }

    /* ELF header: 7f 45 4c 46 */
    if (elf_header[0] != 0x7f ||
        elf_header[1] != 'E' ||
        elf_header[2] != 'L' ||
        elf_header[3] != 'F') {
        return -3;  /* Not ELF */
    }

    /* TODO: Parse ELF to verify no dynamic section
       For now, trust that pre-built dpkg is static */

    return 0;
}

/* Initialize dpkg status database */
static int init_dpkg_status_db(void) {
    char status_header[] = "Package: dpkg\n"
                           "Version: 1.22.6\n"
                           "Status: install ok installed\n"
                           "\n";

    /* Create status file */
    int64_t fd = syscall_open(DPKG_STATUS_FILE, 0x0241, 0644);  /* O_CREAT | O_WRONLY | O_TRUNC */
    if (SYSCALL_ERR(fd)) {
        return -1;
    }

    int64_t written = syscall_write((int)fd, status_header, 65);
    syscall_close((int)fd);

    if (written != 65) {
        return -2;
    }

    return 0;
}

/* Install dpkg: verify binary, initialize database */
int dpkg_install_real(void) {
    /* Step 1: Verify dpkg binary exists */
    if (check_dpkg_binary() != 0) {
        return -1;  /* dpkg binary not found */
    }

    /* Step 2: Verify static linking */
    if (verify_static_binary(DPKG_BINARY_PATH) != 0) {
        return -2;  /* Binary verification failed */
    }

    /* Step 3: Initialize dpkg status database */
    if (init_dpkg_status_db() != 0) {
        return -3;  /* Status database init failed */
    }

    return 0;  /* Success */
}

/* Verify dpkg is correctly installed */
int dpkg_verify_installation(void) {
    int ret = check_dpkg_binary();
    if (ret != 0) {
        return -1;
    }

    /* Verify status file exists */
    int64_t fd = syscall_open(DPKG_STATUS_FILE, 0, 0);
    if (SYSCALL_ERR(fd)) {
        return -2;
    }
    syscall_close((int)fd);

    return 0;
}

/* Run dpkg command (via execve) */
int dpkg_run_command(const char *cmd_arg) {
    char *argv[] = {
        (char *)DPKG_BINARY_PATH,
        (char *)cmd_arg,
        NULL
    };

    char *env[] = {
        (char *)"TERMUX_PREFIX=/data/data/com.termux.rafacodephi",
        (char *)"PATH=/bin:/usr/bin",
        NULL
    };

    /* Fork and execve */
    int64_t pid = syscall_fork();

    if (SYSCALL_ERR(pid)) {
        return -1;  /* Fork failed */
    }

    if (pid == 0) {
        /* Child: exec dpkg */
        int64_t ret = syscall_execve(DPKG_BINARY_PATH, argv, env);
        if (SYSCALL_ERR(ret)) {
            syscall_exit(127);  /* execve failed */
        }
    } else {
        /* Parent: wait for child */
        int wstatus = 0;
        int64_t ret = syscall_wait4((int)pid, &wstatus, 0, NULL);

        if (SYSCALL_ERR(ret)) {
            return -2;  /* wait4 failed */
        }

        /* Check exit status */
        if (wstatus != 0) {
            return -3;  /* dpkg exited with error */
        }
    }

    return 0;
}

/* Get dpkg status (installed, version, etc.) */
typedef struct {
    int installed;
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
} DpkgStatus;

int dpkg_get_status(DpkgStatus *status) {
    status->installed = 0;
    status->version_major = 0;
    status->version_minor = 0;
    status->version_patch = 0;

    /* Check if dpkg binary exists */
    if (check_dpkg_binary() != 0) {
        return -1;
    }

    status->installed = 1;

    /* Version: 1.22.6 (hard-coded for now) */
    status->version_major = 1;
    status->version_minor = 22;
    status->version_patch = 6;

    return 0;
}
