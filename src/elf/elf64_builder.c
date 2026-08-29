/* ELF64 builder — freestanding, no libc */

#include "freestanding.h"

/* ELF64 header structure */
typedef struct {
    uint8_t e_ident[16];      /* Magic: 0x7f 'E' 'L' 'F' */
    uint16_t e_type;          /* ET_EXEC=2, ET_DYN=3 */
    uint16_t e_machine;       /* EM_AARCH64=183 */
    uint32_t e_version;       /* 1 */
    uint64_t e_entry;         /* Entry point */
    uint64_t e_phoff;         /* Program header offset */
    uint64_t e_shoff;         /* Section header offset */
    uint32_t e_flags;         /* Flags */
    uint16_t e_ehsize;        /* ELF header size (64) */
    uint16_t e_phentsize;     /* Program header entry size (56) */
    uint16_t e_phnum;         /* Number of program headers */
    uint16_t e_shentsize;     /* Section header entry size (64) */
    uint16_t e_shnum;         /* Number of section headers */
    uint16_t e_shstrndx;      /* Section string table index */
} Elf64_Ehdr;

/* Program header structure */
typedef struct {
    uint32_t p_type;          /* PT_LOAD=1, PT_DYNAMIC=3 */
    uint32_t p_flags;         /* PF_X=1, PF_W=2, PF_R=4 */
    uint64_t p_offset;        /* File offset */
    uint64_t p_vaddr;         /* Virtual address */
    uint64_t p_paddr;         /* Physical address */
    uint64_t p_filesz;        /* File size */
    uint64_t p_memsz;         /* Memory size */
    uint64_t p_align;         /* Alignment */
} Elf64_Phdr;

/* Section header structure */
typedef struct {
    uint32_t sh_name;         /* Name offset in string table */
    uint32_t sh_type;         /* SHT_PROGBITS=1, SHT_SYMTAB=2, etc */
    uint64_t sh_flags;        /* SHF_WRITE=1, SHF_ALLOC=2, SHF_EXECINSTR=4 */
    uint64_t sh_addr;         /* Virtual address */
    uint64_t sh_offset;       /* File offset */
    uint64_t sh_size;         /* Section size */
    uint32_t sh_link;         /* Link to another section */
    uint32_t sh_info;         /* Additional info */
    uint64_t sh_addralign;    /* Alignment */
    uint64_t sh_entsize;      /* Entry size */
} Elf64_Shdr;

/* ELF64 builder context */
typedef struct {
    uint8_t *buf;             /* Output buffer */
    uint32_t buf_size;        /* Buffer size */
    uint32_t pos;             /* Current position */
    uint64_t text_base;       /* .text base address */
    uint64_t data_base;       /* .data base address */
    uint32_t text_size;       /* .text size */
    uint32_t data_size;       /* .data size */
} Elf64Builder;

/* Write bytes to builder buffer */
static void write_bytes(Elf64Builder *b, const uint8_t *data, uint32_t len) {
    if (b->pos + len > b->buf_size) return;  /* Overflow check */
    for (uint32_t i = 0; i < len; i++) {
        b->buf[b->pos + i] = data[i];
    }
    b->pos += len;
}

/* Write little-endian 16-bit value */
static void write_le16(Elf64Builder *b, uint16_t val) {
    uint8_t bytes[2];
    bytes[0] = val & 0xFF;
    bytes[1] = (val >> 8) & 0xFF;
    write_bytes(b, bytes, 2);
}

/* Write little-endian 32-bit value */
static void write_le32(Elf64Builder *b, uint32_t val) {
    uint8_t bytes[4];
    bytes[0] = val & 0xFF;
    bytes[1] = (val >> 8) & 0xFF;
    bytes[2] = (val >> 16) & 0xFF;
    bytes[3] = (val >> 24) & 0xFF;
    write_bytes(b, bytes, 4);
}

/* Write little-endian 64-bit value */
static void write_le64(Elf64Builder *b, uint64_t val) {
    uint8_t bytes[8];
    bytes[0] = val & 0xFF;
    bytes[1] = (val >> 8) & 0xFF;
    bytes[2] = (val >> 16) & 0xFF;
    bytes[3] = (val >> 24) & 0xFF;
    bytes[4] = (val >> 32) & 0xFF;
    bytes[5] = (val >> 40) & 0xFF;
    bytes[6] = (val >> 48) & 0xFF;
    bytes[7] = (val >> 56) & 0xFF;
    write_bytes(b, bytes, 8);
}

