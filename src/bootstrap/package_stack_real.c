/*
 * RAFCODEPHI real package-stack controller.
 * Freestanding controller; apt/pkg/proot remain platform payloads.
 */

#include "freestanding.h"
#include "syscall_arm.h"

#ifndef RAFCODEPHI_PREFIX
#define RAFCODEPHI_PREFIX "/data/data/com.termux.rafacodephi/files/usr"
#endif

#define APT_GET RAFCODEPHI_PREFIX "/bin/apt-get"
#define PKG_BIN RAFCODEPHI_PREFIX "/bin/pkg"
#define LOCAL_LIST RAFCODEPHI_PREFIX "/etc/apt/rafcodephi-local.list"
#define LOCAL_REPO RAFCODEPHI_PREFIX "/var/lib/rafcodephi/repo/dists/stable/Release"

static int run_probe(const char *binary, const char *arg) {
    char *argv[] = {(char *)binary, (char *)arg, (char *)0};
    char *envp[] = {
        (char *)"PREFIX=" RAFCODEPHI_PREFIX,
        (char *)"PATH=" RAFCODEPHI_PREFIX "/bin:/system/bin",
        (char *)"TMPDIR=" RAFCODEPHI_PREFIX "/tmp",
        (char *)"HOME=/data/data/com.termux.rafacodephi/files/home",
        (char *)"LANG=C.UTF-8",
        (char *)0,
    };
    int64_t pid = syscall_fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        (void)syscall_execve(binary, argv, envp);
        syscall_exit(127);
    }
    int status = 0;
    int64_t waited = syscall_wait4((int)pid, &status, 0, (void *)0);
    if (waited != pid) return -2;
    return status == 0 ? 0 : -3;
}

int package_stack_local_repo_present(void) {
    return syscall_access(LOCAL_LIST, 0) == 0 && syscall_access(LOCAL_REPO, 0) == 0;
}

int package_stack_probe_real(void) {
    if (syscall_access(APT_GET, 1) != 0) return -1;
    if (syscall_access(PKG_BIN, 1) != 0) return -2;
    if (run_probe(APT_GET, "--version") != 0) return -3;
    if (run_probe(PKG_BIN, "help") != 0) return -4;
    return 0;
}
