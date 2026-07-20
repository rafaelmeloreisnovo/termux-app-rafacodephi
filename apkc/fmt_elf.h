/* fmt_elf.h — Bounded structural and executable ELF emitters for APKC.
 *
 * Structural outputs:
 *   - ELF32 / EM_ARM / EABI5 / ET_REL / null section only
 *   - ELF64 / EM_AARCH64 / ET_REL / null section only
 *
 * Executable-stub outputs:
 *   - ELF32 / EM_ARM / ET_EXEC / one RX PT_LOAD / exit(0)
 *   - ELF64 / EM_AARCH64 / ET_EXEC / one RX PT_LOAD / exit(0)
 *
 * The executable stubs contain fixed architecture-specific Linux/Android
 * syscall instructions. They have no section table, symbols, relocations,
 * dynamic linking or user payload. Structural validation does not establish
 * execution on a physical Android device.
 */
#pragma once
#include "mem.h"

#define APKC_ELFCLASS32       1u
#define APKC_ELFCLASS64       2u
#define APKC_ELFDATA2LSB      1u
#define APKC_EV_CURRENT       1u
#define APKC_ET_REL           1u
#define APKC_ET_EXEC          2u
#define APKC_EM_ARM          40u
#define APKC_EM_AARCH64     183u
#define APKC_EF_ARM_EABI5 0x05000000u

#define APKC_PT_LOAD          1u
#define APKC_PF_X             1u
#define APKC_PF_R             4u
#define APKC_PAGE_ALIGN  0x1000u

#define APKC_ELF32_EHDR_SZ 52u
#define APKC_ELF32_PHDR_SZ 32u
#define APKC_ELF32_SHDR_SZ 40u
#define APKC_ELF32_REL_SZ  (APKC_ELF32_EHDR_SZ + APKC_ELF32_SHDR_SZ)

#define APKC_ELF64_EHDR_SZ 64u
#define APKC_ELF64_PHDR_SZ 56u
#define APKC_ELF64_SHDR_SZ 64u
#define APKC_ELF64_REL_SZ  (APKC_ELF64_EHDR_SZ + APKC_ELF64_SHDR_SZ)

#define APKC_ELF_EXEC_CODE_OFF 0x100u
#define APKC_ELF_EXEC_CODE_SZ  12u
#define APKC_ELF_EXEC_SZ       (APKC_ELF_EXEC_CODE_OFF + APKC_ELF_EXEC_CODE_SZ)
#define APKC_ELF32_EXEC_BASE   0x00010000u
#define APKC_ELF64_EXEC_BASE   0x0000000000400000ULL

/* Backward-compatible names used by existing structural tests. */
#define APKC_ELF32_MIN_SZ APKC_ELF32_REL_SZ
#define APKC_ELF64_MIN_SZ APKC_ELF64_REL_SZ

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
    if (!out || cap < (sz)APKC_ELF32_REL_SZ) return 0u;

    m_set(out, 0u, (sz)APKC_ELF32_REL_SZ);
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

    return APKC_ELF32_REL_SZ;
}

static inline u32 apkc_elf64_aarch64_build_checked(u8 *out, sz cap) {
    if (!out || cap < (sz)APKC_ELF64_REL_SZ) return 0u;

    m_set(out, 0u, (sz)APKC_ELF64_REL_SZ);
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

    return APKC_ELF64_REL_SZ;
}

