/* SPDX-License-Identifier: GPL-3.0-only */
#include "../bootstrap/raf_guard_v1/raf_tool_name_guard.h"
#include <stdio.h>
static int checks = 0;
#define TEST(s, expected) do { int result = raf_tool_name_is_safe((s)); ++checks; if (result != (expected)) { fprintf(stderr,"case %d: expected %d got %d\n",checks,(expected),result); return 1; } } while(0)
int main(void) {
    const char *allow[] = {"pkg", "apt-get", "proot", "proot-distro", "ninja", "clang", "cmake", "qemu-system-x86_64", "g++", "python3.12", "_shim", "rafproot-fs"};
    const char *deny[] = {"", ".", "..", "../pkg", "../../etc/passwd", "foo/bar", "/system/bin/sh", "bin\\sh", "a b", "a\nb", "a\rb", "$(id)", "-n", ".hidden", "a:b", "a;b", "x|y", "x&y", "\303\251"};
    char limit127[128], limit128[129];
    unsigned int i;
    for (i = 0u; i < sizeof(allow) / sizeof(allow[0]); ++i) TEST(allow[i], 1);
    for (i = 0u; i < sizeof(deny) / sizeof(deny[0]); ++i) TEST(deny[i], 0);
    TEST((const char *)0, 0);
    for (i = 0u; i < 127u; ++i) { limit127[i] = 'a'; }
    limit127[127] = '\0';
    for (i = 0u; i < 128u; ++i) { limit128[i] = 'b'; }
    limit128[128] = '\0';
    TEST(limit127, 1); TEST(limit128, 0);
    puts("RAF_TOOL_NAME_GUARD_PASS");
    return 0;
}
