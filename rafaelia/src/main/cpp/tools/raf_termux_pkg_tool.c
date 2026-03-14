#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} str_list_t;

static char *dup_cstr(const char *s) {
    size_t n = strlen(s);
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n + 1);
    return out;
}

static void list_free(str_list_t *ls) {
    size_t i;
    for (i = 0; i < ls->len; ++i) free(ls->items[i]);
    free(ls->items);
    ls->items = NULL;
    ls->len = 0;
    ls->cap = 0;
}

static int list_push(str_list_t *ls, const char *s) {
    if (ls->len == ls->cap) {
        size_t ncap = ls->cap ? ls->cap * 2 : 128;
        char **tmp = (char **)realloc(ls->items, ncap * sizeof(char *));
        if (!tmp) return -1;
        ls->items = tmp;
        ls->cap = ncap;
    }
    ls->items[ls->len] = dup_cstr(s);
    if (!ls->items[ls->len]) return -1;
    ls->len++;
    return 0;
}

static int cmp_str_ptr(const void *a, const void *b) {
    const char *sa = *(const char *const *)a;
    const char *sb = *(const char *const *)b;
    return strcmp(sa, sb);
}

static uint32_t fnv1a32(const char *s) {
    uint32_t h = 2166136261u;
    const unsigned char *p = (const unsigned char *)s;
    while (*p) {
        h ^= (uint32_t)(*p++);
        h *= 16777619u;
    }
    return h;
}

static int is_dir_with_build_sh(const char *packages_dir, const char *name) {
    char path[PATH_MAX];
    struct stat st;
    if (snprintf(path, sizeof(path), "%s/%s", packages_dir, name) >= (int)sizeof(path)) return 0;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) return 0;
    if (snprintf(path, sizeof(path), "%s/%s/build.sh", packages_dir, name) >= (int)sizeof(path)) return 0;
    return access(path, R_OK) == 0;
}

static int collect_packages(const char *repo_path, str_list_t *out) {
    char packages_dir[PATH_MAX];
    DIR *d;
    struct dirent *de;
    if (snprintf(packages_dir, sizeof(packages_dir), "%s/packages", repo_path) >= (int)sizeof(packages_dir)) return -1;
    d = opendir(packages_dir);
    if (!d) {
        fprintf(stderr, "[tool] erro abrindo %s: %s\n", packages_dir, strerror(errno));
        return -1;
    }
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        if (is_dir_with_build_sh(packages_dir, de->d_name)) {
            if (list_push(out, de->d_name) != 0) {
                closedir(d);
                return -1;
            }
        }
    }
    closedir(d);
    qsort(out->items, out->len, sizeof(char *), cmp_str_ptr);
    return 0;
}

static char *read_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    long sz;
    char *buf;
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (sz > 0 && fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);
    buf[sz] = '\0';
    if (out_size) *out_size = (size_t)sz;
    return buf;
}

static int write_atomic(const char *path, const char *data, size_t size) {
    char tmp[PATH_MAX];
    FILE *f;
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp)) return -1;
    f = fopen(tmp, "wb");
    if (!f) return -1;
    if (size && fwrite(data, 1, size, f) != size) {
        fclose(f);
        unlink(tmp);
        return -1;
    }
    if (fclose(f) != 0) {
        unlink(tmp);
        return -1;
    }
    if (rename(tmp, path) != 0) {
        unlink(tmp);
        return -1;
    }
    return 0;
}

static int ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return S_ISDIR(st.st_mode) ? 0 : -1;
    if (mkdir(path, 0755) == 0) return 0;
    return -1;
}

static int build_rows(const str_list_t *pkgs, char **out_rows) {
    size_t i;
    size_t cap = pkgs->len * 64 + 1;
    size_t pos = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) return -1;
    buf[0] = '\0';
    for (i = 0; i < pkgs->len; ++i) {
        const char *name = pkgs->items[i];
        uint32_t id = fnv1a32(name);
        size_t need = strlen(name) + 80;
        int n;
        if (pos + need >= cap) {
            size_t ncap = cap * 2 + need;
            char *tmp = (char *)realloc(buf, ncap);
            if (!tmp) {
                free(buf);
                return -1;
            }
            buf = tmp;
            cap = ncap;
        }
        n = snprintf(buf + pos, cap - pos, "  { 0x%08xu, %zuu, 0u }, /* %s */\n", id, strlen(name), name);
        if (n < 0) {
            free(buf);
            return -1;
        }
        pos += (size_t)n;
    }
    *out_rows = buf;
    return 0;
}

