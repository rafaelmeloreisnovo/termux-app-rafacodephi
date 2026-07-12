/* api_lowlevel.h — API dispatch contract
 * freestanding · no-malloc · no-libc · branchless dispatch
 * Register contract (AGENTS2.md): x0=state x1=C x2=H x3=phase x4=att x6=fnv x7=flags
 */
#ifndef API_LOWLEVEL_H
#define API_LOWLEVEL_H
#include <stdint.h>
#include <stddef.h>

/* ── Constants ──────────────────────────────────────────────────────────── */
#define API_LL_PHI32      0x9E3779B9u
#define API_LL_FNV_BASIS  0xCBF29CE484222325ULL
#define API_LL_FNV_PRIME  0x100000001B3ULL
#define API_LL_CRC_POLY   0x82F63B78u
#define API_LL_Q16_ONE    0x00010000u

/* Branchless CSEL: (t) when c != 0, (f) otherwise — no branch misprediction */
#define API_LL_CSEL(c,t,f)  ((t) ^ (((t)^(f)) & (uint32_t)(-(uint32_t)(!!(c)))))

/* ── Flags (x7, bit-packed per AGENTS2.md) ──────────────────────────────── */
#define API_FL_LOCK     (1u<<0)
#define API_FL_FLOW     (1u<<1)
#define API_FL_VOID     (1u<<2)
#define API_FL_TRICKST  (1u<<3)
#define API_FL_VISCNEG  (1u<<4)
#define API_FL_ATTJUMP  (1u<<5)
#define API_FL_MERKLE   (1u<<6)
#define API_FL_GEOFAIL  (1u<<7)

/* ── API method IDs — direct index into dispatch table (power-of-2 size) ── */
#define API_ID_SENSOR       0x01u
#define API_ID_LOCATION     0x02u
#define API_ID_CAMERA_INFO  0x03u
#define API_ID_CAMERA_PHOTO 0x04u
#define API_ID_VIBRATE      0x05u
#define API_ID_TORCH        0x06u
#define API_ID_WIFI_SCAN    0x07u
#define API_ID_WIFI_CONN    0x08u
#define API_ID_WIFI_ENABLE  0x09u
#define API_ID_IR_FREQ      0x0Au
#define API_ID_IR_TX        0x0Bu
#define API_ID_SMS_SEND     0x0Cu
#define API_ID_SMS_INBOX    0x0Du
#define API_ID_NFC          0x0Eu
#define API_ID_FINGERPRINT  0x0Fu
#define API_ID_TELEPHONY    0x10u
#define API_ID_MIC_RECORD   0x11u
#define API_ID_AUDIO_INFO   0x12u
#define API_ID_TTS          0x13u
#define API_ID_CONTACTS     0x14u
#define API_ID_CLIP_GET     0x15u
#define API_ID_CLIP_SET     0x16u
#define API_ID_BATTERY      0x17u
#define API_ID_BRIGHTNESS   0x18u
#define API_ID_VOLUME       0x19u
#define API_ID_NOTIFICATION 0x1Au
#define API_ID_TOAST        0x1Bu
#define API_ID_DIALOG       0x1Cu
#define API_ID_USB          0x1Du
#define API_ID_STORAGE      0x1Eu
#define API_ID_KEYSTORE     0x1Fu
#define API_ID_CALL_LOG     0x20u
#define API_ID_TEL_DEVINFO  0x21u
#define API_ID_TEL_CELLINFO 0x22u
#define API_ID_STT          0x23u
#define API_ID_MAX          0x40u  /* 64 slots: power-of-2 for mask dispatch */

/* ── Shared state (64-byte cache-line aligned, BSS — no malloc) ─────────── */
typedef struct __attribute__((packed, aligned(64))) ApiLLState {
    uint64_t fnv;       /* x6: FNV-1a 64-bit accumulator */
    uint32_t crc32c;    /* CRC32C of last dispatched payload */
    uint32_t event_cnt; /* total events dispatched */
    uint32_t phase;     /* x3: phase counter mod 42 */
    uint32_t flags;     /* x7: bit-packed flags */
    uint32_t coherence; /* x1: Q16.16 coherence C */
    uint32_t entropy;   /* x2: Q16.16 entropy H */
    uint32_t attractor; /* x4: current attractor index [0..41] */
    uint32_t pad[7];    /* fill to 64 bytes: 8+4*8+7*4 = 8+32+28 = 68? recalc */
} ApiLLState;
/* Layout: fnv=8, crc32c=4, event_cnt=4, phase=4, flags=4, coherence=4, entropy=4, attractor=4 = 36, pad[7]=28 → 64 ✓ */

typedef void (*api_fn_t)(const uint8_t*, uint32_t, ApiLLState*);

/* ── External ASM hot paths (api_ll_asm.S) ──────────────────────────────── */
#if defined(__aarch64__)
extern uint64_t api_ll_cycle_read(void);
extern uint32_t api_ll_crc32c_hw(uint32_t crc, const void *buf, uint32_t len);
extern void     api_ll_state_update(ApiLLState *st);
extern uint32_t api_ll_neon_xor_fold(const void *buf, uint32_t len);
#elif defined(__arm__)
extern uint64_t api_ll_cycle_read(void);  /* MRRC CNTVCT, lo in lo-word */
extern uint32_t api_ll_neon_xor_fold(const void *buf, uint32_t len);
#else
static inline uint64_t api_ll_cycle_read(void) { return 0; }
#endif

/* ── Public API ─────────────────────────────────────────────────────────── */
void              api_ll_init(void);
uint64_t          api_ll_dispatch(uint8_t api_id, const uint8_t *payload, uint32_t plen);
uint32_t          api_ll_crc32c_sw(const void *buf, uint32_t len);
int32_t           api_ll_recv_fd(int fd, uint32_t max_bytes);
const ApiLLState *api_ll_state(void);

#endif /* API_LOWLEVEL_H */
