/* fmt_axml.h — Android Binary XML (AXML) encoder for AndroidManifest.xml.
 * Produces a NativeActivity manifest ready to be stored into an APK.
 * No heap. No malloc. Writes into caller-supplied buffer.
 *
 * String pool layout (indices fixed by SI_* constants):
 *   0  ""                                    (empty)
 *   1  "android"                             (NS prefix)
 *   2  "http://schemas.android.com/apk/res/android"  (NS URI)
 *   3  "versionCode"    resID 0x0101021B
 *   4  "versionName"    resID 0x0101021C
 *   5  "package"        resID 0 (plain XML attr)
 *   6  "minSdkVersion"  resID 0x0101020C
 *   7  "targetSdkVersion" resID 0x01010270
 *   8  "label"          resID 0x01010001
 *   9  "hasCode"        resID 0x0101000C
 *  10  "name"           resID 0x01010003
 *  11  "exported"       resID 0x01010010
 *  12  "value"          resID 0x01010024
 *  13..20  element names: manifest uses-sdk application activity meta-data
 *                         intent-filter action category
 *  21  PKG_NAME  (variable, injected by caller)
 *  22  "1.0"     (versionName value)
 *  23  APP_LABEL (variable)
 *  24  "false"
 *  25  "android.app.NativeActivity"
 *  26  "android.app.lib_name"
 *  27  LIB_NAME  (variable, e.g. "main")
 *  28  "android.intent.action.MAIN"
 *  29  "android.intent.category.LAUNCHER"
 */
#pragma once
#include "mem.h"

/* ── String-pool index constants ──────────────────────────────────────── */
#define SI_EMPTY    0u
#define SI_AND_PFX  1u
#define SI_AND_URI  2u
#define SI_VCODE    3u
#define SI_VNAME    4u
#define SI_PKG      5u
#define SI_MINSDK   6u
#define SI_TGTSDK   7u
#define SI_LABEL    8u
#define SI_HASCODE  9u
#define SI_NAME    10u
#define SI_EXPORTED 11u
#define SI_VALUE   12u
#define SI_EL_MAN  13u
#define SI_EL_SDK  14u
#define SI_EL_APP  15u
#define SI_EL_ACT  16u
#define SI_EL_META 17u
#define SI_EL_FILT 18u
#define SI_EL_ACN  19u
#define SI_EL_CAT  20u
#define SI_PKG_VAL 21u
#define SI_VN_VAL  22u
#define SI_LBL_VAL 23u
#define SI_FALSE   24u
#define SI_NACT    25u
#define SI_LIBKEY  26u
#define SI_LIB_VAL 27u
#define SI_ACTMAIN 28u
#define SI_CATLNCH 29u
#define SI_COUNT   30u

/* Resource IDs for android-namespace attributes (index 0..12) */
static const u32 _ax_resid[13] = {
    0u, 0u, 0u,
    0x0101021Bu, /* versionCode  */
    0x0101021Cu, /* versionName  */
    0u,          /* package      */
    0x0101020Cu, /* minSdkVersion */
    0x01010270u, /* targetSdkVersion */
    0x01010001u, /* label        */
    0x0101000Cu, /* hasCode      */
    0x01010003u, /* name         */
    0x01010010u, /* exported     */
    0x01010024u, /* value        */
};

/* Fixed string literals (indices 0-20 and 22,24-26,28-29) */
static const char *const _ax_lit[SI_COUNT] = {
    "",
    "android",
    "http://schemas.android.com/apk/res/android",
    "versionCode", "versionName", "package",
    "minSdkVersion", "targetSdkVersion",
    "label", "hasCode", "name", "exported", "value",
    "manifest", "uses-sdk", "application", "activity",
    "meta-data", "intent-filter", "action", "category",
    NULL,         /* SI_PKG_VAL  — caller fills */
    "1.0",
    NULL,         /* SI_LBL_VAL  — caller fills */
    "false",
    "android.app.NativeActivity",
    "android.app.lib_name",
    NULL,         /* SI_LIB_VAL  — caller fills */
    "android.intent.action.MAIN",
    "android.intent.category.LAUNCHER",
};

/* ── AXML writer state ───────────────────────────────────────────────────── */
typedef struct {
    u8  *buf;
    sz   cap;
    sz   pos;
    u32  str_off[SI_COUNT]; /* string offsets in pool (from stringsStart) */
    const char *sv[SI_COUNT]; /* resolved string values */
} AxWr;

static inline void _ax16(AxWr *a, u16 v){ u8 b[2]; w16(b,v); m_cpy(a->buf+a->pos,b,2); a->pos+=2; }
static inline void _ax32(AxWr *a, u32 v){ u8 b[4]; w32(b,v); m_cpy(a->buf+a->pos,b,4); a->pos+=4; }
static inline void _axb (AxWr *a, const void *src, sz n){ m_cpy(a->buf+a->pos,src,n); a->pos+=n; }

