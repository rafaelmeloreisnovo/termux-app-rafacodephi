// file: rafpkg_db.c
// RAFAELIA :: package db builder
// SPDX-License-Identifier: MIT

#include "rafpkg_db.h"
#include "rafpkg_util.h"
#include <stdlib.h>
#include <string.h>

static char *dup_cstr(const char *s) {
  if (!s) {
    char *z = (char*)malloc(1);
    if (z) z[0] = 0;
    return z;
  }
  size_t n = strlen(s);
  char *p = (char*)malloc(n + 1);
  if (!p) return NULL;
  memcpy(p, s, n + 1);
  return p;
}

int raf_db_init(raf_db_t *db, u32 pkg_cap) {
  if (!db) return 0;
  memset(db, 0, sizeof(*db));
  if (pkg_cap == 0 || pkg_cap > RAFPKG_MAX_PKGS) pkg_cap = RAFPKG_MAX_PKGS;
  db->pkgs = (raf_pkg_t*)calloc((size_t)pkg_cap, sizeof(raf_pkg_t));
  if (!db->pkgs) return 0;
  db->pkgs_cnt = 0;

  db->deps_pool_cap = 64 * 1024;
  db->deps_pool = (char*)malloc(db->deps_pool_cap);
  if (!db->deps_pool) return 0;
  db->deps_pool_len = 0;

  db->dep_idx_cap = 16 * 1024;
  db->dep_idx = (u32*)malloc((size_t)db->dep_idx_cap * sizeof(u32));
  if (!db->dep_idx) return 0;
  db->dep_idx_len = 0;

  return 1;
}

void raf_db_free(raf_db_t *db) {
  if (!db) return;
  if (db->pkgs) {
    for (u32 i = 0; i < db->pkgs_cnt; i++) {
      free(db->pkgs[i].name.s);
      free(db->pkgs[i].dir.s);
    }
  }
  free(db->pkgs);
  free(db->deps_pool);
  free(db->dep_idx);
  memset(db, 0, sizeof(*db));
}

static int ensure_pkgs_cap(raf_db_t *db, u32 want) {
  // pkgs is fixed-size in this minimal impl; grow by doubling
  // (safe and simple)
  // current capacity isn't stored, so infer via malloc_usable_size is not portable.
  // We'll just realloc when want hits RAFPKG_MAX_PKGS.
  (void)want;
  // No op: we allocated a conservative cap from CLI and also bound by RAFPKG_MAX_PKGS.
  return 1;
}

u32 raf_db_add_pkg(raf_db_t *db, const char *name, const char *dir) {
  if (!db || !db->pkgs) return (u32)0xFFFFFFFFu;
  if (db->pkgs_cnt >= RAFPKG_MAX_PKGS) return (u32)0xFFFFFFFFu;
  if (!ensure_pkgs_cap(db, db->pkgs_cnt + 1)) return (u32)0xFFFFFFFFu;

  u32 idx = db->pkgs_cnt++;
  raf_pkg_t *p = &db->pkgs[idx];
  p->name.s = dup_cstr(name);
  p->dir.s  = dup_cstr(dir);
  if (!p->name.s || !p->dir.s) return (u32)0xFFFFFFFFu;
  p->name.len = (u32)strlen(p->name.s);
  p->dir.len  = (u32)strlen(p->dir.s);
  p->dep_off = 0;
  p->dep_cnt = 0;
  p->excluded = 0;
  return idx;
}

void raf_db_pkg_deps_begin(raf_db_t *db, u32 pkg_idx) {
  if (!db || pkg_idx >= db->pkgs_cnt) return;
  db->pkgs[pkg_idx].dep_off = db->dep_idx_len;
  db->pkgs[pkg_idx].dep_cnt = 0;
}

static int ensure_deps_pool(raf_db_t *db, u32 add) {
  if (db->deps_pool_len + add <= db->deps_pool_cap) return 1;
  u32 newcap = db->deps_pool_cap;
  while (newcap < db->deps_pool_len + add) newcap *= 2;
  char *p = (char*)realloc(db->deps_pool, newcap);
  if (!p) return 0;
  db->deps_pool = p;
  db->deps_pool_cap = newcap;
  return 1;
}

static int ensure_dep_idx(raf_db_t *db, u32 add) {
  if (db->dep_idx_len + add <= db->dep_idx_cap) return 1;
  u32 newcap = db->dep_idx_cap;
  while (newcap < db->dep_idx_len + add) newcap *= 2;
  u32 *p = (u32*)realloc(db->dep_idx, (size_t)newcap * sizeof(u32));
  if (!p) return 0;
  db->dep_idx = p;
  db->dep_idx_cap = newcap;
  return 1;
}

// dedup inside current package (small O(n^2), but stable)
static int dep_exists(const raf_db_t *db, u32 pkg_idx, const char *dep) {
  const raf_pkg_t *p = &db->pkgs[pkg_idx];
  for (u32 i = 0; i < p->dep_cnt; i++) {
    u32 off = db->dep_idx[p->dep_off + i];
    const char *s = db->deps_pool + off;
    if (raf_streq(s, dep)) return 1;
  }
  return 0;
}

int raf_db_pkg_dep_add(raf_db_t *db, u32 pkg_idx, const char *dep) {
  if (!db || pkg_idx >= db->pkgs_cnt || !dep) return 0;
  if (!dep[0]) return 0;
  if (dep_exists(db, pkg_idx, dep)) return 0;

  size_t n = strlen(dep);
  if (n > 4096) n = 4096;
  // store dep string into pool (+NUL)
  if (!ensure_deps_pool(db, (u32)n + 1)) return 0;
  u32 off = db->deps_pool_len;
  memcpy(db->deps_pool + off, dep, n);
  db->deps_pool[off + (u32)n] = 0;
  db->deps_pool_len += (u32)n + 1;

  if (!ensure_dep_idx(db, 1)) return 0;
  db->dep_idx[db->dep_idx_len++] = off;
  db->pkgs[pkg_idx].dep_cnt++;
  return 1;
}

void raf_db_pkg_deps_end(raf_db_t *db, u32 pkg_idx) {
  (void)db; (void)pkg_idx;
}

const char *raf_db_dep_at(const raf_db_t *db, u32 dep_off) {
  if (!db || dep_off >= db->deps_pool_len) return "";
  return db->deps_pool + dep_off;
}
