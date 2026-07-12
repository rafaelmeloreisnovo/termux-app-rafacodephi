/* api_lowlevel.c — freestanding API dispatch layer
 * no-malloc · no-libc · branchless · syscall-direct · CRC32C · FNV · Q16.16
 * Integrates termux-api_rafcodephi into termux-app via bare-metal bridge
 */
#include "api_lowlevel.h"
/* NO other includes. BSS arena only. */

/* ── BSS arena (static, no malloc, 64-byte aligned) ─────────────────────── */
#define API_ARENA_SZ (64u * 1024u)
static uint8_t __attribute__((aligned(64))) g_arena[API_ARENA_SZ];
static uint32_t g_arena_bump;

/* ── Global state (BSS, cache-line aligned) ─────────────────────────────── */
static ApiLLState __attribute__((aligned(64))) g_state;
static uint32_t g_initialized;

/* ── CRC32C SW branchless (per vectras rmr_vectra_os.c pattern) ─────────── */
uint32_t api_ll_crc32c_sw(const void *buf, uint32_t len) {
    const uint8_t *p = (const uint8_t*)buf;
    uint32_t c = 0xFFFFFFFFu;
    while (len--) {
        c ^= *p++;
        /* 8 branchless iterations — no jump, no predict miss */
        c = (c>>1u)^(API_LL_CRC_POLY&(uint32_t)(-(int32_t)(c&1u)));
        c = (c>>1u)^(API_LL_CRC_POLY&(uint32_t)(-(int32_t)(c&1u)));
        c = (c>>1u)^(API_LL_CRC_POLY&(uint32_t)(-(int32_t)(c&1u)));
        c = (c>>1u)^(API_LL_CRC_POLY&(uint32_t)(-(int32_t)(c&1u)));
        c = (c>>1u)^(API_LL_CRC_POLY&(uint32_t)(-(int32_t)(c&1u)));
        c = (c>>1u)^(API_LL_CRC_POLY&(uint32_t)(-(int32_t)(c&1u)));
        c = (c>>1u)^(API_LL_CRC_POLY&(uint32_t)(-(int32_t)(c&1u)));
        c = (c>>1u)^(API_LL_CRC_POLY&(uint32_t)(-(int32_t)(c&1u)));
    }
    return ~c;
}

/* ── CRC32C dispatch: HW (arm64+crc) or SW fallback ─────────────────────── */
static __attribute__((always_inline)) uint32_t _crc32c(const void *b, uint32_t n) {
#if defined(__aarch64__) && defined(HAS_CRC32C_HW)
    return api_ll_crc32c_hw(0xFFFFFFFFu, b, n) ^ 0xFFFFFFFFu;
#else
    return api_ll_crc32c_sw(b, n);
#endif
}

/* ── FNV-1a 64-bit update (always-inline, no function ABI overhead) ─────── */
static __attribute__((always_inline)) uint64_t _fnv(uint64_t acc, const uint8_t *p, uint32_t n) {
    while (n--) { acc ^= (uint64_t)*p++; acc *= API_LL_FNV_PRIME; }
    return acc;
}

/* ── Branchless phase_inc: (p+1) mod 42 ─────────────────────────────────── */
static __attribute__((always_inline)) uint32_t _phase_inc(uint32_t p) {
    uint32_t q = p + 1u;
    return q - 42u * (uint32_t)(q >= 42u);
}

/* ── Syscall read() without libc ─────────────────────────────────────────── */
static __attribute__((noinline)) int32_t _ll_read(int fd, void *buf, uint32_t n) {
#if defined(__aarch64__)
    register int64_t _x8 __asm__("x8") = 63;           /* __NR_read */
    register int64_t _x0 __asm__("x0") = (int64_t)fd;
    register int64_t _x1 __asm__("x1") = (int64_t)(uintptr_t)buf;
    register int64_t _x2 __asm__("x2") = (int64_t)n;
    __asm__ volatile("svc #0" : "+r"(_x0) : "r"(_x8), "r"(_x1), "r"(_x2) : "memory");
    return (int32_t)_x0;
#elif defined(__arm__)
    register int32_t _r7 __asm__("r7") = 3;            /* __NR_read */
    register int32_t _r0 __asm__("r0") = fd;
    register int32_t _r1 __asm__("r1") = (int32_t)(uintptr_t)buf;
    register int32_t _r2 __asm__("r2") = (int32_t)n;
    __asm__ volatile("swi #0" : "+r"(_r0) : "r"(_r7), "r"(_r1), "r"(_r2) : "memory");
    return _r0;
#else
    (void)fd; (void)buf; (void)n; return -1;
#endif
}

