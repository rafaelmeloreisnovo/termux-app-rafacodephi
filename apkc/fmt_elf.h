/* fmt_elf.h — Minimal ELF shared library generator.
 * Produces a valid .so exporting ANativeActivity_onCreate + android_main.
 * Both symbols are implemented as a single RET (arm64) or BX LR (arm32).
 * Output is a self-contained ELF that Android's linker will accept.
 * No heap. No malloc. No libc. */
#pragma once
#include "arch_arm64.h"
#include "arch_arm32.h"

/* ── ELF constants ───────────────────────────────────────────────────────── */
#define ET_DYN       3u
#define EM_AARCH64   0xB7u
#define EM_ARM       0x28u
#define PT_LOAD      1u
#define PT_DYNAMIC   2u
#define PF_R         4u
#define PF_W         2u
#define PF_X         1u
#define SHT_NULL     0u
#define SHT_PROGBITS 1u
#define SHT_SYMTAB   2u
#define SHT_STRTAB   3u
#define SHT_HASH     5u
#define SHT_DYNAMIC  6u
#define SHT_DYNSYM   11u
#define SHF_ALLOC    0x2u
#define SHF_EXECINSTR 0x4u
#define SHF_WRITE    0x1u
#define STB_GLOBAL   1u
#define STT_FUNC     2u
#define STV_DEFAULT  0u
#define DT_NULL      0u
#define DT_NEEDED    1u
#define DT_HASH      4u
#define DT_STRTAB    5u
#define DT_SYMTAB    6u
#define DT_STRSZ    10u
#define DT_SYMENT   11u
#define SHN_UNDEF    0u

/* ── Helper: write ELF field sizes ────────────────────────────────────────── */
typedef struct { u8 *p; sz pos; } ELFBuf;
static inline void _eb8 (ELFBuf *b, u8  v) { b->p[b->pos++]=v; }
static inline void _eb16(ELFBuf *b, u16 v) { w16(b->p+b->pos,v); b->pos+=2; }
static inline void _eb32(ELFBuf *b, u32 v) { w32(b->p+b->pos,v); b->pos+=4; }
static inline void _eb64(ELFBuf *b, u64 v) { w64(b->p+b->pos,v); b->pos+=8; }
static inline void _epad(ELFBuf *b, sz n)  {
    for (sz i=0;i<n;i++) b->p[b->pos++]=0;
}
static inline void _epatch32(ELFBuf *b, sz off, u32 v){ w32(b->p+off,v); }
static inline void _epatch64(ELFBuf *b, sz off, u64 v){ w64(b->p+off,v); }

/* ── .dynstr content ────────────────────────────────────────────────────────── */
/* Layout: \0 ANativeActivity_onCreate\0 android_main\0 */
static const u8 _dynstr[] =
    "\x00"
    "ANativeActivity_onCreate\x00"   /* offset 1, len 24 */
    "android_main\x00"               /* offset 26, len 12 */
    "\x00";                          /* extra null, total 40 bytes */
#define DYNSTR_SZ 40u
#define SYM1_NAME 1u  /* ANativeActivity_onCreate */
#define SYM2_NAME 26u /* android_main */

/* ── ELF hash table (traditional, nbucket=1) ───────────────────────────── */
/* nbucket=1, nchain=3, bucket[0]=1, chain={0,2,0} — 24 bytes */
static const u8 _elfhash[] = {
    0x01,0x00,0x00,0x00,  /* nbucket = 1    */
    0x03,0x00,0x00,0x00,  /* nchain  = 3    */
    0x01,0x00,0x00,0x00,  /* bucket[0] = 1  */
    0x00,0x00,0x00,0x00,  /* chain[0]  = 0  (null sym) */
    0x02,0x00,0x00,0x00,  /* chain[1]  = 2  (ANativeActivity → android_main) */
    0x00,0x00,0x00,0x00,  /* chain[2]  = 0  (end) */
};
#define ELFHASH_SZ 24u

