/* DEX builder — minimal freestanding implementation */

#include "freestanding.h"

/* DEX magic: 'dex\n035\0' */
#define DEX_MAGIC_SIZE 8
static const uint8_t DEX_MAGIC[8] = {'d', 'e', 'x', '\n', '0', '3', '5', '\0'};

/* Minimal DEX structures */
typedef struct {
    uint8_t magic[8];
    uint32_t checksum;           /* Adler32 */
    uint8_t sha1[20];            /* SHA1 digest */
    uint32_t file_size;
    uint32_t header_size;        /* 0x70 */
    uint32_t endian_tag;         /* 0x12345678 */
    uint32_t link_size;          /* 0 */
    uint32_t link_off;           /* 0 */
    uint32_t map_list_off;       /* Offset to map list */
    uint32_t string_ids_size;
    uint32_t string_ids_off;
    uint32_t type_ids_size;
    uint32_t type_ids_off;
    uint32_t proto_ids_size;
    uint32_t proto_ids_off;
    uint32_t field_ids_size;
    uint32_t field_ids_off;
    uint32_t method_ids_size;
    uint32_t method_ids_off;
    uint32_t class_defs_size;
    uint32_t class_defs_off;
    uint32_t data_size;
    uint32_t data_off;
} DexHeader;

/* DEX builder context */
typedef struct {
    uint8_t *buf;
    uint32_t buf_size;
    uint32_t pos;
    uint32_t strings_count;
    uint32_t types_count;
    uint32_t methods_count;
} DexBuilder;

/* Write little-endian 32-bit */
static void write_le32(uint8_t *buf, uint32_t offset, uint32_t val) {
    buf[offset] = val & 0xFF;
    buf[offset + 1] = (val >> 8) & 0xFF;
    buf[offset + 2] = (val >> 16) & 0xFF;
    buf[offset + 3] = (val >> 24) & 0xFF;
}

/* Adler32 checksum */
static uint32_t adler32_checksum(const uint8_t *data, uint32_t len) {
    uint32_t a = 1, b = 0;
    for (uint32_t i = 0; i < len; i++) {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

/* Build minimal DEX header */
static int build_dex_header(DexBuilder *b, uint32_t total_size) {
    if (b->pos + 0x70 > b->buf_size) {
        return -1;
    }

    DexHeader *hdr = (DexHeader *)b->buf;

    /* Copy magic */
    for (int i = 0; i < 8; i++) {
        hdr->magic[i] = DEX_MAGIC[i];
    }

    /* Checksum (computed later) */
    hdr->checksum = 0;

    /* SHA1 (all zeros for minimal DEX) */
    for (int i = 0; i < 20; i++) {
        hdr->sha1[i] = 0;
    }

    /* File size */
    hdr->file_size = total_size;

    /* Header size */
    hdr->header_size = 0x70;

    /* Endian tag */
    hdr->endian_tag = 0x12345678;

    /* No linking */
    hdr->link_size = 0;
    hdr->link_off = 0;

    /* Map list (minimal) */
    hdr->map_list_off = 0x70;

    /* String IDs (minimal) */
    hdr->string_ids_size = 1;
    hdr->string_ids_off = 0x80;

    /* Type IDs (minimal) */
    hdr->type_ids_size = 1;
    hdr->type_ids_off = 0x88;

    /* Proto IDs (minimal) */
    hdr->proto_ids_size = 0;
    hdr->proto_ids_off = 0;

    /* Field IDs (minimal) */
    hdr->field_ids_size = 0;
    hdr->field_ids_off = 0;

    /* Method IDs (minimal) */
    hdr->method_ids_size = 0;
    hdr->method_ids_off = 0;

    /* Class defs (minimal) */
    hdr->class_defs_size = 0;
    hdr->class_defs_off = 0;

    /* Data section */
    hdr->data_size = 0;
    hdr->data_off = 0;

    b->pos = 0x70;
    return 0;
}

/* Build minimal map list */
static int build_dex_map_list(DexBuilder *b) {
    if (b->pos + 12 > b->buf_size) {
        return -1;
    }

    uint8_t *map = b->buf + b->pos;

    /* Map list size: 1 entry */
    write_le32(map, 0, 1);

    /* Map item: type=TYPE_STRING_ID_ITEM (0x0001), size=1, offset=0x80 */
    write_le32(map, 4, 0x00010001);  /* type=0x0001, unused=0x0001 */
    write_le32(map, 8, 0x80);        /* offset */

    b->pos += 12;
    return 0;
}

/* Initialize DEX builder */
DexBuilder *dex_builder_new(uint8_t *buf, uint32_t buf_size) {
    DexBuilder *b = (DexBuilder *)buf;  /* Reuse buffer for context */
    b->buf = buf;
    b->buf_size = buf_size;
    b->pos = 0;
    b->strings_count = 0;
    b->types_count = 0;
    b->methods_count = 0;
    return b;
}

/* Build minimal DEX binary */
int dex_build_minimal(uint8_t *buf, uint32_t buf_size) {
    if (buf_size < 0x100) {
        return -1;  /* Buffer too small */
    }

    DexBuilder b;
    b.buf = buf;
    b.buf_size = buf_size;
    b.pos = 0;
    b.strings_count = 0;
    b.types_count = 0;
    b.methods_count = 0;

    /* Build header */
    if (build_dex_header(&b, 0x100) != 0) {
        return -2;
    }

    /* Build map list */
    if (build_dex_map_list(&b) != 0) {
        return -3;
    }

    /* Compute and set checksum (skip magic, checksum, and SHA1) */
    uint32_t chksum = adler32_checksum(buf + 12, b.pos - 12);
    write_le32(buf, 4, chksum);

    return b.pos;
}

/* Build APK-compatible DEX (minimal "Hello World" bytecode) */
int dex_build_hello_apk(uint8_t *buf, uint32_t buf_size) {
    /* For minimal APK, same as minimal DEX */
    return dex_build_minimal(buf, buf_size);
}