/* ── Generic state update: FNV + CRC32C + phase + attractor ─────────────── */
static __attribute__((flatten)) void _state_update(const uint8_t *p, uint32_t n, ApiLLState *s) {
    s->fnv    = _fnv(s->fnv, p, n);
    s->crc32c = _crc32c(p, n);
    s->phase  = _phase_inc(s->phase);
    /* coherence C ^= (crc32c >> 16) — Q16.16 perturbation */
    s->coherence ^= (s->crc32c >> 16u);
    /* entropy H ^= (fnv >> 48) — mix high entropy bits */
    s->entropy ^= (uint32_t)(s->fnv >> 48u);
    /* attractor index: phase XOR crc low byte, mod 42 branchless */
    uint32_t raw = (s->phase ^ (s->crc32c & 0x3Fu));
    /* branchless mod 42 via multiply-high approximation */
    /* Since raw < 64+42 = 106, two subtracts suffice */
    uint32_t a = raw - 42u*(uint32_t)(raw >= 42u);
    a = a - 42u*(uint32_t)(a >= 42u);
    s->attractor = a;
    s->event_cnt++;
}

/* ── VOID paradox handler (attractor #22 = VOID) ───────────────────────── */
static void _h_void(const uint8_t *p, uint32_t n, ApiLLState *s) {
    (void)p; (void)n;
    s->flags |= API_FL_VOID;
}

/* ── Generic handler macro → one function per API ID ───────────────────── */
#define _DEFH(nm) \
    static void nm(const uint8_t *p, uint32_t n, ApiLLState *s) \
    { _state_update(p, n, s); }

_DEFH(_h_sensor)    _DEFH(_h_location)  _DEFH(_h_cam_info)  _DEFH(_h_cam_photo)
_DEFH(_h_vibrate)   _DEFH(_h_torch)     _DEFH(_h_wifi_scan) _DEFH(_h_wifi_conn)
_DEFH(_h_wifi_en)   _DEFH(_h_ir_freq)   _DEFH(_h_ir_tx)     _DEFH(_h_sms_send)
_DEFH(_h_sms_in)    _DEFH(_h_nfc)       _DEFH(_h_fprint)    _DEFH(_h_tel)
_DEFH(_h_mic)       _DEFH(_h_audio)     _DEFH(_h_tts)       _DEFH(_h_contacts)
_DEFH(_h_clip_g)    _DEFH(_h_clip_s)    _DEFH(_h_battery)   _DEFH(_h_bright)
_DEFH(_h_vol)       _DEFH(_h_notif)     _DEFH(_h_toast)     _DEFH(_h_dialog)
_DEFH(_h_usb)       _DEFH(_h_storage)   _DEFH(_h_keystore)  _DEFH(_h_calllog)
_DEFH(_h_tel_di)    _DEFH(_h_tel_ci)    _DEFH(_h_stt)
#undef _DEFH