/* ── .shstrtab ────────────────────────────────────────═──────────────── */
static const u8 _shstrtab[] =
    "\x00"          /* 0  */
    ".text\x00"     /* 1  */
    ".hash\x00"     /* 7  */
    ".dynsym\x00"   /* 13 */
    ".dynstr\x00"   /* 21 */
    ".dynamic\x00"  /* 29 */
    ".shstrtab\x00";/* 38 */
#define SHSTRTAB_SZ 48u
/* Name offsets within .shstrtab */
#define SHN_TEXT   1u
#define SHN_HASH   7u
#define SHN_DYNSYM 13u
#define SHN_DYNSTR 21u
#define SHN_DYN    29u
#define SHN_SHSTR  38u

/* ─═════════════════════════════ ARM64 .so generator ══════════════════════════════ */
/*
 * Memory layout (ARM64):
 *  0x000  ELF header      64 B
 *  0x040  PHT             2 × 56 = 112 B
 *  0x0B0  .text           16 B  (RET + RET + 2×NOP)
 *  0x0C0  .hash           24 B
 *  0x0D8  .dynsym         3 × 24 = 72 B
 *  0x120  .dynstr         40 B
 *  0x148  .dynamic        6 × 16 = 96 B
 *  0x1A8  .shstrtab       48 B
 *  0x1D8  SHT             7 × 64 = 448 B
 *  Total  0x398  = 920 B
 */
#define A64SO_TEXT_OFF   0x0B0u
#define A64SO_HASH_OFF   0x0C0u
#define A64SO_DSYM_OFF   0x0D8u
#define A64SO_DSTR_OFF   0x120u
#define A64SO_DYN_OFF    0x148u
#define A64SO_SHSTR_OFF  0x1A8u
#define A64SO_SHT_OFF    0x1D8u
#define A64SO_TOTAL      0x398u
#define A64SO_NSYM       3u     /* null + 2 exported */
#define A64SO_NSECT      7u     /* null text hash dynsym dynstr dynamic shstrtab */
#define A64SO_NDYN       6u     /* DT entries */

