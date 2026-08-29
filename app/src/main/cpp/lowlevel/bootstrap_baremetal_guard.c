#include "bootstrap_baremetal_guard.h"

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#include "freestanding_syscalls.h"
#include "freestanding_string.h"
#include "freestanding_log.h"

#define RAF_OK 0
#define RAF_ERR_PARAM -1
#define RAF_ERR_PREFIX -2
#define RAF_ERR_PAGESZ -3
#define RAF_ERR_DIRS -4
#define RAF_ERR_BINS -5
#define RAF_ERR_JSON -6

/* File mode constants (from sys/stat.h) */
#define S_IFMT      0170000
#define S_IFREG     0100000
#define S_IFDIR     0040000
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_IXUSR     0100

static int has_usr_suffix(const char* prefix) {
    if (!prefix) return 0;
    /* Check if prefix contains "/files/usr" */
    const char *p = prefix;
    while (*p) {
        if (p[0] == '/' && p[1] == 'f' && p[2] == 'i' && p[3] == 'l' &&
            p[4] == 'e' && p[5] == 's' && p[6] == '/' && p[7] == 'u' &&
            p[8] == 's' && p[9] == 'r' &&
            (p[10] == '\0' || p[10] == '/')) {
            return 1;
        }
        p++;
    }
    return 0;
}

static int join_path(char* out, size_t out_sz, const char* prefix, const char* suffix) {
    if (!out || !prefix || !suffix || out_sz == 0) return RAF_ERR_PARAM;

    uint32_t plen = (uint32_t)freestanding_strlen(prefix);
    uint32_t slen = (uint32_t)freestanding_strlen(suffix);

    if ((uint32_t)(plen + slen + 1) > out_sz) return RAF_ERR_PARAM;

    freestanding_memcpy(out, prefix, plen);
    freestanding_memcpy(out + plen, suffix, slen);
    out[plen + slen] = '\0';

    return RAF_OK;
}

int raf_bootstrap_guard_check_page_size(void) {
    /* Android default: 4096 bytes. Android 15+ uses 16384 bytes.
     * Hardcoded since sysconf is from libc */
#ifdef __ANDROID__
    return 16384;  /* Android 15+ page size */
#else
    return 4096;   /* Legacy page size */
#endif
}

int raf_bootstrap_guard_check_exec(const char* path) {
    struct freestanding_stat st;
    if (!path || !*path) return RAF_ERR_PARAM;
    if (freestanding_stat(path, &st) != 0) return RAF_ERR_BINS;
    if (!S_ISREG((uint32_t)st.st_mode)) return RAF_ERR_BINS;
    if ((st.st_mode & S_IXUSR) == 0) return RAF_ERR_BINS;
    return RAF_OK;
}

int raf_bootstrap_guard_check_basic_dirs(const char* prefix) {
    static const char* dirs[] = {"/bin", "/etc", "/lib", "/tmp", "/var"};
    char path[PATH_MAX];
    struct freestanding_stat st;
    size_t i;

    if (!prefix || !*prefix) return RAF_ERR_PARAM;

    for (i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
        if (join_path(path, sizeof(path), prefix, dirs[i]) < 0) return RAF_ERR_DIRS;
        if (freestanding_stat(path, &st) != 0 || !S_ISDIR((uint32_t)st.st_mode)) return RAF_ERR_DIRS;
    }

    return RAF_OK;
}

int raf_bootstrap_guard_check_required_bins(const char* prefix) {
    static const char* bins[] = {"/bin/sh", "/bin/pkg"};
    char path[PATH_MAX];
    size_t i;

    if (!prefix || !*prefix) return RAF_ERR_PARAM;

    for (i = 0; i < sizeof(bins) / sizeof(bins[0]); i++) {
        if (join_path(path, sizeof(path), prefix, bins[i]) < 0) return RAF_ERR_BINS;
        if (raf_bootstrap_guard_check_exec(path) < 0) return RAF_ERR_BINS;
    }

    return RAF_OK;
}

static void append_bool_json(char* out, size_t* off, size_t cap, int val) {
    const char *s = val ? "true" : "false";
    uint32_t len = (uint32_t)freestanding_strlen(s);
    if (*off + len < cap) {
        freestanding_memcpy(out + *off, s, len);
        *off += len;
    }
}

static void append_str_json(char* out, size_t* off, size_t cap, const char* s) {
    if (!s || !out) return;
    uint32_t len = (uint32_t)freestanding_strlen(s);
    if (*off + len < cap) {
        freestanding_memcpy(out + *off, s, len);
        *off += len;
    }
}

static void append_int_json(char* out, size_t* off, size_t cap, int val) {
    char buf[32];
    if (val < 0) {
        buf[0] = '-';
        val = -val;
    } else {
        buf[0] = '\0';
    }
    int has_neg = buf[0] == '-';
    int idx = has_neg ? 1 : 0;
    if (val == 0) {
        buf[idx++] = '0';
    } else {
        int digits[10], d = 0;
        int tmp = val;
        while (tmp > 0) { digits[d++] = tmp % 10; tmp /= 10; }
        for (int i = d - 1; i >= 0; i--) buf[idx++] = (char)('0' + digits[i]);
    }
    buf[idx] = '\0';
    append_str_json(out, off, cap, buf);
}