static int replace_token(const char *src, const char *token, const char *value, char **out) {
    const char *p = src;
    size_t token_len = strlen(token);
    size_t value_len = strlen(value);
    size_t out_cap = strlen(src) + value_len + 128;
    size_t out_pos = 0;
    char *buf = (char *)malloc(out_cap);
    if (!buf) return -1;
    while (*p) {
        const char *m = strstr(p, token);
        size_t copy_len;
        if (!m) {
            copy_len = strlen(p);
        } else {
            copy_len = (size_t)(m - p);
        }
        if (out_pos + copy_len + value_len + 1 >= out_cap) {
            size_t ncap = out_cap * 2 + copy_len + value_len + 1;
            char *tmp = (char *)realloc(buf, ncap);
            if (!tmp) {
                free(buf);
                return -1;
            }
            buf = tmp;
            out_cap = ncap;
        }
        memcpy(buf + out_pos, p, copy_len);
        out_pos += copy_len;
        if (!m) break;
        memcpy(buf + out_pos, value, value_len);
        out_pos += value_len;
        p = m + token_len;
    }
    buf[out_pos] = '\0';
    *out = buf;
    return 0;
}

static int cmd_gen_packages_c(const char *repo, const char *template_path, const char *out_c) {
    str_list_t pkgs = {0};
    char *tpl = NULL;
    char *rows = NULL;
    char count_buf[64];
    char *stage1 = NULL;
    char *final = NULL;
    size_t tpl_sz = 0;
    int rc = -1;

    if (collect_packages(repo, &pkgs) != 0) goto done;
    if (build_rows(&pkgs, &rows) != 0) goto done;
    tpl = read_file(template_path, &tpl_sz);
    if (!tpl) {
        fprintf(stderr, "[tool] erro lendo template %s\n", template_path);
        goto done;
    }
    snprintf(count_buf, sizeof(count_buf), "%zu", pkgs.len);
    if (replace_token(tpl, "__PKG_ROWS__", rows, &stage1) != 0) goto done;
    if (replace_token(stage1, "__PKG_COUNT__", count_buf, &final) != 0) goto done;
    if (write_atomic(out_c, final, strlen(final)) != 0) {
        fprintf(stderr, "[tool] erro gravando %s\n", out_c);
        goto done;
    }
    printf("[tool] pacotes: %zu\n", pkgs.len);
    rc = 0;

done:
    free(tpl);
    free(rows);
    free(stage1);
    free(final);
    list_free(&pkgs);
    return rc;
}

static void trim_line(char *s) {
    size_t len;
    while (*s && isspace((unsigned char)*s)) s++;
    len = strlen(s);
    while (len && (s[len - 1] == '\r' || s[len - 1] == '\n')) s[--len] = '\0';
}

static void unquote_inplace(char *s) {
    size_t len = strlen(s);
    while (*s && isspace((unsigned char)*s)) s++;
    len = strlen(s);
    while (len && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    if (len >= 2 && ((s[0] == '"' && s[len - 1] == '"') || (s[0] == '\'' && s[len - 1] == '\''))) {
        memmove(s, s + 1, len - 2);
        s[len - 2] = '\0';
    }
}

static void parse_meta_file(const char *build_sh, char *version, size_t vcap, char *homepage, size_t hcap, char *desc, size_t dcap) {
    FILE *f = fopen(build_sh, "r");
    char line[8192];
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        char *t = line;
        while (*t && isspace((unsigned char)*t)) t++;
        if (!version[0] && strncmp(t, "TERMUX_PKG_VERSION=", 19) == 0) {
            strncpy(version, t + 19, vcap - 1);
            version[vcap - 1] = '\0';
            trim_line(version);
            unquote_inplace(version);
        } else if (!homepage[0] && strncmp(t, "TERMUX_PKG_HOMEPAGE=", 20) == 0) {
            strncpy(homepage, t + 20, hcap - 1);
            homepage[hcap - 1] = '\0';
            trim_line(homepage);
            unquote_inplace(homepage);
        } else if (!desc[0] && strncmp(t, "TERMUX_PKG_DESCRIPTION=", 23) == 0) {
            strncpy(desc, t + 23, dcap - 1);
            desc[dcap - 1] = '\0';
            trim_line(desc);
            unquote_inplace(desc);
        }
        if (version[0] && homepage[0] && desc[0]) break;
    }
    fclose(f);
}

static int clean_rafpkg_files(const char *out_dir) {
    DIR *d = opendir(out_dir);
    struct dirent *de;
    if (!d) return -1;
    while ((de = readdir(d)) != NULL) {
        size_t len;
        char path[PATH_MAX];
        if (de->d_name[0] == '.') continue;
        len = strlen(de->d_name);
        if (len < 7 || strcmp(de->d_name + len - 7, ".rafpkg") != 0) continue;
        if (snprintf(path, sizeof(path), "%s/%s", out_dir, de->d_name) >= (int)sizeof(path)) continue;
        unlink(path);
    }
    closedir(d);
    return 0;
}