static sz elf64_build_so(u8 *out, const u8 *text, u32 text_sz,
                          u32 sym1_va, u32 sym2_va)
{
    m_set(out, 0, (sz)A64SO_TOTAL);
    ELFBuf B; B.p = out; B.pos = 0;

    /* ── ELF Header (64 bytes) ── */
    /* e_ident */
    _eb8(&B,0x7F);_eb8(&B,'E');_eb8(&B,'L');_eb8(&B,'F');
    _eb8(&B,2);    /* EI_CLASS: ELFCLASS64 */
    _eb8(&B,1);    /* EI_DATA:  ELFDATA2LSB */
    _eb8(&B,1);    /* EI_VERSION */
    _eb8(&B,0);    /* EI_OSABI: ELFOSABI_NONE (Android uses 0) */
    _epad(&B,8);   /* EI_ABIVERSION + padding */
    _eb16(&B,(u16)ET_DYN);
    _eb16(&B,(u16)EM_AARCH64);
    _eb32(&B,1u);              /* e_version */
    _eb64(&B,0u);              /* e_entry: none */
    _eb64(&B,0x40u);           /* e_phoff: PHT after header */
    _eb64(&B,(u64)A64SO_SHT_OFF); /* e_shoff */
    _eb32(&B,0u);              /* e_flags */
    _eb16(&B,64u);             /* e_ehsize */
    _eb16(&B,56u);             /* e_phentsize */
    _eb16(&B,2u);              /* e_phnum */
    _eb16(&B,64u);             /* e_shentsize */
    _eb16(&B,(u16)A64SO_NSECT);/* e_shnum */
    _eb16(&B,6u);              /* e_shstrndx = index of .shstrtab = 6 */

    /* ── Program Header Table (2 × 56 bytes) ── */
    /* PT_LOAD: covers the whole file */
    _eb32(&B,(u32)PT_LOAD);
    _eb32(&B,(u32)(PF_R|PF_X));  /* p_flags */
    _eb64(&B,0u);                /* p_offset */
    _eb64(&B,0u);                /* p_vaddr */
    _eb64(&B,0u);                /* p_paddr */
    _eb64(&B,(u64)A64SO_TOTAL);  /* p_filesz */
    _eb64(&B,(u64)A64SO_TOTAL);  /* p_memsz */
    _eb64(&B,0x1000u);           /* p_align */
    /* PT_DYNAMIC */
    _eb32(&B,(u32)PT_DYNAMIC);
    _eb32(&B,(u32)(PF_R|PF_W));
    _eb64(&B,(u64)A64SO_DYN_OFF);
    _eb64(&B,(u64)A64SO_DYN_OFF);
    _eb64(&B,(u64)A64SO_DYN_OFF);
    _eb64(&B,(u64)(A64SO_NDYN*16u));
    _eb64(&B,(u64)(A64SO_NDYN*16u));
    _eb64(&B,8u);

    /* ── .text section (16 bytes) ── */
    B.pos = A64SO_TEXT_OFF;
    if (text && text_sz) {
        m_cpy(out+A64SO_TEXT_OFF, text, (sz)u32_min(text_sz,16u));
        B.pos = A64SO_TEXT_OFF + u32_min(text_sz,16u);
    } else {
        /* Default: both functions just return */
        w32(out+A64SO_TEXT_OFF+0,  A64_RET);  /* ANativeActivity_onCreate */
        w32(out+A64SO_TEXT_OFF+4,  A64_RET);  /* android_main             */
        w32(out+A64SO_TEXT_OFF+8,  A64_NOP);
        w32(out+A64SO_TEXT_OFF+12, A64_NOP);
    }
    sym1_va = sym1_va ? sym1_va : A64SO_TEXT_OFF;
    sym2_va = sym2_va ? sym2_va : A64SO_TEXT_OFF + 4u;

    /* ── .hash (24 bytes) ── */
    m_cpy(out+A64SO_HASH_OFF, _elfhash, ELFHASH_SZ);

    /* ── .dynsym (3 × 24 = 72 bytes) ── */
    u8 *ds = out + A64SO_DSYM_OFF;
    /* sym[0]: null */
    m_set(ds, 0, 24u);
    /* sym[1]: ANativeActivity_onCreate */
    w32(ds+24,  SYM1_NAME);                        /* st_name */
    ds[28] = (u8)((STB_GLOBAL<<4)|STT_FUNC);       /* st_info */
    ds[29] = STV_DEFAULT;                          /* st_other */
    w16(ds+30, 1u);                                /* st_shndx = .text = sect 1 */
    w64(ds+32, (u64)sym1_va);                      /* st_value */
    w64(ds+40, 4u);                                /* st_size */
    /* sym[2]: android_main */
    w32(ds+48, SYM2_NAME);
    ds[52] = (u8)((STB_GLOBAL<<4)|STT_FUNC);
    ds[53] = STV_DEFAULT;
    w16(ds+54, 1u);
    w64(ds+56, (u64)sym2_va);
    w64(ds+64, 4u);

    /* ── .dynstr (40 bytes) ── */
    m_cpy(out+A64SO_DSTR_OFF, _dynstr, DYNSTR_SZ);

    /* ── .dynamic (6 × 16 = 96 bytes) ── */
    u8 *dy = out + A64SO_DYN_OFF;
    /* DT_HASH */
    w64(dy,    (u64)DT_HASH);   w64(dy+8,  (u64)A64SO_HASH_OFF);  dy+=16;
    /* DT_STRTAB */
    w64(dy,    (u64)DT_STRTAB); w64(dy+8,  (u64)A64SO_DSTR_OFF);  dy+=16;
    /* DT_SYMTAB */
    w64(dy,    (u64)DT_SYMTAB); w64(dy+8,  (u64)A64SO_DSYM_OFF);  dy+=16;
    /* DT_STRSZ */
    w64(dy,    (u64)DT_STRSZ);  w64(dy+8,  (u64)DYNSTR_SZ);       dy+=16;
    /* DT_SYMENT */
    w64(dy,    (u64)DT_SYMENT); w64(dy+8,  24ULL);                 dy+=16;
    /* DT_NULL */
    w64(dy,    0ULL);           w64(dy+8,  0ULL);

    /* ── .shstrtab ── */
    m_cpy(out+A64SO_SHSTR_OFF, _shstrtab, SHSTRTAB_SZ);

    /* ── Section Header Table (7 × 64 bytes) ── */
    /* Macro: sh_name,type,flags,addr,off,sz,link,info,align,entsize */
#define SH64(nm,ty,fl,ad,of,sz,lk,in,al,es) do { \
    _eb32(&B,(nm)); _eb32(&B,(ty)); \
    _eb64(&B,(fl)); _eb64(&B,(ad)); \
    _eb64(&B,(of)); _eb64(&B,(sz)); \
    _eb32(&B,(lk)); _eb32(&B,(in)); \
    _eb64(&B,(al)); _eb64(&B,(es)); \
} while(0)
    B.pos = A64SO_SHT_OFF;
    SH64(0,SHT_NULL,0,0,0,0,0,0,0,0);                               /* 0: null */
    SH64(SHN_TEXT,  SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR,
         A64SO_TEXT_OFF, A64SO_TEXT_OFF, 16,    0,0,4,0);            /* 1: .text */
    SH64(SHN_HASH,  SHT_HASH,     SHF_ALLOC,
         A64SO_HASH_OFF, A64SO_HASH_OFF, ELFHASH_SZ, 3,0,4,4);      /* 2: .hash */
    SH64(SHN_DYNSYM,SHT_DYNSYM,  SHF_ALLOC,
         A64SO_DSYM_OFF, A64SO_DSYM_OFF, A64SO_NSYM*24, 4,1,8,24);  /* 3: .dynsym */
    SH64(SHN_DYNSTR,SHT_STRTAB,  SHF_ALLOC,
         A64SO_DSTR_OFF, A64SO_DSTR_OFF, DYNSTR_SZ, 0,0,1,0);       /* 4: .dynstr */
    SH64(SHN_DYN,   SHT_DYNAMIC, SHF_ALLOC|SHF_WRITE,
         A64SO_DYN_OFF,  A64SO_DYN_OFF,  A64SO_NDYN*16,4,0,8,16);   /* 5: .dynamic */
    SH64(SHN_SHSTR, SHT_STRTAB,  0,
         0, A64SO_SHSTR_OFF, SHSTRTAB_SZ, 0,0,1,0);                  /* 6: .shstrtab */
