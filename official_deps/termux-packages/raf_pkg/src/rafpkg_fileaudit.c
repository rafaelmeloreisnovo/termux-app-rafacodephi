// file: rafpkg_fileaudit.c
// RAFAELIA :: file audit (ELF/scripts/unknown ext) + rafstore cache
// SPDX-License-Identifier: MIT
//
// Walks a directory tree and reports suspicious / unknown files:
//   - ELF binaries/objects
//   - shebang scripts
//   - files without extension
//   - files with unusual extensions
//
// Output is deterministic: directory entries are sorted lexicographically.
// Cache: optional rafstore speeds repeated audits (less physical IO).

#include "rafpkg_fileaudit.h"
#include "rafpkg_store.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef RAF_MAX_PATH
#define RAF_MAX_PATH 4096
#endif

static int str_cmp(const void *a, const void *b) {
  const char *sa = *(const char * const *)a;
  const char *sb = *(const char * const *)b;
  return strcmp(sa, sb);
}

static char *raf_strdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *p = (char*)malloc(n);
  if (p) memcpy(p, s, n);
  return p;
}

static int is_ignored_dir(const char *name) {
  return (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0) ||
         (strcmp(name, ".git") == 0) || (strcmp(name, "build") == 0) ||
         (strcmp(name, "out") == 0) || (strcmp(name, ".gradle") == 0) ||
         (strcmp(name, ".rafstore") == 0);
}

static const char *ext_of(const char *name) {
  const char *dot = strrchr(name, '.');
  if (!dot || dot == name) return NULL;
  return dot + 1;
}

static int is_known_ext(const char *ext) {
  if (!ext) return 0;
  // common text/code
  if (!strcmp(ext,"c")||!strcmp(ext,"h")||!strcmp(ext,"S")||!strcmp(ext,"mk")||!strcmp(ext,"gradle")||
      !strcmp(ext,"java")||!strcmp(ext,"kt")||!strcmp(ext,"xml")||!strcmp(ext,"md")||!strcmp(ext,"txt")||
      !strcmp(ext,"sh")||!strcmp(ext,"py")||!strcmp(ext,"json")||!strcmp(ext,"yml")||!strcmp(ext,"yaml")||
      !strcmp(ext,"properties")||!strcmp(ext,"pro")||!strcmp(ext,"aidl")||!strcmp(ext,"png")||!strcmp(ext,"jpg")||
      !strcmp(ext,"jpeg")||!strcmp(ext,"webp")||!strcmp(ext,"svg")||!strcmp(ext,"so")||!strcmp(ext,"a")||
      !strcmp(ext,"o")||!strcmp(ext,"jar")||!strcmp(ext,"aar")||!strcmp(ext,"zip")||!strcmp(ext,"gz")||!strcmp(ext,"xz"))
    return 1;
  return 0;
}

static u64 mtime_ns_from_stat(const struct stat *st) {
  // Best-effort portability: fallback to seconds.
#if defined(__APPLE__)
  return (u64)st->st_mtimespec.tv_sec * 1000000000ULL;
#elif defined(st_mtim)
  return (u64)st->st_mtim.tv_sec * 1000000000ULL + (u64)st->st_mtim.tv_nsec;
#else
  return (u64)st->st_mtime * 1000000000ULL;
#endif
}

static u8 sniff_kind_uncached(const char *path) {
  unsigned char hdr[4];
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  size_t n = fread(hdr, 1, sizeof(hdr), f);
  fclose(f);
  if (n >= 4 && hdr[0]==0x7f && hdr[1]=='E' && hdr[2]=='L' && hdr[3]=='F') return 1; // elf
  if (n >= 2 && hdr[0]=='#' && hdr[1]=='!') return 2; // script
  return 0;
}

static const char* kind_str(u8 kind) {
  if (kind == 1) return "elf";
  if (kind == 2) return "script";
  if (kind == 3) return "other";
  return "unk";
}

static int ensure_dir(const char *path) {
  if (!path || !path[0]) return 0;
  if (mkdir(path, 0755) == 0) return 1;
  if (errno == EEXIST) return 1;
  return 0;
}