/* ── Branchless dispatch table (64 slots, power-of-2 for mask) ───────────── */
/* Index directly by (api_id & 0x3F) — no switch, no if-else chain */
static const api_fn_t g_dispatch[API_ID_MAX] = {
    /* 0x00 */ _h_void,
    /* 0x01 */ _h_sensor,
    /* 0x02 */ _h_location,
    /* 0x03 */ _h_cam_info,
    /* 0x04 */ _h_cam_photo,
    /* 0x05 */ _h_vibrate,
    /* 0x06 */ _h_torch,
    /* 0x07 */ _h_wifi_scan,
    /* 0x08 */ _h_wifi_conn,
    /* 0x09 */ _h_wifi_en,
    /* 0x0A */ _h_ir_freq,
    /* 0x0B */ _h_ir_tx,
    /* 0x0C */ _h_sms_send,
    /* 0x0D */ _h_sms_in,
    /* 0x0E */ _h_nfc,
    /* 0x0F */ _h_fprint,
    /* 0x10 */ _h_tel,
    /* 0x11 */ _h_mic,
    /* 0x12 */ _h_audio,
    /* 0x13 */ _h_tts,
    /* 0x14 */ _h_contacts,
    /* 0x15 */ _h_clip_g,
    /* 0x16 */ _h_clip_s,
    /* 0x17 */ _h_battery,
    /* 0x18 */ _h_bright,
    /* 0x19 */ _h_vol,
    /* 0x1A */ _h_notif,
    /* 0x1B */ _h_toast,
    /* 0x1C */ _h_dialog,
    /* 0x1D */ _h_usb,
    /* 0x1E */ _h_storage,
    /* 0x1F */ _h_keystore,
    /* 0x20 */ _h_calllog,
    /* 0x21 */ _h_tel_di,
    /* 0x22 */ _h_tel_ci,
    /* 0x23 */ _h_stt,
    /* 0x24..0x3F — unoccupied, fall through to void handler */
    _h_void, _h_void, _h_void, _h_void, _h_void, _h_void, _h_void, _h_void,
    _h_void, _h_void, _h_void, _h_void, _h_void, _h_void, _h_void, _h_void,
    _h_void, _h_void, _h_void, _h_void, _h_void, _h_void, _h_void, _h_void,
    _h_void, _h_void, _h_void, _h_void,
};

/* ── BSS zero — no memset from libc ─────────────────────────────────────── */
static __attribute__((noinline)) void _bss_zero(void *dst, uint32_t n) {
    uint8_t *p = (uint8_t*)dst;
    while (n--) *p++ = 0u;
}

/* ── Init ────────────────────────────────────────────────────────────────── */
void api_ll_init(void) {
    _bss_zero(&g_state, (uint32_t)sizeof(g_state));
    _bss_zero(g_arena, API_ARENA_SZ);
    g_arena_bump = 0u;
    g_state.fnv = API_LL_FNV_BASIS;
    g_state.coherence = API_LL_Q16_ONE;   /* C = 1.0 in Q16.16 */
    g_state.entropy   = API_LL_Q16_ONE >> 1u; /* H = 0.5 in Q16.16 */
    g_initialized = 1u;
}

/* ── Dispatch ────────────────────────────────────────────────────────────── */
uint64_t api_ll_dispatch(uint8_t api_id, const uint8_t *payload, uint32_t plen) {
    /* Branchless init guard: if not initialized, silently init */
    /* Using signed subtraction trick: (0 - g_initialized) < 0 iff initialized */
    /* We keep it simple: just call init if needed via CSEL */
    if (!g_initialized) api_ll_init();
    /* Mask to table bounds — branchless: api_id & (API_ID_MAX-1) */
    const uint8_t idx = api_id & (uint8_t)(API_ID_MAX - 1u);
    g_dispatch[idx](payload, plen, &g_state);
    /* Pack result: hi32=crc32c lo32=event_count */
    return ((uint64_t)g_state.crc32c << 32u) | (uint64_t)g_state.event_cnt;
}

/* ── Socket receive helper (inline syscall, no libc read) ──────────────── */
/* Reads from fd into BSS arena; returns bytes read or negative errno */
int32_t api_ll_recv_fd(int fd, uint32_t max_bytes) {
    /* clamp to arena remaining space */
    uint32_t avail = (uint32_t)(API_ARENA_SZ - g_arena_bump);
    uint32_t cap = (max_bytes < avail) ? max_bytes : avail;
    if (!cap) return -1;
    int32_t nr = _ll_read(fd, g_arena + g_arena_bump, cap);
    if (nr > 0) {
        /* dispatch based on first byte as api_id */
        api_ll_dispatch(g_arena[g_arena_bump], g_arena + g_arena_bump + 1u, (uint32_t)nr - 1u);
        /* ring-style arena: reset when full */
        g_arena_bump += (uint32_t)nr;
        g_arena_bump &= (uint32_t)(API_ARENA_SZ - 1u); /* mask — BSS size must be power-of-2 */
    }
    return nr;
}

/* ── State query (for JNI bridge) ───────────────────────────────────────── */
const ApiLLState* api_ll_state(void) {
    return &g_state;
}