/* Build ELF64 header */
static void build_elf64_header(Elf64Builder *b) {
    /* ELF magic */
    uint8_t e_ident[16];
    e_ident[0] = 0x7f;
    e_ident[1] = 'E';
    e_ident[2] = 'L';
    e_ident[3] = 'F';
    e_ident[4] = 2;      /* EI_CLASS: 64-bit */
    e_ident[5] = 1;      /* EI_DATA: little-endian */
    e_ident[6] = 1;      /* EI_VERSION: current */
    e_ident[7] = 0;      /* EI_OSABI: UNIX System V */
    for (int i = 8; i < 16; i++) {
        e_ident[i] = 0;
    }

    write_bytes(b, e_ident, 16);

    /* e_type: ET_EXEC=2 */
    write_le16(b, 2);

    /* e_machine: EM_AARCH64=183 */
    write_le16(b, 183);

    /* e_version: 1 */
    write_le32(b, 1);

    /* e_entry: entry point at 0x400000 */
    write_le64(b, 0x400000);

    /* e_phoff: program header offset (after ELF header) */
    write_le64(b, 64);

    /* e_shoff: section header offset (after program headers) */
    write_le64(b, 64 + 56 * 2);  /* 2 program headers */

    /* e_flags: ARM64 flags */
    write_le32(b, 0);

    /* e_ehsize: ELF header size */
    write_le16(b, 64);

    /* e_phentsize: program header entry size */
    write_le16(b, 56);

    /* e_phnum: number of program headers */
    write_le16(b, 2);

    /* e_shentsize: section header entry size */
    write_le16(b, 64);

    /* e_shnum: number of sections */
    write_le16(b, 3);  /* .text, .data, .shstrtab */

    /* e_shstrndx: section header string table index */
    write_le16(b, 2);
}

/* Build program headers (PT_LOAD for .text, .data) */
static void build_program_headers(Elf64Builder *b) {
    /* PT_LOAD: .text segment */
    write_le32(b, 1);        /* p_type = PT_LOAD */
    write_le32(b, 5);        /* p_flags = PF_R | PF_X */
    write_le64(b, 0x1000);   /* p_offset (file offset to .text) */
    write_le64(b, b->text_base);  /* p_vaddr */
    write_le64(b, b->text_base);  /* p_paddr */
    write_le64(b, b->text_size);  /* p_filesz */
    write_le64(b, b->text_size);  /* p_memsz */
    write_le64(b, 0x1000);   /* p_align */

    /* PT_LOAD: .data segment */
    write_le32(b, 1);        /* p_type = PT_LOAD */
    write_le32(b, 6);        /* p_flags = PF_R | PF_W */
    write_le64(b, 0x2000);   /* p_offset (file offset to .data) */
    write_le64(b, b->data_base);  /* p_vaddr */
    write_le64(b, b->data_base);  /* p_paddr */
    write_le64(b, b->data_size);  /* p_filesz */
    write_le64(b, b->data_size);  /* p_memsz */
    write_le64(b, 0x1000);   /* p_align */
}

/* Initialize ELF64 builder */
Elf64Builder *elf64_builder_new(uint8_t *buf, uint32_t buf_size) {
    Elf64Builder *b = (Elf64Builder *)buf;
    b->buf = buf;
    b->buf_size = buf_size;
    b->pos = 0;
    b->text_base = 0x400000;
    b->data_base = 0x600000;
    b->text_size = 0;
    b->data_size = 0;
    return b;
}

/* Build complete ELF64 binary */
int elf64_build(Elf64Builder *b, const uint8_t *text_data, uint32_t text_len,
                const uint8_t *data_data, uint32_t data_len) {
    if (!b || !text_data || !data_data) {
        return -1;
    }

    b->text_size = text_len;
    b->data_size = data_len;

    /* Build ELF header */
    build_elf64_header(b);

    /* Build program headers */
    build_program_headers(b);

    /* Padding to .text section (align to 0x1000) */
    while (b->pos < 0x1000) {
        uint8_t pad = 0;
        write_bytes(b, &pad, 1);
    }

    /* Write .text section */
    write_bytes(b, text_data, text_len);

    /* Padding to .data section (align to 0x1000) */
    while (b->pos < 0x2000) {
        uint8_t pad = 0;
        write_bytes(b, &pad, 1);
    }

    /* Write .data section */
    write_bytes(b, data_data, data_len);

    return b->pos;  /* Return final size */
}

/* Quick test: build minimal executable */
int elf64_build_minimal_arm64(uint8_t *buf, uint32_t buf_size) {
    /* Minimal ARM64 .text: mov x0, #0; svc #0 (exit) */
    uint8_t text[] = {
        0x00, 0x00, 0x80, 0xd2,  /* mov x0, #0 */
        0x01, 0x00, 0x00, 0xd4   /* svc #0 */
    };

    uint8_t data[] = { 0 };  /* Empty .data */

    Elf64Builder *b = elf64_builder_new(buf, buf_size);
    return elf64_build(b, text, sizeof(text), data, sizeof(data));
}
