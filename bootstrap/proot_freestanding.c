#include "proot_syscall_bridge.h"

/*
 * RAFCODEPHI freestanding runtime gate.
 *
 * This is deliberately smaller than PRoot/Ninja/pkg themselves. It is the
 * dependency-free control boundary that probes and execs those package
 * payloads without libc, malloc, fork, threads, stdio, JNI or shell-command
 * concatenation.
 *
 * Evidence rule:
 *   found/executable -> OBSERVED
 *   absent           -> TOKEN_VAZIO
 * No probe result is promoted to RUNTIME_PROVEN here.
 */

#define RAF_FD_STDOUT 1
#define RAF_FD_STDERR 2
#define RAF_MAX_PATH  384
#define RAF_MAX_ARGV   64

static const char RAF_FALLBACK_PREFIX[] =
    "/data/data/com.termux.rafacodephi/files/usr";

typedef struct {
    const char *name;
    int required;
} raf_tool_t;

static const raf_tool_t RAF_TOOLS[] = {
    {"pkg", 1},
    {"proot", 1},
    {"proot-distro", 1},
    {"ninja", 1},
    {"clang", 1},
    {"cmake", 1},
    {"qemu-system-x86_64", 0}
};

static raf_word_t raf_strlen(const char *s) {
    raf_word_t n = 0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}

