#define RAF_APKC_HOST_TEST 1
#include "fmt_dex_one_class.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fputs("usage: apkc_emit_one_class_dex OUTPUT\n", stderr);
        return 64;
    }

    u8 dex[DEX_ONE_CLASS_SZ];
    if (dex_build_one_class_checked(NULL, (sz)DEX_ONE_CLASS_SZ) != 0u) return 65;
    if (dex_build_one_class_checked(dex, (sz)(DEX_ONE_CLASS_SZ - 1u)) != 0u) return 66;

    const u32 written = dex_build_one_class_checked(dex, (sz)sizeof(dex));
    if (written != DEX_ONE_CLASS_SZ) return 67;

    FILE *out = fopen(argv[1], "wb");
    if (!out) return 68;
    const size_t count = fwrite(dex, 1u, (size_t)written, out);
    const int close_status = fclose(out);
    if (count != (size_t)written || close_status != 0) return 69;
    return 0;
}