int raf_bootstrap_guard_validate_prefix(const char* prefix, char* out_json, int cap) {
    int page_size;
    int dirs_rc;
    int bins_rc;
    int sh_ok = 0;
    int pkg_ok = 0;
    int busybox_ok = 0;
    int proot_ok = 0;
    int exec_ok;
    char path[PATH_MAX];
    size_t off = 0;

    if (!out_json || cap <= 0) return RAF_ERR_PARAM;
    out_json[0] = '\0';

    if (!prefix) return RAF_ERR_PARAM;
    if (!prefix[0]) return RAF_ERR_PREFIX;
    if (!has_usr_suffix(prefix)) return RAF_ERR_PREFIX;

    page_size = raf_bootstrap_guard_check_page_size();
    if (page_size < 0) return page_size;

    if (join_path(path, sizeof(path), prefix, "/bin/sh") == RAF_OK) {
        sh_ok = raf_bootstrap_guard_check_exec(path) == RAF_OK;
    }
    if (join_path(path, sizeof(path), prefix, "/bin/pkg") == RAF_OK) {
        pkg_ok = raf_bootstrap_guard_check_exec(path) == RAF_OK;
    }
    if (join_path(path, sizeof(path), prefix, "/bin/busybox") == RAF_OK) {
        busybox_ok = raf_bootstrap_guard_check_exec(path) == RAF_OK;
    }
    if (join_path(path, sizeof(path), prefix, "/bin/proot") == RAF_OK) {
        proot_ok = raf_bootstrap_guard_check_exec(path) == RAF_OK;
    }

    exec_ok = sh_ok && pkg_ok;
    dirs_rc = raf_bootstrap_guard_check_basic_dirs(prefix);
    bins_rc = raf_bootstrap_guard_check_required_bins(prefix);

    /* Build JSON manually without snprintf */
    append_str_json(out_json, &off, (size_t)cap, "{\"guard\":\"bootstrap_baremetal\",\"prefix\":\"");
    append_str_json(out_json, &off, (size_t)cap, prefix);
    append_str_json(out_json, &off, (size_t)cap, "\",\"page_size\":");
    append_int_json(out_json, &off, (size_t)cap, page_size);
    append_str_json(out_json, &off, (size_t)cap, ",\"bin_sh\":");
    append_bool_json(out_json, &off, (size_t)cap, sh_ok);
    append_str_json(out_json, &off, (size_t)cap, ",\"bin_pkg\":");
    append_bool_json(out_json, &off, (size_t)cap, pkg_ok);
    append_str_json(out_json, &off, (size_t)cap, ",\"busybox\":");
    append_bool_json(out_json, &off, (size_t)cap, busybox_ok);
    append_str_json(out_json, &off, (size_t)cap, ",\"proot\":");
    append_bool_json(out_json, &off, (size_t)cap, proot_ok);
    append_str_json(out_json, &off, (size_t)cap, ",\"exec_ok\":");
    append_bool_json(out_json, &off, (size_t)cap, exec_ok);
    append_str_json(out_json, &off, (size_t)cap, ",\"status\":\"");
    append_str_json(out_json, &off, (size_t)cap, (dirs_rc == RAF_OK && bins_rc == RAF_OK) ? "OK" : "ERROR");
    append_str_json(out_json, &off, (size_t)cap, "\"}");

    if (off >= (size_t)cap) return RAF_ERR_JSON;
    out_json[off] = '\0';

    if (dirs_rc != RAF_OK) return RAF_ERR_DIRS;
    if (bins_rc != RAF_OK) return RAF_ERR_BINS;
    return RAF_OK;
}

int raf_bootstrap_guard_selftest(char* out_json, int cap) {
    int page_size;
    size_t off = 0;

    if (!out_json || cap <= 0) return RAF_ERR_PARAM;

    page_size = raf_bootstrap_guard_check_page_size();

    /* Build JSON manually without snprintf */
    append_str_json(out_json, &off, (size_t)cap,
        "{\"guard\":\"bootstrap_baremetal\",\"prefix\":\"selftest\",\"page_size\":");
    append_int_json(out_json, &off, (size_t)cap, page_size > 0 ? page_size : 0);
    append_str_json(out_json, &off, (size_t)cap,
        ",\"bin_sh\":false,\"bin_pkg\":false,\"busybox\":false,\"proot\":false,\"exec_ok\":false,\"status\":\"");
    append_str_json(out_json, &off, (size_t)cap, page_size > 0 ? "OK" : "ERROR");
    append_str_json(out_json, &off, (size_t)cap, "\"}");

    if (off >= (size_t)cap) return RAF_ERR_JSON;
    out_json[off] = '\0';

    if (page_size < 0) return page_size;
    return RAF_OK;
}
