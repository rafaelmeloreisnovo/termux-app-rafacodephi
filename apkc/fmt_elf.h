/* fmt_elf.h — Minimal structural ELF emitters for APKC.
 *
 * Produces bounded little-endian ET_REL containers for:
 *   - ELF32 / EM_ARM / EABI5
 *   - ELF64 / EM_AARCH64
 *
 * Each artifact contains an ELF header followed by the mandatory null section
 * header. There are deliberately no program headers, loadable segments,
 * symbols, relocations or executable code.
 *
 * This is a structural format primitive. It is not a linker, executable
 * writer, dynamic loader or proof of runtime execution.
 */
#pragma once
#include "mem.h"

#define APKC_ELFCLASS32       1u
#define APKC_ELFCLASS64       2u
#define APKC_ELFDATA2LSB      1u
#define APKC_EV_CURRENT       1u
#define APKC_ET_REL           1u
#define APKC_EM_ARM          40u
#define APKC_EM_AARCH64     183u
#define APKC_EF_ARM_EABI5 0x05000000u

#define APKC_ELF32_EHDR_SZ 52u
#define APKC_ELF32_SHDR_SZ 40u
#define APKC_ELF32_MIN_SZ  (APKC_ELF32_EHDR_SZ + APKC_ELF32_SHDR_SZ)

#define APKC_ELF64_EHDR_SZ 64u
#define APKC_ELF64_SHDR_SZ 64u
#define APKC_ELF64_MIN_SZ  (APKC_ELF64_EHDR_SZ + APKC_ELF64_SHDR_SZ)

static inline void apkc_elf_ident(u8 *out, u8 elf_class) {
    out[0] = 0x7fu;
    out[1] = 'E';
    out[2] = 'L';
    out[3] = 'F';
    out[4] = elf_class;
    out[5] = APKC_ELFDATA2LSB;
    out[6] = APKC_EV_CURRENT;
    out[7] = 0u; /* ELFOSABI_SYSV */
    out[8] = 0u; /* ABI version */
}

static inline u32 apkc_elf32_arm_build_checked(u8 *out, sz cap) {
    if (!out || cap < (sz)APKC_ELF32_MIN_SZ) return 0u;

    m_set(out, 0u, (sz)APKC_ELF32_MIN_SZ);
    apkc_elf_ident(out, APKC_ELFCLASS32);

    w16(out + 16u, APKC_ET_REL);
    w16(out + 18u, APKC_EM_ARM);
    w32(out + 20u, APKC_EV_CURRENT);
    w32(out + 24u, 0u);                    /* e_entry */
    w32(out + 28u, 0u);                    /* e_phoff */
    w32(out + 32u, APKC_ELF32_EHDR_SZ);    /* e_shoff */
    w32(out + 36u, APKC_EF_ARM_EABI5);     /* e_flags */
    w16(out + 40u, APKC_ELF32_EHDR_SZ);
    w16(out + 42u, 0u);                    /* e_phentsize */
    w16(out + 44u, 0u);                    /* e_phnum */
    w16(out + 46u, APKC_ELF32_SHDR_SZ);
    w16(out + 48u, 1u);                    /* null section only */
    w16(out + 50u, 0u);                    /* no shstrtab */

    /* Section header zero is required to be all-zero and was cleared above. */
    return APKC_ELF32_MIN_SZ;
}

static inline u32 apkc_elf64_aarch64_build_checked(u8 *out, sz cap) {
    if (!out || cap < (sz)APKC_ELF64_MIN_SZ) return 0u;

    m_set(out, 0u, (sz)APKC_ELF64_MIN_SZ);
    apkc_elf_ident(out, APKC_ELFCLASS64);

    w16(out + 16u, APKC_ET_REL);
    w16(out + 18u, APKC_EM_AARCH64);
    w32(out + 20u, APKC_EV_CURRENT);
    w64(out + 24u, 0u);                    /* e_entry */
    w64(out + 32u, 0u);                    /* e_phoff */
    w64(out + 40u, APKC_ELF64_EHDR_SZ);    /* e_shoff */
    w32(out + 48u, 0u);                    /* e_flags */
    w16(out + 52u, APKC_ELF64_EHDR_SZ);
    w16(out + 54u, 0u);                    /* e_phentsize */
    w16(out + 56u, 0u);                    /* e_phnum */
    w16(out + 58u, APKC_ELF64_SHDR_SZ);
    w16(out + 60u, 1u);                    /* null section only */
    w16(out + 62u, 0u);                    /* no shstrtab */

    /* Section header zero is required to be all-zero and was cleared above. */
    return APKC_ELF64_MIN_SZ;
}