static inline u32 apkc_elf32_arm_exec_build_checked(u8 *out, sz cap) {
    if (!out || cap < (sz)APKC_ELF_EXEC_SZ) return 0u;

    m_set(out, 0u, (sz)APKC_ELF_EXEC_SZ);
    apkc_elf_ident(out, APKC_ELFCLASS32);

    w16(out + 16u, APKC_ET_EXEC);
    w16(out + 18u, APKC_EM_ARM);
    w32(out + 20u, APKC_EV_CURRENT);
    w32(out + 24u, APKC_ELF32_EXEC_BASE + APKC_ELF_EXEC_CODE_OFF);
    w32(out + 28u, APKC_ELF32_EHDR_SZ);    /* e_phoff */
    w32(out + 32u, 0u);                    /* no section table */
    w32(out + 36u, APKC_EF_ARM_EABI5);
    w16(out + 40u, APKC_ELF32_EHDR_SZ);
    w16(out + 42u, APKC_ELF32_PHDR_SZ);
    w16(out + 44u, 1u);
    w16(out + 46u, 0u);
    w16(out + 48u, 0u);
    w16(out + 50u, 0u);

    /* Elf32_Phdr at e_phoff. */
    u8 *ph = out + APKC_ELF32_EHDR_SZ;
    w32(ph + 0u, APKC_PT_LOAD);
    w32(ph + 4u, 0u);
    w32(ph + 8u, APKC_ELF32_EXEC_BASE);
    w32(ph + 12u, APKC_ELF32_EXEC_BASE);
    w32(ph + 16u, APKC_ELF_EXEC_SZ);
    w32(ph + 20u, APKC_ELF_EXEC_SZ);
    w32(ph + 24u, APKC_PF_R | APKC_PF_X);
    w32(ph + 28u, APKC_PAGE_ALIGN);

    /* ARM EABI: mov r7,#1 ; mov r0,#0 ; svc #0 => exit(0). */
    u8 *code = out + APKC_ELF_EXEC_CODE_OFF;
    w32(code + 0u, 0xE3A07001u);
    w32(code + 4u, 0xE3A00000u);
    w32(code + 8u, 0xEF000000u);

    return APKC_ELF_EXEC_SZ;
}

static inline u32 apkc_elf64_aarch64_exec_build_checked(u8 *out, sz cap) {
    if (!out || cap < (sz)APKC_ELF_EXEC_SZ) return 0u;

    m_set(out, 0u, (sz)APKC_ELF_EXEC_SZ);
    apkc_elf_ident(out, APKC_ELFCLASS64);

    w16(out + 16u, APKC_ET_EXEC);
    w16(out + 18u, APKC_EM_AARCH64);
    w32(out + 20u, APKC_EV_CURRENT);
    w64(out + 24u, APKC_ELF64_EXEC_BASE + APKC_ELF_EXEC_CODE_OFF);
    w64(out + 32u, APKC_ELF64_EHDR_SZ);    /* e_phoff */
    w64(out + 40u, 0u);                    /* no section table */
    w32(out + 48u, 0u);
    w16(out + 52u, APKC_ELF64_EHDR_SZ);
    w16(out + 54u, APKC_ELF64_PHDR_SZ);
    w16(out + 56u, 1u);
    w16(out + 58u, 0u);
    w16(out + 60u, 0u);
    w16(out + 62u, 0u);

    /* Elf64_Phdr at e_phoff. */
    u8 *ph = out + APKC_ELF64_EHDR_SZ;
    w32(ph + 0u, APKC_PT_LOAD);
    w32(ph + 4u, APKC_PF_R | APKC_PF_X);
    w64(ph + 8u, 0u);
    w64(ph + 16u, APKC_ELF64_EXEC_BASE);
    w64(ph + 24u, APKC_ELF64_EXEC_BASE);
    w64(ph + 32u, APKC_ELF_EXEC_SZ);
    w64(ph + 40u, APKC_ELF_EXEC_SZ);
    w64(ph + 48u, APKC_PAGE_ALIGN);

    /* AArch64: mov x8,#93 ; mov x0,#0 ; svc #0 => exit(0). */
    u8 *code = out + APKC_ELF_EXEC_CODE_OFF;
    w32(code + 0u, 0xD2800BA8u);
    w32(code + 4u, 0xD2800000u);
    w32(code + 8u, 0xD4000001u);

    return APKC_ELF_EXEC_SZ;
}
