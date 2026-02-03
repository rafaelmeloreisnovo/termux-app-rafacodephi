// file: raf_pkg.h
// RAFAELIA :: rafpkg public interface (minimal)
// SPDX-License-Identifier: MIT

#ifndef RAF_PKG_H
#define RAF_PKG_H

#include "raf_pkg_types.h"

#define RAFPKG_MAX_LINE 4096
#define RAFPKG_MAX_DEPS 512
#define RAFPKG_MAX_PKGS 8192

typedef struct {
  char *s;
  u32   len;
} raf_str_t;

typedef struct {
  raf_str_t name;      // package name
  raf_str_t dir;       // directory path
  u32 dep_off;         // offset into deps pool
  u16 dep_cnt;         // number of deps
  u16 excluded;        // 1 if excluded for current arch
} raf_pkg_t;

typedef struct {
  raf_pkg_t *pkgs;
  u32        pkgs_cnt;

  // deps pool: concatenated NUL-terminated strings
  char      *deps_pool;
  u32        deps_pool_len;
  u32        deps_pool_cap;

  // deps index pool: offsets into deps_pool
  u32       *dep_idx;
  u32        dep_idx_len;
  u32        dep_idx_cap;
} raf_db_t;

#endif // RAF_PKG_H