static int write_manifest(const char *out_dir, const char *name, const char *version, const char *homepage, const char *desc) {
    char path[PATH_MAX];
    FILE *f;
    uint32_t id = fnv1a32(name);
    if (snprintf(path, sizeof(path), "%s/%s.rafpkg", out_dir, name) >= (int)sizeof(path)) return -1;
    f = fopen(path, "wb");
    if (!f) return -1;
    fprintf(f, "seal=RAFPKG\n");
    fprintf(f, "name=%s\n", name);
    fprintf(f, "id=0x%08x\n", id);
    fprintf(f, "name_len=%zu\n", strlen(name));
    fprintf(f, "version=%s\n", version);
    fprintf(f, "homepage=%s\n", homepage);
    fprintf(f, "description=%s\n", desc);
    fprintf(f, "source=termux/termux-packages\n");
    fprintf(f, "path=packages/%s/build.sh\n", name);
    if (fclose(f) != 0) return -1;
    return 0;
}

static int write_index(const char *out_dir, const char *upstream_url, size_t total, size_t exported) {
    char path[PATH_MAX];
    FILE *f;
    if (snprintf(path, sizeof(path), "%s/INDEX.rafidx", out_dir) >= (int)sizeof(path)) return -1;
    f = fopen(path, "wb");
    if (!f) return -1;
    fprintf(f, "seal=RAFINDEX\n");
    fprintf(f, "upstream=%s\n", upstream_url);
    fprintf(f, "total_upstream_packages=%zu\n", total);
    fprintf(f, "exported_packages=%zu\n", exported);
    fprintf(f, "format=.rafpkg\n");
    if (fclose(f) != 0) return -1;
    return 0;
}

static int cmd_export_manifests(const char *repo, const char *out_dir, long limit, const char *upstream_url) {
    str_list_t pkgs = {0};
    size_t i;
    size_t exported;
    if (collect_packages(repo, &pkgs) != 0) {
        list_free(&pkgs);
        return -1;
    }
    if (ensure_dir(out_dir) != 0) {
        fprintf(stderr, "[tool] erro criando %s\n", out_dir);
        list_free(&pkgs);
        return -1;
    }
    if (clean_rafpkg_files(out_dir) != 0) {
        fprintf(stderr, "[tool] erro limpando .rafpkg em %s\n", out_dir);
        list_free(&pkgs);
        return -1;
    }
    exported = pkgs.len;
    if (limit > 0 && (size_t)limit < exported) exported = (size_t)limit;
    for (i = 0; i < exported; ++i) {
        char build_path[PATH_MAX];
        char version[1024] = {0};
        char homepage[2048] = {0};
        char desc[4096] = {0};
        if (snprintf(build_path, sizeof(build_path), "%s/packages/%s/build.sh", repo, pkgs.items[i]) >= (int)sizeof(build_path)) {
            list_free(&pkgs);
            return -1;
        }
        parse_meta_file(build_path, version, sizeof(version), homepage, sizeof(homepage), desc, sizeof(desc));
        if (write_manifest(out_dir, pkgs.items[i], version, homepage, desc) != 0) {
            fprintf(stderr, "[tool] erro gerando .rafpkg para %s\n", pkgs.items[i]);
            list_free(&pkgs);
            return -1;
        }
    }
    if (write_index(out_dir, upstream_url, pkgs.len, exported) != 0) {
        list_free(&pkgs);
        return -1;
    }
    printf("[tool] upstream=%zu exported=%zu out=%s\n", pkgs.len, exported, out_dir);
    list_free(&pkgs);
    return 0;
}

static void print_usage(const char *argv0) {
    fprintf(stderr, "uso:\n");
    fprintf(stderr, "  %s gen-packages-c <repo> <template> <out_c>\n", argv0);
    fprintf(stderr, "  %s export-manifests <repo> <out_dir> <limit> <upstream_url>\n", argv0);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "gen-packages-c") == 0) {
        if (argc != 5) {
            print_usage(argv[0]);
            return 1;
        }
        return cmd_gen_packages_c(argv[2], argv[3], argv[4]) == 0 ? 0 : 1;
    }
    if (strcmp(argv[1], "export-manifests") == 0) {
        long limit;
        char *end = NULL;
        if (argc != 6) {
            print_usage(argv[0]);
            return 1;
        }
        errno = 0;
        limit = strtol(argv[4], &end, 10);
        if (errno != 0 || !end || *end != '\0') {
            fprintf(stderr, "[tool] limite invalido: %s\n", argv[4]);
            return 1;
        }
        return cmd_export_manifests(argv[2], argv[3], limit, argv[5]) == 0 ? 0 : 1;
    }
    print_usage(argv[0]);
    return 1;
}
