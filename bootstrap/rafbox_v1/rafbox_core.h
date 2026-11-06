/* SPDX-License-Identifier: GPL-3.0-only
 * RAFAELIA RAFBOX v1 -- original freestanding C11 parsing primitives.
 * No libc, heap, OS, syscalls or external data. This subset is NOT BusyBox.
 */
#ifndef RAFAELIA_RAFBOX_CORE_H
#define RAFAELIA_RAFBOX_CORE_H
#define RAFBOX_MAX_ARGUMENT_TEXT 4095u

typedef struct { const char *ptr; unsigned int size; } rafbox_span;
typedef enum {
    RAFBOX_UNSUPPORTED=0,
    RAFBOX_TRUE=1,
    RAFBOX_FALSE=2,
    RAFBOX_ECHO=3,
    RAFBOX_CAT=4,
    RAFBOX_BASENAME=5,
    RAFBOX_DIRNAME=6
} rafbox_applet;

static inline unsigned int rafbox_strlen_bounded(const char *s) {
    unsigned int i;
    if (!s) return RAFBOX_MAX_ARGUMENT_TEXT+1u;
    for (i=0; i<=RAFBOX_MAX_ARGUMENT_TEXT; ++i) {
        if (s[i] == 0) return i;
    }
    return RAFBOX_MAX_ARGUMENT_TEXT+1u;
}
static inline int rafbox_equals(const char *s, const char *lit) {
    unsigned int i=0u;
    if (!s || !lit) return 0;
    while (i<64u && s[i] && lit[i] && s[i]==lit[i]) ++i;
    return i<64u && s[i]==0 && lit[i]==0;
}
static inline rafbox_applet rafbox_select(const char *s) {
    if (rafbox_equals(s,"true")) return RAFBOX_TRUE;
    if (rafbox_equals(s,"false")) return RAFBOX_FALSE;
    if (rafbox_equals(s,"echo")) return RAFBOX_ECHO;
    if (rafbox_equals(s,"cat")) return RAFBOX_CAT;
    if (rafbox_equals(s,"basename")) return RAFBOX_BASENAME;
    if (rafbox_equals(s,"dirname")) return RAFBOX_DIRNAME;
    return RAFBOX_UNSUPPORTED;
}
/* Path split behavior is deliberately scoped: empty paths rejected, // treated as /. */
static inline int rafbox_basename_span(const char *s, rafbox_span *out) {
    unsigned int n, end, start;
    if (!out || (n=rafbox_strlen_bounded(s)) == 0u || n>RAFBOX_MAX_ARGUMENT_TEXT) return 0;
    end=n;
    while (end>1u && s[end-1u]=='/') --end;
    if (end==1u && s[0]=='/') {out->ptr=s; out->size=1u; return 1;}
    start=end;
    while (start>0u && s[start-1u]!='/') --start;
    out->ptr=s+start;out->size=end-start;
    return 1;
}
static inline int rafbox_dirname_span(const char *s, rafbox_span *out) {
    unsigned int n, end, cut;
    static const char dot[]={'.',0};
    if (!out || (n=rafbox_strlen_bounded(s))==0u || n>RAFBOX_MAX_ARGUMENT_TEXT) return 0;
    end=n;
    while (end>1u && s[end-1u]=='/') --end;
    if (end==1u && s[0]=='/') {out->ptr=s;out->size=1u;return 1;}
    cut=end;
    while (cut>0u && s[cut-1u]!='/') --cut;
    if (cut==0u) {out->ptr=dot;out->size=1u;return 1;}
    while (cut>1u && s[cut-1u]=='/') --cut;
    out->ptr=s;out->size=cut;
    return 1;
}
#endif
