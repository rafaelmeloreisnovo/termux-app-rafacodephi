// file: rafpkg_db.h
// RAFAELIA :: package db builder
// SPDX-License-Identifier: MIT

#ifndef RAFPKG_DB_H
#define RAFPKG_DB_H

#include "../include/raf_pkg.h"

int  raf_db_init(raf_db_t *db, u32 pkg_cap);
void raf_db_free(raf_db_t *db);

u32  raf_db_add_pkg(raf_db_t *db, const char *name, const char *dir);

// begin deps capture for package idx
void raf_db_pkg_deps_begin(raf_db_t *db, u32 pkg_idx);
// add dep; returns 1 if added (dedupbed), 0 if skipped
int  raf_db_pkg_dep_add(raf_db_t *db, u32 pkg_idx, const char *dep);
// finalize dep count
void raf_db_pkg_deps_end(raf_db_t *db, u32 pkg_idx);

// iterate deps (NUL-terminated) for a pkg
const char *raf_db_dep_at(const raf_db_t *db, u32 dep_off);

#endif