#undef SH64
    return A64SO_TOTAL;
}

/* ─═════════════════════════════ ARM32 .so generator ══════════════════════════════ */
/*
 * Memory layout (ARM32):
 *  0x000  ELF header     52 B
 *  0x034  PHT            2 × 32 = 64 B
 *  0x074  .text          16 B
 *  0x084  .hash          24 B
 *  0x09C  .dynsym        3 × 16 = 48 B
 *  0x0CC  .dynstr        40 B
 *  0x0F4  .dynamic       6 × 8  = 48 B
 *  0x124  .shstrtab      48 B
 *  0x154  SHT            7 × 40 = 280 B
 *  Total  0x26C = 620 B
 */
#define A32SO_TEXT_OFF   0x074u
#define A32SO_HASH_OFF   0x084u
#define A32SO_DSYM_OFF   0x09Cu
#define A32SO_DSTR_OFF   0x0CCu
#define A32SO_DYN_OFF    0x0F4u
#define A32SO_SHSTR_OFF  0x124u
#define A32SO_SHT_OFF    0x154u
#define A32SO_TOTAL      0x26Cu
#define A32SO_NSYM       3u
#define A32SO_NSECT      7u
#define A32SO_NDYN       6u

static sz elf32_build_so(u8 *out, const u8 *text, u32 text_sz,
                          u32 sym1_va, u32 sym2_va)
{
    m_set(out, 0, (sz)A32SO_TOTAL);
    ELFBuf B; B.p = out; B.pos = 0;

    /* ELF header (52 bytes) */
    _eb8(&B,0x7F);_eb8(&B,'E');_eb8(&B,'L');_eb8(&B,'F');
    _eb8(&B,1);    /* ELFCLASS32 */
    _eb8(&B,1);    /* ELFDATA2LSB */
    _eb8(&B,1);    /* version */
    _eb8(&B,0);    /* OSABI */
    _epad(&B,8);
    _eb16(&B,(u16)ET_DYN);
    _eb16(&B,(u16)EM_ARM);
    _eb32(&B,1u);
    _eb32(&B,0u);              /* e_entry */
    _eb32(&B,0x34u);           /* e_phoff */
    _eb32(&B,A32SO_SHT_OFF);   /* e_shoff */
    _eb32(&B,0x05000200u);     /* e_flags: EABI v5, uses THUMB */
    _eb16(&B,52u);             /* e_ehsize */
    _eb16(&B,32u);             /* e_phentsize */
    _eb16(&B,2u);              /* e_phnum */
    _eb16(&B,40u);             /* e_shentsize */
    _eb16(&B,(u16)A32SO_NSECT);
    _eb16(&B,6u);              /* e_shstrndx */

    /* PHT (2 × 32) */
    /* PT_LOAD */
    _eb32(&B,(u32)PT_LOAD);  _eb32(&B,0u);
    _eb32(&B,(u32)(PF_R|PF_X)); _eb32(&B,0u);
    _eb32(&B,(u32)A32SO_TOTAL); _eb32(&B,(u32)A32SO_TOTAL);
    _eb32(&B,0x1000u);          _eb32(&B,0u); /* align, dummy */
    /* rewrite properly: p_type,p_offset,p_vaddr,p_paddr,p_filesz,p_memsz,p_flags,p_align */
    B.pos = 0x34u;
    _eb32(&B,(u32)PT_LOAD);
    _eb32(&B,0u);              /* p_offset */
    _eb32(&B,0u);              /* p_vaddr */
    _eb32(&B,0u);              /* p_paddr */
    _eb32(&B,(u32)A32SO_TOTAL);
    _eb32(&B,(u32)A32SO_TOTAL);
    _eb32(&B,(u32)(PF_R|PF_X));
    _eb32(&B,0x1000u);
    /* PT_DYNAMIC */
    _eb32(&B,(u32)PT_DYNAMIC);
    _eb32(&B,(u32)A32SO_DYN_OFF);
    _eb32(&B,(u32)A32SO_DYN_OFF);
    _eb32(&B,(u32)A32SO_DYN_OFF);
    _eb32(&B,(u32)(A32SO_NDYN*8u));
    _eb32(&B,(u32)(A32SO_NDYN*8u));
    _eb32(&B,(u32)(PF_R|PF_W));
    _eb32(&B,8u);

    /* .text */
    sym1_va = sym1_va ? sym1_va : A32SO_TEXT_OFF;
    sym2_va = sym2_va ? sym2_va : A32SO_TEXT_OFF + 4u;
    if (text && text_sz) {
        m_cpy(out+A32SO_TEXT_OFF, text, (sz)u32_min(text_sz,16u));
    } else {
        w32(out+A32SO_TEXT_OFF+0,  A32_BXLR); /* ANativeActivity_onCreate */
        w32(out+A32SO_TEXT_OFF+4,  A32_BXLR); /* android_main             */
        w32(out+A32SO_TEXT_OFF+8,  A32_NOP);
        w32(out+A32SO_TEXT_OFF+12, A32_NOP);
    }

    /* .hash */
    m_cpy(out+A32SO_HASH_OFF, _elfhash, ELFHASH_SZ);

    /* .dynsym (3 × 16 = 48 bytes) */
    u8 *ds = out + A32SO_DSYM_OFF;
    /* Elf32_Sym: st_name(4) st_value(4) st_size(4) st_info(1) st_other(1) st_shndx(2) */
    m_set(ds, 0, 16u);
    w32(ds+16, SYM1_NAME); w32(ds+20,(u32)sym1_va); w32(ds+24,4u);
    ds[28]=(u8)((STB_GLOBAL<<4)|STT_FUNC); ds[29]=STV_DEFAULT; w16(ds+30,1u);
    w32(ds+32, SYM2_NAME); w32(ds+36,(u32)sym2_va); w32(ds+40,4u);
    ds[44]=(u8)((STB_GLOBAL<<4)|STT_FUNC); ds[45]=STV_DEFAULT; w16(ds+46,1u);

    /* .dynstr */
    m_cpy(out+A32SO_DSTR_OFF, _dynstr, DYNSTR_SZ);

    /* .dynamic (6 × 8 = 48 bytes) */
    u8 *dy = out + A32SO_DYN_OFF;
    w32(dy,DT_HASH);   w32(dy+4, A32SO_HASH_OFF); dy+=8;
    w32(dy,DT_STRTAB); w32(dy+4, A32SO_DSTR_OFF); dy+=8;
    w32(dy,DT_SYMTAB); w32(dy+4, A32SO_DSYM_OFF); dy+=8;
    w32(dy,DT_STRSZ);  w32(dy+4, DYNSTR_SZ);       dy+=8;
    w32(dy,DT_SYMENT); w32(dy+4, 16u);             dy+=8;
    w32(dy,DT_NULL);   w32(dy+4, 0u);

    /* .shstrtab */
    m_cpy(out+A32SO_SHSTR_OFF, _shstrtab, SHSTRTAB_SZ);

    /* SHT (7 × 40) */
#define SH32(nm,ty,fl,ad,of,sz,lk,in,al,es) do { \
    _eb32(&B,(nm));_eb32(&B,(ty));_eb32(&B,(fl));_eb32(&B,(ad)); \
    _eb32(&B,(of));_eb32(&B,(sz));_eb32(&B,(lk));_eb32(&B,(in)); \
    _eb32(&B,(al));_eb32(&B,(es)); \
} while(0)
    B.pos = A32SO_SHT_OFF;
    SH32(0,SHT_NULL,0,0,0,0,0,0,0,0);
    SH32(SHN_TEXT,  SHT_PROGBITS,SHF_ALLOC|SHF_EXECINSTR,
         A32SO_TEXT_OFF,A32SO_TEXT_OFF,16,0,0,4,0);
    SH32(SHN_HASH,  SHT_HASH,    SHF_ALLOC,
         A32SO_HASH_OFF,A32SO_HASH_OFF,ELFHASH_SZ,3,0,4,4);
    SH32(SHN_DYNSYM,SHT_DYNSYM, SHF_ALLOC,
         A32SO_DSYM_OFF,A32SO_DSYM_OFF,A32SO_NSYM*16,4,1,4,16);
    SH32(SHN_DYNSTR,SHT_STRTAB, SHF_ALLOC,
         A32SO_DSTR_OFF,A32SO_DSTR_OFF,DYNSTR_SZ,0,0,1,0);
    SH32(SHN_DYN,   SHT_DYNAMIC,SHF_ALLOC|SHF_WRITE,
         A32SO_DYN_OFF,A32SO_DYN_OFF,A32SO_NDYN*8,4,0,4,8);
    SH32(SHN_SHSTR, SHT_STRTAB, 0,
         0,A32SO_SHSTR_OFF,SHSTRTAB_SZ,0,0,1,0);
#undef SH32
    return A32SO_TOTAL;
}
