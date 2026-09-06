/*
 * RAFCODEPHI bootstrap payload extractor.
 *
 * Canonical freestanding transport is an uncompressed, hash-bound TAR stream.
 * Compression belongs to the build/transport adapter; the on-device controller
 * does not link zlib and does not shell out to gzip.
 */

#include "freestanding.h"
#include "syscall_arm.h"

#ifndef RAFCODEPHI_PREFIX
#define RAFCODEPHI_PREFIX "/data/data/com.termux.rafacodephi/files/usr"
#endif
#ifndef RAFCODEPHI_BOOTSTRAP_TAR
#define RAFCODEPHI_BOOTSTRAP_TAR "/data/data/com.termux.rafacodephi/files/rafcodephi-bootstrap.tar"
#endif

#define TAR_BLOCK 512u
#define IO_CHUNK  16384u
#define PATH_CAP  512u

struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag[1];
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
};

typedef char raf_tar_header_size_must_be_512[(sizeof(struct tar_header) == TAR_BLOCK) ? 1 : -1];

static uint32_t cstrlen_bounded(const char *s, uint32_t cap) {
    uint32_t n = 0;
    if (!s) return 0;
    while (n < cap && s[n]) ++n;
    return n;
}

static int mem_is_zero(const uint8_t *p, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n; ++i) if (p[i] != 0) return 0;
    return 1;
}

static uint64_t parse_octal64(const char *s, uint32_t n, int *ok) {
    uint64_t v = 0;
    uint32_t i = 0;
    *ok = 0;
    while (i < n && (s[i] == ' ' || s[i] == '\0')) ++i;
    for (; i < n; ++i) {
        char c = s[i];
        if (c == '\0' || c == ' ') break;
        if (c < '0' || c > '7') return 0;
        if (v > ((~(uint64_t)0) >> 3)) return 0;
        v = (v << 3) + (uint64_t)(c - '0');
    }
    *ok = 1;
    return v;
}

static int tar_checksum_ok(const uint8_t block[TAR_BLOCK]) {
    const struct tar_header *h = (const struct tar_header *)block;
    int ok = 0;
    uint64_t expected = parse_octal64(h->checksum, sizeof(h->checksum), &ok);
    if (!ok) return 0;
    uint64_t sum = 0;
    uint32_t i;
    for (i = 0; i < TAR_BLOCK; ++i) {
        if (i >= 148u && i < 156u) sum += (uint8_t)' ';
        else sum += block[i];
    }
    return sum == expected;
}

static int path_has_dotdot(const char *p) {
    uint32_t i = 0;
    while (p[i]) {
        while (p[i] == '/') ++i;
        uint32_t start = i;
        while (p[i] && p[i] != '/') ++i;
        uint32_t len = i - start;
        if (len == 2u && p[start] == '.' && p[start + 1u] == '.') return 1;
    }
    return 0;
}

static int relative_path_safe(const char *p) {
    if (!p || !p[0] || p[0] == '/') return 0;
    if (path_has_dotdot(p)) return 0;
    return cstrlen_bounded(p, PATH_CAP) < PATH_CAP;
}

static int copy_component(char *dst, uint32_t *pos, uint32_t cap, const char *src, uint32_t max) {
    uint32_t i = 0;
    while (i < max && src[i]) {
        if (*pos + 1u >= cap) return -1;
        dst[(*pos)++] = src[i++];
    }
    return 0;
}

static int tar_member_name(const struct tar_header *h, char out[PATH_CAP]) {
    uint32_t p = 0;
    if (h->prefix[0]) {
        if (copy_component(out, &p, PATH_CAP, h->prefix, sizeof(h->prefix)) != 0) return -1;
        if (p + 1u >= PATH_CAP) return -1;
        out[p++] = '/';
    }
    if (copy_component(out, &p, PATH_CAP, h->name, sizeof(h->name)) != 0) return -1;
    out[p] = 0;
    return relative_path_safe(out) ? 0 : -2;
}

static int full_path(const char *relative, char out[PATH_CAP]) {
    uint32_t p = 0;
    if (copy_component(out, &p, PATH_CAP, RAFCODEPHI_PREFIX, PATH_CAP) != 0) return -1;
    if (p + 1u >= PATH_CAP) return -1;
    out[p++] = '/';
    if (copy_component(out, &p, PATH_CAP, relative, PATH_CAP) != 0) return -1;
    out[p] = 0;
    return 0;
}

