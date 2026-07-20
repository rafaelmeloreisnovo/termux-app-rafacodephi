#define RAF_APKC_HOST_TEST 1
#include "fmt_elf.h"

#include <stdio.h>

static int write_file(const char *path, const u8 *data, u32 size) {
    FILE *out = fopen(path, "wb");
    if (!out) return 1;
    const size_t count = fwrite(data, 1u, (size_t)size, out);
    const int close_status = fclose(out);
    return count == (size_t)size && close_status == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fputs("usage: apkc_emit_exec_elf ARM32_OUTPUT ARM64_OUTPUT\n", stderr);
        return 64;
    }

    u8 arm32[APKC_ELF_EXEC_SZ];
    u8 arm64[APKC_ELF_EXEC_SZ];

    if (apkc_elf32_arm_exec_build_checked(NULL, (sz)APKC_ELF_EXEC_SZ) != 0u) return 65;
    if (apkc_elf32_arm_exec_build_checked(arm32, (sz)(APKC_ELF_EXEC_SZ - 1u)) != 0u) return 66;
    if (apkc_elf64_aarch64_exec_build_checked(NULL, (sz)APKC_ELF_EXEC_SZ) != 0u) return 67;
    if (apkc_elf64_aarch64_exec_build_checked(arm64, (sz)(APKC_ELF_EXEC_SZ - 1u)) != 0u) return 68;

    const u32 size32 = apkc_elf32_arm_exec_build_checked(arm32, (sz)sizeof(arm32));
    const u32 size64 = apkc_elf64_aarch64_exec_build_checked(arm64, (sz)sizeof(arm64));
    if (size32 != APKC_ELF_EXEC_SZ || size64 != APKC_ELF_EXEC_SZ) return 69;

    if (write_file(argv[1], arm32, size32) != 0) return 70;
    if (write_file(argv[2], arm64, size64) != 0) return 71;
    return 0;
}
