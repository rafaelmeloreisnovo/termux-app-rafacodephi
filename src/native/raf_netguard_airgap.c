/*
 * RAFCODEPhi NetGuard airgap launcher.
 *
 * Defensive scope: isolates a command intentionally launched by this process.
 * No root, no remote-process injection, no packet capture, no bypass behavior.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#if defined(__aarch64__)
#define RAF_AUDIT_ARCH AUDIT_ARCH_AARCH64
#elif defined(__arm__)
#define RAF_AUDIT_ARCH AUDIT_ARCH_ARM
#elif defined(__x86_64__)
#define RAF_AUDIT_ARCH AUDIT_ARCH_X86_64
#elif defined(__i386__)
#define RAF_AUDIT_ARCH AUDIT_ARCH_I386
#elif defined(__riscv) && __riscv_xlen == 64
#define RAF_AUDIT_ARCH AUDIT_ARCH_RISCV64
#else
#error "Unsupported architecture for RAFCODEPhi NetGuard airgap"
#endif

#define RAF_DENY_SYSCALL(nr)     BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (uint32_t)(nr), 0, 1),     BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA))

static int raf_stdio_is_socket(void) {
    int fd;
    for (fd = 0; fd <= 2; ++fd) {
        struct stat st;
        if (fstat(fd, &st) == 0 && S_ISSOCK(st.st_mode)) return 1;
    }
    return 0;
}

static void raf_close_inherited_fds(void) {
    struct rlimit lim;
    rlim_t maxfd = 4096u;
    int fd;

    if (getrlimit(RLIMIT_NOFILE, &lim) == 0 && lim.rlim_cur != RLIM_INFINITY) {
        maxfd = lim.rlim_cur;
    }
    if (maxfd > 65536u) maxfd = 65536u;

    for (fd = 3; (rlim_t)fd < maxfd; ++fd) {
        (void)close(fd);
    }
}

static int raf_install_airgap(void) {
    struct sock_filter filter[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (uint32_t)offsetof(struct seccomp_data, arch)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, RAF_AUDIT_ARCH, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (uint32_t)offsetof(struct seccomp_data, nr)),

#ifdef __NR_socket
        RAF_DENY_SYSCALL(__NR_socket),
#endif
#ifdef __NR_socketpair
        RAF_DENY_SYSCALL(__NR_socketpair),
#endif
#ifdef __NR_connect
        RAF_DENY_SYSCALL(__NR_connect),
#endif
#ifdef __NR_bind
        RAF_DENY_SYSCALL(__NR_bind),
#endif
#ifdef __NR_listen
        RAF_DENY_SYSCALL(__NR_listen),
#endif
#ifdef __NR_accept
        RAF_DENY_SYSCALL(__NR_accept),
#endif
#ifdef __NR_accept4
        RAF_DENY_SYSCALL(__NR_accept4),
#endif
#ifdef __NR_sendto
        RAF_DENY_SYSCALL(__NR_sendto),
#endif
#ifdef __NR_recvfrom
        RAF_DENY_SYSCALL(__NR_recvfrom),
#endif
#ifdef __NR_sendmsg
        RAF_DENY_SYSCALL(__NR_sendmsg),
#endif
#ifdef __NR_recvmsg
        RAF_DENY_SYSCALL(__NR_recvmsg),
#endif
#ifdef __NR_sendmmsg
        RAF_DENY_SYSCALL(__NR_sendmmsg),
#endif
#ifdef __NR_recvmmsg
        RAF_DENY_SYSCALL(__NR_recvmmsg),
#endif
#ifdef __NR_shutdown
        RAF_DENY_SYSCALL(__NR_shutdown),
#endif
#ifdef __NR_io_uring_setup
        RAF_DENY_SYSCALL(__NR_io_uring_setup),
#endif
#ifdef __NR_io_uring_enter
        RAF_DENY_SYSCALL(__NR_io_uring_enter),
#endif
#ifdef __NR_io_uring_register
        RAF_DENY_SYSCALL(__NR_io_uring_register),
#endif

        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)
    };
    struct sock_fprog program;

    program.len = (unsigned short)(sizeof(filter) / sizeof(filter[0]));
    program.filter = filter;

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) return -1;
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) != 0) return -1;
    return 0;
}

static int raf_selftest(void) {
    int fd;

    if (raf_stdio_is_socket()) {
        fprintf(stderr, "RAF_NETGUARD_SELFTEST FAIL stdio_socket\n");
        return 2;
    }
    raf_close_inherited_fds();

    if (raf_install_airgap() != 0) {
        perror("RAF_NETGUARD_SELFTEST seccomp");
        return 3;
    }

    errno = 0;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1 && errno == EPERM) {
        puts("RAF_NETGUARD_AIRGAP_SELFTEST PASS");
        return 0;
    }
    if (fd >= 0) close(fd);
    fprintf(stderr, "RAF_NETGUARD_AIRGAP_SELFTEST FAIL errno=%d\n", errno);
    return 4;
}

int main(int argc, char **argv) {
    if (argc == 2 && argv[1][0] == '-' && argv[1][1] == '-' &&
        argv[1][2] == 's' && argv[1][3] == 'e' && argv[1][4] == 'l' &&
        argv[1][5] == 'f' && argv[1][6] == 't' && argv[1][7] == 'e' &&
        argv[1][8] == 's' && argv[1][9] == 't' && argv[1][10] == '\0') {
        return raf_selftest();
    }

    if (argc < 2) {
        fprintf(stderr, "usage: %s --selftest | command [args...]\n", argv[0]);
        return 64;
    }

    if (raf_stdio_is_socket()) {
        fprintf(stderr, "RAF_NETGUARD BLOCK: stdio is a socket; refusing ambiguous airgap\n");
        return 65;
    }

    raf_close_inherited_fds();

    if (raf_install_airgap() != 0) {
        perror("RAF_NETGUARD: cannot install seccomp airgap");
        return 66;
    }

    execvp(argv[1], &argv[1]);
    perror("RAF_NETGUARD: execvp");
    return 67;
}