static int mkdir_if_needed(const char *path, int mode) {
    int64_t rc = syscall_mkdir(path, mode);
    if (rc == 0) return 0;
    /* EEXIST == 17.  We deliberately do not treat arbitrary mkdir errors as
     * success because that can hide permission/path failures. */
    return SYSCALL_ERR_VAL(rc) == 17 ? 0 : -1;
}

static int ensure_parent_dirs(const char *full) {
    char tmp[PATH_CAP];
    uint32_t n = cstrlen_bounded(full, PATH_CAP);
    if (n == 0 || n >= PATH_CAP) return -1;
    uint32_t i;
    for (i = 0; i <= n; ++i) tmp[i] = full[i];

    /* Never attempt to mkdir the leading /data, /data/data... components that
     * are outside app ownership. Begin only after the canonical prefix. */
    uint32_t base = cstrlen_bounded(RAFCODEPHI_PREFIX, PATH_CAP);
    if (base >= n) return 0;
    for (i = base + 1u; i < n; ++i) {
        if (tmp[i] == '/') {
            tmp[i] = 0;
            if (mkdir_if_needed(tmp, 0700) != 0) return -2;
            tmp[i] = '/';
        }
    }
    return 0;
}

static int read_exact(int fd, void *buf, uint32_t len) {
    uint8_t *p = (uint8_t *)buf;
    uint32_t done = 0;
    while (done < len) {
        int64_t n = syscall_read(fd, p + done, len - done);
        if (n == 0) return done == 0 ? 1 : -1;
        if (n < 0) return -2;
        done += (uint32_t)n;
    }
    return 0;
}

static int write_all(int fd, const uint8_t *buf, uint32_t len) {
    uint32_t done = 0;
    while (done < len) {
        int64_t n = syscall_write(fd, buf + done, len - done);
        if (n <= 0) return -1;
        done += (uint32_t)n;
    }
    return 0;
}

static int discard_bytes(int fd, uint64_t len) {
    uint8_t buf[IO_CHUNK];
    while (len) {
        uint32_t want = len > IO_CHUNK ? IO_CHUNK : (uint32_t)len;
        int rc = read_exact(fd, buf, want);
        if (rc != 0) return -1;
        len -= want;
    }
    return 0;
}

static int extract_regular(int tar_fd, const char *rel, uint64_t size, uint32_t mode) {
    char path[PATH_CAP];
    uint8_t buf[IO_CHUNK];
    if (full_path(rel, path) != 0) return -1;
    if (ensure_parent_dirs(path) != 0) return -2;

    int64_t fd = syscall_open(path, RAF_O_WRONLY | RAF_O_CREAT | RAF_O_TRUNC | RAF_O_CLOEXEC,
                              (int)(mode ? (mode & 0777u) : 0600u));
    if (fd < 0) return -3;

    uint64_t remain = size;
    while (remain) {
        uint32_t want = remain > IO_CHUNK ? IO_CHUNK : (uint32_t)remain;
        int rc = read_exact(tar_fd, buf, want);
        if (rc != 0 || write_all((int)fd, buf, want) != 0) {
            (void)syscall_close((int)fd);
            return -4;
        }
        remain -= want;
    }
    if (syscall_fsync((int)fd) < 0) {
        (void)syscall_close((int)fd);
        return -5;
    }
    if (syscall_close((int)fd) < 0) return -6;
    if (syscall_chmod(path, (int)(mode ? (mode & 0777u) : 0600u)) < 0) return -7;
    return 0;
}

static int extract_directory(const char *rel, uint32_t mode) {
    char path[PATH_CAP];
    if (full_path(rel, path) != 0) return -1;
    if (ensure_parent_dirs(path) != 0) return -2;
    return mkdir_if_needed(path, (int)(mode ? (mode & 0777u) : 0700u));
}