/* Write a UTF-16LE string: u16 char_count, u16[] chars, u16 null */
static inline u32 _ax_utf16(u8 *out, const char *s) {
    u32 n = (u32)s_len(s);
    w16(out, (u16)n);
    for (u32 i = 0; i < n; i++) w16(out+2+i*2, (u16)(u8)s[i]);
    w16(out+2+n*2, 0u);
    return 2u + n*2u + 2u;
}

/* Patch a u32 at absolute offset in buf */
static inline void _ax_patch32(AxWr *a, sz off, u32 v){ w32(a->buf+off, v); }

/* ── Public API ─────────────────────────────────────────────────────────── */
/*
 * axml_build() — generate a NativeActivity AndroidManifest.xml (AXML).
 *   pkg      : package name, e.g. "com.example.myapp"
 *   label    : app label string, e.g. "My App"
 *   libname  : native library name (without lib prefix or .so), e.g. "main"
 *   min_sdk  : minSdkVersion integer (e.g. 21)
 *   tgt_sdk  : targetSdkVersion integer (e.g. 33)
 *   out      : caller-supplied buffer (at least 8 KiB)
 * Returns number of bytes written.
 */
static sz axml_build(const char *pkg, const char *label,
                     const char *libname, u32 min_sdk, u32 tgt_sdk,
                     u8 *out, sz cap)
{
    AxWr A; A.buf = out; A.cap = cap; A.pos = 0;

    /* Resolve variable strings */
    for (u32 i = 0; i < SI_COUNT; i++) A.sv[i] = _ax_lit[i];
    A.sv[SI_PKG_VAL] = pkg;
    A.sv[SI_LBL_VAL] = label;
    A.sv[SI_LIB_VAL] = libname;

    /* ---- 1. Compute string pool sizes ---- */
    u32 str_data_sz = 0;
    for (u32 i = 0; i < SI_COUNT; i++) {
        A.str_off[i] = str_data_sz;
        u32 n = (u32)s_len(A.sv[i]);
        str_data_sz += 2u + n*2u + 2u; /* u16 len + UTF-16 chars + null */
    }
    /* Pad string data to 4-byte boundary */
    str_data_sz = u32_aln(str_data_sz, 4u);

    u32 str_off_arr = 0x1Cu;             /* string pool headerSize = 28 */
    u32 strings_start = str_off_arr + SI_COUNT*4u;
    u32 sp_chunk_sz = strings_start + str_data_sz;

    /* Resource map chunk: 8-byte header + 13 resIDs * 4 */
    u32 rm_chunk_sz = 8u + 13u*4u;

    /* ---- 2. File header (RES_XML_TYPE) ---- */
    sz file_hdr_off = A.pos;
    _ax16(&A, 0x0003u);        /* type = RES_XML_TYPE */
    _ax16(&A, 8u);              /* headerSize */
    _ax32(&A, 0u);              /* total size — patched at end */

    /* ---- 3. String pool chunk ---- */
    sz sp_off = A.pos;
    _ax16(&A, 0x0001u);         /* type = RES_STRING_POOL_TYPE */
    _ax16(&A, 0x001Cu);         /* headerSize = 28 */
    _ax32(&A, sp_chunk_sz);     /* chunkSize */
    _ax32(&A, SI_COUNT);        /* stringCount */
    _ax32(&A, 0u);              /* styleCount */
    _ax32(&A, 0u);              /* flags = UTF-16 */
    _ax32(&A, strings_start);   /* stringsStart (offset from chunk start) */
    _ax32(&A, 0u);              /* stylesStart */
    /* String offset table */
    for (u32 i = 0; i < SI_COUNT; i++) _ax32(&A, A.str_off[i]);
    /* String data */
    sz str_data_start = A.pos;
    for (u32 i = 0; i < SI_COUNT; i++) {
        u32 n = _ax_utf16(A.buf + A.pos, A.sv[i]);
        A.pos += n;
    }
    /* Pad to 4 bytes */
    while ((A.pos - str_data_start) < str_data_sz) { A.buf[A.pos++] = 0; }
    (void)sp_off;

    /* ---- 4. Resource map chunk ---- */
    _ax16(&A, 0x0180u);         /* type = RES_XML_RESOURCE_MAP_TYPE */
    _ax16(&A, 8u);               /* headerSize */
    _ax32(&A, rm_chunk_sz);      /* chunkSize */
    for (u32 i = 0; i < 13u; i++) _ax32(&A, _ax_resid[i]);

    /* ---- 5. XML nodes ---- */
    /* Helper macros for node writing */
#define _LINE 1u
#define _CMT  0xFFFFFFFFu
#define _NS   SI_AND_URI
#define _NONS 0xFFFFFFFFu

    /* ── Start namespace ── */
    _ax16(&A,0x0100u);_ax16(&A,16u);_ax32(&A,24u); /* type hdr sz chunkSz */
    _ax32(&A,_LINE);_ax32(&A,_CMT);
    _ax32(&A,SI_AND_PFX);_ax32(&A,SI_AND_URI);

#define _START_ELEM(nsidx, nameidx, nattr) do { \
    sz _csz_off = A.pos+4; \
    _ax16(&A,0x0102u); _ax16(&A,16u); _ax32(&A,0u); \
    _ax32(&A,_LINE); _ax32(&A,_CMT); \
    _ax32(&A,(u32)(nsidx)); _ax32(&A,(u32)(nameidx)); \
    _ax16(&A,0x14u); _ax16(&A,0x14u); _ax16(&A,(u16)(nattr)); \
    _ax16(&A,0u); _ax16(&A,0u); _ax16(&A,0u); \
    sz _body_start = A.pos; (void)_body_start; (void)_csz_off; \
} while(0)

