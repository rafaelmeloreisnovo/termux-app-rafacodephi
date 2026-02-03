// file: rafpkg_parse.h
// RAFAELIA :: parse termux build.sh variables (static)
// SPDX-License-Identifier: MIT

#ifndef RAFPKG_PARSE_H
#define RAFPKG_PARSE_H

#include "rafpkg_db.h"

// parse build.sh and append deps into db for pkg_idx
// sets db->pkgs[pkg_idx].excluded if TERMUX_PKG_EXCLUDED_ARCHES contains current arch
int raf_parse_build_sh(raf_db_t *db, u32 pkg_idx, const char *build_sh_path, const char *arch);

#endif
