// file: rafpkg_scan.c
// RAFAELIA :: scan package directories
// SPDX-License-Identifier: MIT

#include "rafpkg_scan.h"
#include "rafpkg_util.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int is_dir(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0) return 0;
  return S_ISDIR(st.st_mode) ? 1 : 0;
}

static int file_exists(const char *path) {
  struct stat st;
  return (stat(path, &st) == 0) ? 1 : 0;
}

static void join_path(char *dst, size_t cap, const char *a, const char *b) {
  if (!dst || cap == 0) return;
  size_t la = strlen(a);
  size_t lb = strlen(b);
  size_t need = la + 1 + lb + 1;
  if (need > cap) {
    dst[0] = 0;
    return;
  }
  memcpy(dst, a, la);
  if (la && a[la-1] != '/') {
    dst[la] = '/';
    memcpy(dst + la + 1, b, lb);
    dst[la + 1 + lb] = 0;
  } else {
    memcpy(dst + la, b, lb);
    dst[la + lb] = 0;
  }
}

int raf_scan_roots(raf_db_t *db, const char **roots, u32 roots_cnt, const char *arch) {
  if (!db || !roots || roots_cnt == 0 || !arch) return 0;

  for (u32 r = 0; r < roots_cnt; r++) {
    const char *root = roots[r];
    if (!root || !root[0]) continue;
    if (!is_dir(root)) continue;

    DIR *d = opendir(root);
    if (!d) continue;

    struct dirent *e;
    while ((e = readdir(d))) {
      if (!e->d_name[0] || e->d_name[0] == '.') continue;

      char pkg_dir[4096];
      join_path(pkg_dir, sizeof(pkg_dir), root, e->d_name);
      if (!pkg_dir[0]) continue;
      if (!is_dir(pkg_dir)) continue;

      char build_sh[4096];
      join_path(build_sh, sizeof(build_sh), pkg_dir, "build.sh");
      if (!build_sh[0] || !file_exists(build_sh)) continue;

      u32 idx = raf_db_add_pkg(db, e->d_name, pkg_dir);
      if (idx == 0xFFFFFFFFu) continue;

      // parse deps + excluded arches
      raf_parse_build_sh(db, idx, build_sh, arch);
    }

    closedir(d);
  }

  return 1;
}