#define _END_ELEM(nsidx, nameidx) do { \
    _ax16(&A,0x0103u); _ax16(&A,16u); _ax32(&A,24u); \
    _ax32(&A,_LINE); _ax32(&A,_CMT); \
    _ax32(&A,(u32)(nsidx)); _ax32(&A,(u32)(nameidx)); \
} while(0)

    /* Attribute writer: (ns, name, rawVal, dataType, data) */
    /* dataType: 0x03=TYPE_STRING, 0x10=TYPE_INT_DEC, 0x12=TYPE_BOOL */
#define _ATTR(ns, nm, rv, dt, data) do { \
    _ax32(&A,(u32)(ns)); _ax32(&A,(u32)(nm)); _ax32(&A,(u32)(rv)); \
    _ax16(&A,8u); A.buf[A.pos++]=0; A.buf[A.pos++]=(u8)(dt); \
    _ax32(&A,(u32)(data)); \
} while(0)

    /* ── <manifest package="PKG" android:versionCode="1" android:versionName="1.0"> */
    /* chunkSize = 16(hdr)+8(node)+20(attrExt)+3*20(attrs) = 16+8+20+60 = 104 */
    /* We'll compute it precisely: headerSize=16, nodeHdr=8, attrExt=20, each attr=20 */
    sz _man_csz = A.pos + 4;
    _ax16(&A,0x0102u); _ax16(&A,16u); _ax32(&A,104u);
    _ax32(&A,_LINE); _ax32(&A,_CMT);
    _ax32(&A,_NONS); _ax32(&A,SI_EL_MAN);
    _ax16(&A,0x14u); _ax16(&A,0x14u); _ax16(&A,3u);
    _ax16(&A,0u); _ax16(&A,0u); _ax16(&A,0u);
    _ATTR(_NONS,   SI_PKG,   SI_PKG_VAL, 0x03u, SI_PKG_VAL); /* package=".." */
    _ATTR(SI_AND_URI, SI_VCODE, _CMT, 0x10u, 1u);            /* versionCode=1 */
    _ATTR(SI_AND_URI, SI_VNAME, SI_VN_VAL, 0x03u, SI_VN_VAL);/* versionName */
    (void)_man_csz;

    /* ── <uses-sdk android:minSdkVersion="N" android:targetSdkVersion="N"/> */
    sz _sdk_csz = A.pos + 4;
    _ax16(&A,0x0102u); _ax16(&A,16u); _ax32(&A,96u);
    _ax32(&A,_LINE); _ax32(&A,_CMT);
    _ax32(&A,_NONS); _ax32(&A,SI_EL_SDK);
    _ax16(&A,0x14u); _ax16(&A,0x14u); _ax16(&A,2u);
    _ax16(&A,0u); _ax16(&A,0u); _ax16(&A,0u);
    _ATTR(SI_AND_URI, SI_MINSDK, _CMT, 0x10u, min_sdk);
    _ATTR(SI_AND_URI, SI_TGTSDK, _CMT, 0x10u, tgt_sdk);
    _END_ELEM(_NONS, SI_EL_SDK);
    (void)_sdk_csz;

    /* ── <application android:label=".." android:hasCode="false"> */
    _ax16(&A,0x0102u); _ax16(&A,16u); _ax32(&A,96u);
    _ax32(&A,_LINE); _ax32(&A,_CMT);
    _ax32(&A,_NONS); _ax32(&A,SI_EL_APP);
    _ax16(&A,0x14u); _ax16(&A,0x14u); _ax16(&A,2u);
    _ax16(&A,0u); _ax16(&A,0u); _ax16(&A,0u);
    _ATTR(SI_AND_URI, SI_LABEL,   SI_LBL_VAL, 0x03u, SI_LBL_VAL);
    _ATTR(SI_AND_URI, SI_HASCODE, SI_FALSE,    0x12u, 0u); /* false */

    /* ── <activity android:name="NativeActivity" android:label=".." android:exported="true"> */
    _ax16(&A,0x0102u); _ax16(&A,16u); _ax32(&A,116u);
    _ax32(&A,_LINE); _ax32(&A,_CMT);
    _ax32(&A,_NONS); _ax32(&A,SI_EL_ACT);
    _ax16(&A,0x14u); _ax16(&A,0x14u); _ax16(&A,3u);
    _ax16(&A,0u); _ax16(&A,0u); _ax16(&A,0u);
    _ATTR(SI_AND_URI, SI_NAME,     SI_NACT,    0x03u, SI_NACT);
    _ATTR(SI_AND_URI, SI_LABEL,    SI_LBL_VAL, 0x03u, SI_LBL_VAL);
    _ATTR(SI_AND_URI, SI_EXPORTED, _CMT,       0x12u, 1u); /* true */

    /* ── <meta-data android:name="android.app.lib_name" android:value="libname"/> */
    _ax16(&A,0x0102u); _ax16(&A,16u); _ax32(&A,96u);
    _ax32(&A,_LINE); _ax32(&A,_CMT);
    _ax32(&A,_NONS); _ax32(&A,SI_EL_META);
    _ax16(&A,0x14u); _ax16(&A,0x14u); _ax16(&A,2u);
    _ax16(&A,0u); _ax16(&A,0u); _ax16(&A,0u);
    _ATTR(SI_AND_URI, SI_NAME,  SI_LIBKEY,  0x03u, SI_LIBKEY);
    _ATTR(SI_AND_URI, SI_VALUE, SI_LIB_VAL, 0x03u, SI_LIB_VAL);
    _END_ELEM(_NONS, SI_EL_META);

    /* ── <intent-filter> */
    _ax16(&A,0x0102u); _ax16(&A,16u); _ax32(&A,44u);
    _ax32(&A,_LINE); _ax32(&A,_CMT);
    _ax32(&A,_NONS); _ax32(&A,SI_EL_FILT);
    _ax16(&A,0x14u); _ax16(&A,0x14u); _ax16(&A,0u);
    _ax16(&A,0u); _ax16(&A,0u); _ax16(&A,0u);

    /* ── <action android:name="android.intent.action.MAIN"/> */
    _ax16(&A,0x0102u); _ax16(&A,16u); _ax32(&A,76u);
    _ax32(&A,_LINE); _ax32(&A,_CMT);
    _ax32(&A,_NONS); _ax32(&A,SI_EL_ACN);
    _ax16(&A,0x14u); _ax16(&A,0x14u); _ax16(&A,1u);
    _ax16(&A,0u); _ax16(&A,0u); _ax16(&A,0u);
    _ATTR(SI_AND_URI, SI_NAME, SI_ACTMAIN, 0x03u, SI_ACTMAIN);
    _END_ELEM(_NONS, SI_EL_ACN);

    /* ── <category android:name="android.intent.category.LAUNCHER"/> */
    _ax16(&A,0x0102u); _ax16(&A,16u); _ax32(&A,76u);
    _ax32(&A,_LINE); _ax32(&A,_CMT);
    _ax32(&A,_NONS); _ax32(&A,SI_EL_CAT);
    _ax16(&A,0x14u); _ax16(&A,0x14u); _ax16(&A,1u);
    _ax16(&A,0u); _ax16(&A,0u); _ax16(&A,0u);
    _ATTR(SI_AND_URI, SI_NAME, SI_CATLNCH, 0x03u, SI_CATLNCH);
    _END_ELEM(_NONS, SI_EL_CAT);

    _END_ELEM(_NONS, SI_EL_FILT);
    _END_ELEM(_NONS, SI_EL_ACT);
    _END_ELEM(_NONS, SI_EL_APP);
    _END_ELEM(_NONS, SI_EL_MAN);

    /* ── End namespace ── */
    _ax16(&A,0x0101u); _ax16(&A,16u); _ax32(&A,24u);
    _ax32(&A,_LINE); _ax32(&A,_CMT);
    _ax32(&A,SI_AND_PFX); _ax32(&A,SI_AND_URI);

    /* Patch total file size */
    _ax_patch32(&A, file_hdr_off+4, (u32)A.pos);

#undef _LINE
#undef _CMT
#undef _NS
#undef _NONS
#undef _START_ELEM
#undef _END_ELEM
#undef _ATTR

    return A.pos;
}
