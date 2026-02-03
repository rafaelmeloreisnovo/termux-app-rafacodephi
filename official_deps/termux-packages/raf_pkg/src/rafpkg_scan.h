// file: rafpkg_scan.h
// RAFAELIA :: scan package directories
// SPDX-License-Identifier: MIT

#ifndef RAFPKG_SCAN_H
#define RAFPKG_SCAN_H

#include "rafpkg_parse.h"

// scan roots (array of dirs) and fill db with packages
int raf_scan_roots(raf_db_t *db, const char **roots, u32 roots_cnt, const char *arch);

#endif
