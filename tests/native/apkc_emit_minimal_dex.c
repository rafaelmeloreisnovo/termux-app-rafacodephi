#define RAF_APKC_HOST_TEST 1
#include "fmt_dex.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fputs("usage: apkc_emit_minimal_dex OUTPUT\n", stderr);
        return 64;
    }

    u8 dex[DEX_MINIMAL_SZ];
    if (dex_build_checked(NULL, (sz)DEX_MINIMAL_SZ) != 0u) return 65;
    if (dex_build_checked(dex, (sz)(DEX_MINIMAL_SZ - 1u)) != 0u) return 66;

    const u32 written = dex_build_checked(dex, (sz)sizeof(dex));
    if (written != DEX_MINIMAL_SZ) return 67;

    FILE *out = fopen(argv[1], "wb");
    if (!out) return 68;
    const size_t count = fwrite(dex, 1u, (size_t)written, out);
    const int close_status = fclose(out);
    if (count != (size_t)written || close_status != 0) return 69;

    return 0;
}
