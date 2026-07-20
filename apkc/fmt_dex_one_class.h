/* fmt_dex_one_class.h — One-class DEX 035 fixture emitter.
 *
 * Emits Lraf/apkc/Stub; with public static run()V containing return-void.
 * Reuses SHA-1 and Adler-32 from fmt_dex.h. No heap, malloc or libc.
 * This is a bounded table/code emitter, not a Java/Kotlin compiler.
 */
#pragma once
#include "fmt_dex.h"

#define DEX_ONE_CLASS_SZ             0x188u
#define DEX_ONE_STRING_IDS_OFF       0x070u
#define DEX_ONE_TYPE_IDS_OFF         0x080u
#define DEX_ONE_PROTO_IDS_OFF        0x08Cu
#define DEX_ONE_METHOD_IDS_OFF       0x098u
#define DEX_ONE_CLASS_DEFS_OFF       0x0A0u
#define DEX_ONE_DATA_OFF             0x0C0u
#define DEX_ONE_STRING_OBJECT_OFF    0x0C0u
#define DEX_ONE_STRING_CLASS_OFF     0x0D4u
#define DEX_ONE_STRING_VOID_OFF      0x0E5u
#define DEX_ONE_STRING_RUN_OFF       0x0E8u
#define DEX_ONE_CLASS_DATA_OFF       0x0EDu
#define DEX_ONE_CODE_OFF             0x0F8u
#define DEX_ONE_MAP_OFF              0x10Cu
#define DEX_ONE_DATA_SZ              (DEX_ONE_CLASS_SZ - DEX_ONE_DATA_OFF)
#define DEX_ONE_MAP_ITEM_COUNT       10u

#define DEX_ONE_NO_INDEX             0xFFFFFFFFu
#define DEX_ONE_ACC_PUBLIC           0x0001u
#define DEX_ONE_ACC_STATIC           0x0008u
#define DEX_ONE_OP_RETURN_VOID       0x000Eu

#define DEX_ONE_TYPE_STRING_ID       0x0001u
#define DEX_ONE_TYPE_TYPE_ID         0x0002u
#define DEX_ONE_TYPE_PROTO_ID        0x0003u
#define DEX_ONE_TYPE_METHOD_ID       0x0005u
#define DEX_ONE_TYPE_CLASS_DEF       0x0006u
#define DEX_ONE_TYPE_CLASS_DATA      0x2000u
#define DEX_ONE_TYPE_CODE_ITEM       0x2001u
#define DEX_ONE_TYPE_STRING_DATA     0x2002u

static inline void dex_one_map_item(u8 *p, u16 type, u32 size, u32 offset) {
    w16(p + 0u, type);
    w16(p + 2u, 0u);
    w32(p + 4u, size);
    w32(p + 8u, offset);
}

static inline void dex_one_ascii_string(u8 *out, u32 offset, const char *text, u8 length) {
    out[offset] = length;
    m_cpy(out + offset + 1u, text, (sz)length);
    out[offset + 1u + length] = 0u;
}

static inline void dex_one_finish(u8 *out) {
    SHA1Ctx sc;
    sha1_init(&sc);
    sha1_update(&sc, out + 32u, (sz)(DEX_ONE_CLASS_SZ - 32u));
    sha1_final(&sc, out + 12u);
    w32(out + 8u, adler32(out + 12u, (sz)(DEX_ONE_CLASS_SZ - 12u)));
}