static int raf_streq(const char *a, const char *b) {
    raf_word_t i = 0;
    if (!a || !b) return 0;
    while (a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == 0 && b[i] == 0;
}

static int raf_starts(const char *s, const char *prefix) {
    raf_word_t i = 0;
    if (!s || !prefix) return 0;
    while (prefix[i]) {
        if (s[i] != prefix[i]) return 0;
        ++i;
    }
    return 1;
}

static void raf_write_text(int fd, const char *s) {
    raf_word_t n = raf_strlen(s);
    while (n) {
        raf_sysret_t r = proot_sys_write(fd, s, n);
        if (r <= 0) return;
        s += (raf_word_t)r;
        n -= (raf_word_t)r;
    }
}

static void raf_line3(const char *a, const char *b, const char *c) {
    raf_write_text(RAF_FD_STDOUT, a);
    raf_write_text(RAF_FD_STDOUT, b);
    raf_write_text(RAF_FD_STDOUT, c);
    raf_write_text(RAF_FD_STDOUT, "\n");
}

static const char *raf_env_prefix(char *const envp[]) {
    raf_word_t i = 0;
    if (!envp) return RAF_FALLBACK_PREFIX;
    while (envp[i]) {
        if (raf_starts(envp[i], "PREFIX=") && envp[i][7]) return envp[i] + 7;
        ++i;
    }
    return RAF_FALLBACK_PREFIX;
}

static int raf_tool_path(char *out, raf_word_t cap,
                         const char *prefix, const char *tool) {
    raf_word_t i = 0, j = 0;
    const char suffix[] = "/bin/";
    if (!out || cap < 2 || !prefix || !tool) return -1;

    while (prefix[i]) {
        if (j + 1 >= cap) return -1;
        out[j++] = prefix[i++];
    }
    i = 0;
    while (suffix[i]) {
        if (j + 1 >= cap) return -1;
        out[j++] = suffix[i++];
    }
    i = 0;
    while (tool[i]) {
        if (j + 1 >= cap) return -1;
        out[j++] = tool[i++];
    }
    out[j] = 0;
    return 0;
}

static int raf_probe_tool(const char *prefix, const char *tool) {
    char path[RAF_MAX_PATH];
    if (raf_tool_path(path, sizeof(path), prefix, tool) != 0) return 0;
    return proot_sys_access_exec(path) == 0;
}

static int raf_probe_all(char *const envp[]) {
    raf_word_t i;
    int missing_required = 0;
    const char *prefix = raf_env_prefix(envp);

    raf_line3("RAFCODEPHI prefix=", prefix, "");
    for (i = 0; i < (sizeof(RAF_TOOLS) / sizeof(RAF_TOOLS[0])); ++i) {
        int ok = raf_probe_tool(prefix, RAF_TOOLS[i].name);
        if (ok) {
            raf_line3("OBSERVED executable: ", RAF_TOOLS[i].name, "");
        } else {
            raf_line3("TOKEN_VAZIO executable: ", RAF_TOOLS[i].name,
                      RAF_TOOLS[i].required ? " [required]" : " [optional]");
            if (RAF_TOOLS[i].required) ++missing_required;
        }
    }
    return missing_required ? 20 + missing_required : 0;
}

static int raf_exec_named(const char *name, int tailc, char *const *tailv,
                          char *const envp[]) {
    char path[RAF_MAX_PATH];
    char *execv[RAF_MAX_ARGV];
    int i;
    const char *prefix = raf_env_prefix(envp);

    if (!name || tailc < 0 || tailc + 2 > RAF_MAX_ARGV) return 64;
    if (raf_tool_path(path, sizeof(path), prefix, name) != 0) return 65;
    if (proot_sys_access_exec(path) != 0) {
        raf_line3("TOKEN_VAZIO executable: ", name, "");
        return 126;
    }

    execv[0] = (char *)name;
    for (i = 0; i < tailc; ++i) execv[i + 1] = tailv[i];
    execv[tailc + 1] = (char *)0;

    raf_line3("EXEC boundary: ", name, " (claim remains runtime-pending)");
    (void)proot_sys_execve(path, execv, (char *const *)envp);
    raf_line3("EXEC_FAILED: ", name, "");
    return 127;
}

static int raf_pkg_bootstrap(char *const envp[]) {
    static char *const args[] = {
        "install", "-y",
        "x11-repo",
        "proot", "proot-distro",
        "ninja", "clang", "lld", "cmake", "make", "binutils",
        "file", "patchelf",
        (char *)0
    };
    return raf_exec_named("pkg", 13, args, envp);
}

static int raf_pkg_vectras(char *const envp[]) {
    static char *const args[] = {
        "install", "-y",
        "qemu-common", "qemu-system-x86-64-headless", "qemu-utils",
        (char *)0
    };
    return raf_exec_named("pkg", 5, args, envp);
}

static int raf_main(int argc, char **argv, char *const envp[]) {
    if (argc <= 1 || raf_streq(argv[1], "--probe")) {
        return raf_probe_all(envp);
    }

    if (raf_streq(argv[1], "--pkg-bootstrap")) {
        return raf_pkg_bootstrap(envp);
    }

    if (raf_streq(argv[1], "--pkg-vectras")) {
        return raf_pkg_vectras(envp);
    }

    if (raf_streq(argv[1], "--run")) {
        if (argc < 3) {
            raf_write_text(RAF_FD_STDERR, "usage: rafproot-fs --run TOOL [args...]\n");
            return 64;
        }
        return raf_exec_named(argv[2], argc - 3, argv + 3, envp);
    }

    if (raf_streq(argv[1], "--help")) {
        raf_write_text(RAF_FD_STDOUT,
            "rafproot-fs --probe | --pkg-bootstrap | --pkg-vectras | --run TOOL [args...]\n");
        return 0;
    }

    raf_write_text(RAF_FD_STDERR, "unknown mode; use --help\n");
    return 64;
}

__attribute__((noreturn, used, noinline))
void raf_entry(raf_word_t *initial_sp) {
    int argc = (int)initial_sp[0];
    char **argv = (char **)(initial_sp + 1);
    char **envp = argv + argc + 1;
    int rc = raf_main(argc, argv, envp);
    proot_sys_exit(rc);
}

#if defined(__aarch64__)
__attribute__((naked, noreturn, used))
void _start(void) {
    __asm__ volatile(
        "mov x0, sp\n"
        "b raf_entry\n");
}
#elif defined(__arm__)
__attribute__((naked, noreturn, used))
void _start(void) {
    __asm__ volatile(
        "mov r0, sp\n"
        "b raf_entry\n");
}
#endif
