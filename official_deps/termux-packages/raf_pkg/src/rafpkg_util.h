// file: rafpkg_util.h
// RAFAELIA :: tiny util (hosted)
// SPDX-License-Identifier: MIT

#ifndef RAFPKG_UTIL_H
#define RAFPKG_UTIL_H

#include "../include/raf_pkg.h"
#include <stddef.h>

u32 raf_hash_fnv1a(const char *s, u32 len);

int raf_streq(const char *a, const char *b);

// in-place trim of leading/trailing spaces/tabs
char *raf_trim(char *s);

#endif
