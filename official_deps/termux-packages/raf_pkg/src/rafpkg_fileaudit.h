// file: rafpkg_fileaudit.h
// RAFAELIA :: file audit (detect ELF/scripts/unknown)
// SPDX-License-Identifier: MIT

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Backwards compatible: uses default cache path (".rafstore/fileaudit.bin")
int raf_fileaudit_run(const char *root, int max_depth);

// Explicit cache path (NULL/"" disables cache)
int raf_fileaudit_run_cached(const char *root, int max_depth, const char *cache_path);

#ifdef __cplusplus
}
#endif