static int walk(const char *base, int depth, int max_depth,
                raf_store_t *cache,
                unsigned long long *files, unsigned long long *dirs,
                unsigned long long *elfs, unsigned long long *scripts,
                unsigned long long *noext, unsigned long long *unknown_ext) {
  if (depth > max_depth) return 1;

  DIR *d = opendir(base);
  if (!d) return 0;

  // collect names for deterministic order
  char **names = NULL;
  size_t cap = 0, cnt = 0;
  for (;;) {
    errno = 0;
    struct dirent *e = readdir(d);
    if (!e) break;
    if (is_ignored_dir(e->d_name)) continue;
    if (cnt == cap) {
      size_t ncap = cap ? cap*2 : 64;
      char **nn = (char**)realloc(names, ncap * sizeof(char*));
      if (!nn) { closedir(d); return 0; }
      names = nn; cap = ncap;
    }
    names[cnt] = raf_strdup(e->d_name);
    if (!names[cnt]) { closedir(d); return 0; }
    cnt++;
  }
  closedir(d);

  qsort(names, cnt, sizeof(char*), str_cmp);

  for (size_t i = 0; i < cnt; i++) {
    char path[RAF_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", base, names[i]);

    struct stat st;
    if (stat(path, &st) != 0) { free(names[i]); continue; }

    if (S_ISDIR(st.st_mode)) {
      (*dirs)++;
      walk(path, depth+1, max_depth, cache, files, dirs, elfs, scripts, noext, unknown_ext);
    } else if (S_ISREG(st.st_mode)) {
      (*files)++;

      const char *ext = ext_of(names[i]);
      if (!ext) (*noext)++;
      else if (!is_known_ext(ext)) (*unknown_ext)++;

      u64 mtime_ns = mtime_ns_from_stat(&st);
      u64 size = (u64)st.st_size;

      // cached kind (if possible)
      u8 kind = 0;
      if (cache && cache->slots) {
        if (!raf_store_get(cache, path, mtime_ns, size, &kind)) {
          kind = sniff_kind_uncached(path);
          raf_store_put(cache, path, mtime_ns, size, kind);
        }
      } else {
        kind = sniff_kind_uncached(path);
      }

      if (kind == 1) (*elfs)++;
      if (kind == 2) (*scripts)++;

      // report only if suspicious
      if (!ext || !is_known_ext(ext) || kind == 1) {
        printf("fileaudit: %s kind=%s ext=%s size=%llu\n",
               path,
               kind_str(kind),
               ext ? ext : "(none)",
               (unsigned long long)st.st_size);
      }
    }
    free(names[i]);
  }
  free(names);
  return 1;
}

int raf_fileaudit_run_cached(const char *root, int max_depth, const char *cache_path) {
  if (!root || !root[0]) root = ".";
  if (max_depth < 0) max_depth = 32;

  raf_store_t st;
  raf_store_init(&st);

  // default cache path
  char default_cache[RAF_MAX_PATH];
  const char *cache_use = cache_path;
  if (!cache_use || !cache_use[0]) {
    // enable by default
    ensure_dir(".rafstore");
    snprintf(default_cache, sizeof(default_cache), "%s", ".rafstore/fileaudit.bin");
    cache_use = default_cache;
  } else {
    // user-specified: try create parent if it's under .rafstore
    // (best-effort; ignore errors)
    if (strstr(cache_use, ".rafstore") == cache_use) ensure_dir(".rafstore");
  }

  int cache_ok = 1;
  if (cache_use && cache_use[0]) cache_ok = raf_store_load(&st, cache_use);
  if (!cache_ok) {
    // disable cache if load failed
    raf_store_free(&st);
    raf_store_init(&st);
  }

  unsigned long long files=0, dirs=0, elfs=0, scripts=0, noext=0, unknown_ext=0;
  int ok = walk(root, 0, max_depth, (st.slots ? &st : NULL), &files, &dirs, &elfs, &scripts, &noext, &unknown_ext);
  if (!ok) { raf_store_free(&st); return 2; }

  fprintf(stdout,
          "fileaudit: root=%s dirs=%llu files=%llu elf=%llu scripts=%llu noext=%llu unknown_ext=%llu\n",
          root, dirs, files, elfs, scripts, noext, unknown_ext);

  if (st.slots && cache_use && cache_use[0]) (void)raf_store_save(&st, cache_use);
  raf_store_free(&st);
  return 0;
}

int raf_fileaudit_run(const char *root, int max_depth) {
  return raf_fileaudit_run_cached(root, max_depth, NULL);
}