static inline u32 dex_build_one_class_checked(u8 *out, sz cap) {
    if (!out || cap < (sz)DEX_ONE_CLASS_SZ) return 0u;

    m_set(out, 0u, (sz)DEX_ONE_CLASS_SZ);
    out[0] = 'd'; out[1] = 'e'; out[2] = 'x'; out[3] = '\n';
    out[4] = '0'; out[5] = '3'; out[6] = '5'; out[7] = '\0';

    /* Header. */
    w32(out + 32u, DEX_ONE_CLASS_SZ);
    w32(out + 36u, DEX_HEADER_SZ);
    w32(out + 40u, DEX_ENDIAN_TAG);
    w32(out + 52u, DEX_ONE_MAP_OFF);
    w32(out + 56u, 4u); w32(out + 60u, DEX_ONE_STRING_IDS_OFF);
    w32(out + 64u, 3u); w32(out + 68u, DEX_ONE_TYPE_IDS_OFF);
    w32(out + 72u, 1u); w32(out + 76u, DEX_ONE_PROTO_IDS_OFF);
    w32(out + 88u, 1u); w32(out + 92u, DEX_ONE_METHOD_IDS_OFF);
    w32(out + 96u, 1u); w32(out + 100u, DEX_ONE_CLASS_DEFS_OFF);
    w32(out + 104u, DEX_ONE_DATA_SZ);
    w32(out + 108u, DEX_ONE_DATA_OFF);

    /* string_ids sorted by UTF-16 value. */
    w32(out + DEX_ONE_STRING_IDS_OFF + 0u, DEX_ONE_STRING_OBJECT_OFF);
    w32(out + DEX_ONE_STRING_IDS_OFF + 4u, DEX_ONE_STRING_CLASS_OFF);
    w32(out + DEX_ONE_STRING_IDS_OFF + 8u, DEX_ONE_STRING_VOID_OFF);
    w32(out + DEX_ONE_STRING_IDS_OFF + 12u, DEX_ONE_STRING_RUN_OFF);

    /* type_ids: Object, Stub, void. */
    w32(out + DEX_ONE_TYPE_IDS_OFF + 0u, 0u);
    w32(out + DEX_ONE_TYPE_IDS_OFF + 4u, 1u);
    w32(out + DEX_ONE_TYPE_IDS_OFF + 8u, 2u);

    /* proto_ids: shorty V, return V, no parameters. */
    w32(out + DEX_ONE_PROTO_IDS_OFF + 0u, 2u);
    w32(out + DEX_ONE_PROTO_IDS_OFF + 4u, 2u);
    w32(out + DEX_ONE_PROTO_IDS_OFF + 8u, 0u);

    /* method_ids: Lraf/apkc/Stub;->run()V. */
    w16(out + DEX_ONE_METHOD_IDS_OFF + 0u, 1u);
    w16(out + DEX_ONE_METHOD_IDS_OFF + 2u, 0u);
    w32(out + DEX_ONE_METHOD_IDS_OFF + 4u, 3u);

    /* class_defs: public Stub extends java.lang.Object. */
    w32(out + DEX_ONE_CLASS_DEFS_OFF + 0u, 1u);
    w32(out + DEX_ONE_CLASS_DEFS_OFF + 4u, DEX_ONE_ACC_PUBLIC);
    w32(out + DEX_ONE_CLASS_DEFS_OFF + 8u, 0u);
    w32(out + DEX_ONE_CLASS_DEFS_OFF + 12u, 0u);
    w32(out + DEX_ONE_CLASS_DEFS_OFF + 16u, DEX_ONE_NO_INDEX);
    w32(out + DEX_ONE_CLASS_DEFS_OFF + 20u, 0u);
    w32(out + DEX_ONE_CLASS_DEFS_OFF + 24u, DEX_ONE_CLASS_DATA_OFF);
    w32(out + DEX_ONE_CLASS_DEFS_OFF + 28u, 0u);

    dex_one_ascii_string(out, DEX_ONE_STRING_OBJECT_OFF, "Ljava/lang/Object;", 18u);
    dex_one_ascii_string(out, DEX_ONE_STRING_CLASS_OFF, "Lraf/apkc/Stub;", 15u);
    dex_one_ascii_string(out, DEX_ONE_STRING_VOID_OFF, "V", 1u);
    dex_one_ascii_string(out, DEX_ONE_STRING_RUN_OFF, "run", 3u);

    /* class_data_item ULEB128: 0 fields, 1 direct static method. */
    out[DEX_ONE_CLASS_DATA_OFF + 0u] = 0u;
    out[DEX_ONE_CLASS_DATA_OFF + 1u] = 0u;
    out[DEX_ONE_CLASS_DATA_OFF + 2u] = 1u;
    out[DEX_ONE_CLASS_DATA_OFF + 3u] = 0u;
    out[DEX_ONE_CLASS_DATA_OFF + 4u] = 0u;
    out[DEX_ONE_CLASS_DATA_OFF + 5u] = (u8)(DEX_ONE_ACC_PUBLIC | DEX_ONE_ACC_STATIC);
    out[DEX_ONE_CLASS_DATA_OFF + 6u] = 0xF8u;
    out[DEX_ONE_CLASS_DATA_OFF + 7u] = 0x01u;

    /* code_item: no registers/arguments/outs/tries/debug; return-void. */
    w16(out + DEX_ONE_CODE_OFF + 0u, 0u);
    w16(out + DEX_ONE_CODE_OFF + 2u, 0u);
    w16(out + DEX_ONE_CODE_OFF + 4u, 0u);
    w16(out + DEX_ONE_CODE_OFF + 6u, 0u);
    w32(out + DEX_ONE_CODE_OFF + 8u, 0u);
    w32(out + DEX_ONE_CODE_OFF + 12u, 1u);
    w16(out + DEX_ONE_CODE_OFF + 16u, DEX_ONE_OP_RETURN_VOID);

    /* map_list sorted by file offset. */
    u8 *mp = out + DEX_ONE_MAP_OFF;
    w32(mp, DEX_ONE_MAP_ITEM_COUNT);
    dex_one_map_item(mp + 4u,   DEX_TYPE_HEADER,           1u, 0u);
    dex_one_map_item(mp + 16u,  DEX_ONE_TYPE_STRING_ID,    4u, DEX_ONE_STRING_IDS_OFF);
    dex_one_map_item(mp + 28u,  DEX_ONE_TYPE_TYPE_ID,      3u, DEX_ONE_TYPE_IDS_OFF);
    dex_one_map_item(mp + 40u,  DEX_ONE_TYPE_PROTO_ID,     1u, DEX_ONE_PROTO_IDS_OFF);
    dex_one_map_item(mp + 52u,  DEX_ONE_TYPE_METHOD_ID,    1u, DEX_ONE_METHOD_IDS_OFF);
    dex_one_map_item(mp + 64u,  DEX_ONE_TYPE_CLASS_DEF,    1u, DEX_ONE_CLASS_DEFS_OFF);
    dex_one_map_item(mp + 76u,  DEX_ONE_TYPE_STRING_DATA,  4u, DEX_ONE_STRING_OBJECT_OFF);
    dex_one_map_item(mp + 88u,  DEX_ONE_TYPE_CLASS_DATA,   1u, DEX_ONE_CLASS_DATA_OFF);
    dex_one_map_item(mp + 100u, DEX_ONE_TYPE_CODE_ITEM,    1u, DEX_ONE_CODE_OFF);
    dex_one_map_item(mp + 112u, DEX_TYPE_MAPLIST,          1u, DEX_ONE_MAP_OFF);

    dex_one_finish(out);
    return DEX_ONE_CLASS_SZ;
}