static int extract_symlink(const struct tar_header *h, const char *rel) {
    char path[PATH_CAP];
    char target[101];
    uint32_t i = 0;
    while (i < sizeof(h->linkname) && h->linkname[i]) {
        target[i] = h->linkname[i];
        ++i;
    }
    target[i] = 0;
    if (i == 0) return -1;
    if (full_path(rel, path) != 0) return -2;
    if (ensure_parent_dirs(path) != 0) return -3;
    (void)syscall_unlink(path);
    return syscall_symlink(target, path) == 0 ? 0 : -4;
}

static int prefix_stack_already_materialized(void) {
    static const char *required[] = {
        RAFCODEPHI_PREFIX "/bin/sh",
        RAFCODEPHI_PREFIX "/bin/dpkg",
        RAFCODEPHI_PREFIX "/bin/apt",
        RAFCODEPHI_PREFIX "/bin/apt-get",
        RAFCODEPHI_PREFIX "/bin/pkg",
        (const char *)0
    };
    uint32_t i;
    for (i = 0; required[i]; ++i) if (syscall_access(required[i], 0) != 0) return 0;
    return 1;
}

int extract_bootstrap_payload(void) {
    if (prefix_stack_already_materialized()) return 1; /* validated pre-existing payload */

    if (mkdir_if_needed(RAFCODEPHI_PREFIX, 0700) != 0) {
        /* The parent files/ directory may exist while usr does not. */
        return -1;
    }

    int64_t fd = syscall_open(RAFCODEPHI_BOOTSTRAP_TAR, RAF_O_RDONLY | RAF_O_CLOEXEC, 0);
    if (fd < 0) return -2;

    uint8_t block[TAR_BLOCK];
    int saw_symlink = 0;
    uint32_t members = 0;

    for (;;) {
        int rr = read_exact((int)fd, block, TAR_BLOCK);
        if (rr == 1) break;
        if (rr != 0) { (void)syscall_close((int)fd); return -3; }
        if (mem_is_zero(block, TAR_BLOCK)) break;
        if (!tar_checksum_ok(block)) { (void)syscall_close((int)fd); return -4; }

        struct tar_header *h = (struct tar_header *)block;
        char rel[PATH_CAP];
        if (tar_member_name(h, rel) != 0) { (void)syscall_close((int)fd); return -5; }

        int ok_size = 0, ok_mode = 0;
        uint64_t size = parse_octal64(h->size, sizeof(h->size), &ok_size);
        uint64_t mode64 = parse_octal64(h->mode, sizeof(h->mode), &ok_mode);
        if (!ok_size || !ok_mode || mode64 > 07777u) { (void)syscall_close((int)fd); return -6; }
        uint32_t mode = (uint32_t)mode64;
        char type = h->typeflag[0] ? h->typeflag[0] : '0';

        int rc = 0;
        if (type == '0') {
            if (saw_symlink) { (void)syscall_close((int)fd); return -7; }
            rc = extract_regular((int)fd, rel, size, mode);
        } else if (type == '5') {
            if (saw_symlink) { (void)syscall_close((int)fd); return -8; }
            rc = extract_directory(rel, mode);
        } else if (type == '2') {
            /* Symlinks are permitted only in the tail of the archive.  This
             * prevents later regular-file extraction from traversing a link. */
            saw_symlink = 1;
            rc = extract_symlink(h, rel);
            if (size != 0) { (void)syscall_close((int)fd); return -9; }
        } else {
            /* Hardlinks/devices/FIFOs and extended records are not silently
             * accepted by the minimal bootstrap transport. */
            (void)syscall_close((int)fd);
            return -10;
        }
        if (rc != 0) { (void)syscall_close((int)fd); return -11; }

        if (type != '0' && size != 0) {
            if (discard_bytes((int)fd, size) != 0) { (void)syscall_close((int)fd); return -12; }
        }
        uint64_t pad = (TAR_BLOCK - (size & (TAR_BLOCK - 1u))) & (TAR_BLOCK - 1u);
        if (pad && discard_bytes((int)fd, pad) != 0) { (void)syscall_close((int)fd); return -13; }
        ++members;
    }

    (void)syscall_close((int)fd);
    if (members == 0) return -14;
    return prefix_stack_already_materialized() ? 0 : -15;
}

int extract_payload_validate(void) {
    int rc = extract_bootstrap_payload();
    return (rc == 0 || rc == 1) ? 0 : rc;
}
