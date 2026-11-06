/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef RAF_TOOL_NAME_GUARD_H
#define RAF_TOOL_NAME_GUARD_H
/* Pure C11 boundary: one executable BASENAME, not a path or shell expression.
 * No libc, syscalls, globals, allocation or platform headers. */
static inline int raf_tool_name_is_safe(const char *name) {
    unsigned int i = 0u;
    unsigned char c;
    if (!name) return 0;
    c = (unsigned char)name[0];
    if (!((c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_')) return 0;
    for (; (c = (unsigned char)name[i]) != 0u; ++i) {
        if (i >= 127u) return 0;
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '.' || c == '+')) return 0;
    }
    return i != 0u;
}
#endif
