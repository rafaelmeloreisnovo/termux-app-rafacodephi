/**
 * rafaelia_jni_direct.c
 * JNI bridge zero-copy usando DirectByteBuffer
 * ZERO malloc/NewByteArray por chamada
 *
 * Java lado:
 *   static final ByteBuffer IN  = ByteBuffer.allocateDirect(65536);
 *   static final ByteBuffer OUT = ByteBuffer.allocateDirect(65536);
 *   static native int processNative(ByteBuffer in, int len, ByteBuffer out);
 *   static native int stepNative(ByteBuffer state, int cycle);
 *   static native long profileNative(ByteBuffer out, int cap);
 *
 * Compilar como parte do Android.mk (veja Android_nomalloc.mk)
 */

#include <jni.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>
#include "raf_vcpu.h"
#include "raf_clock.h"
#include "raf_memory_layers.h"

#ifdef __ANDROID__
#include <android/log.h>
#define TAG "RafaeliaJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#else
#define LOGI(...) (void)0
#define LOGE(...) (void)0
#endif

/* ── CRC32C inline (sem dep externa) ──────────────────────────────────── */
static uint32_t _crc_tab[256];
static int      _crc_ready = 0;

static void _crc_build(void) {
    for (uint32_t i=0;i<256;i++){
        uint32_t v=i;
        for(int j=0;j<8;j++) v=(v&1)?(v>>1)^0x82F63B78u:(v>>1);
        _crc_tab[i]=v;
    }
    _crc_ready=1;
}

static uint32_t _crc32(const void *buf, size_t n) {
    if (!_crc_ready) _crc_build();
    const uint8_t *p=(const uint8_t*)buf;
    uint32_t c=0xFFFFFFFFu;
    while(n--) c=(c>>8)^_crc_tab[(c^*p++)&0xFF];
    return ~c;
}

/* ── Estado global do orquestrador (sem malloc) ───────────────────────── */

/* arena estática de 256KB para JNI — sem malloc */
#define JNI_ARENA_SZ (256u*1024u)
static uint8_t __attribute__((aligned(64))) g_jni_arena[JNI_ARENA_SZ];
static uint32_t g_jni_bump = 0;

static void *jni_alloc(uint32_t n) {
    uint32_t s = (g_jni_bump + 63u) & ~63u;
    if (s+n > JNI_ARENA_SZ) return NULL;
    g_jni_bump = s+n;
    return g_jni_arena+s;
}

static raf_state_t *g_state = NULL;  /* aponta para arena */
static pthread_once_t g_state_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_sched_mutex = PTHREAD_MUTEX_INITIALIZER;
static raf_clock_t g_clock;
static uint32_t g_target_hz = 10;
static int g_scheduler_initialized = 0;

static void ensure_scheduler_locked(uint32_t fallback_hz) {
    if (g_scheduler_initialized) return;
    if (fallback_hz == 0) fallback_hz = 10;
    g_target_hz = fallback_hz;
    raf_clock_init(&g_clock, g_target_hz);
    raf_vcpu_init(g_target_hz);
    g_scheduler_initialized = 1;
}

static void ensure_state(void) {
    if (g_state) return;
    g_state = (raf_state_t*)jni_alloc(sizeof(raf_state_t));
    if (!g_state) return;
    /* init: toroidal map com constantes irracionais */
    const uint32_t seeds[7] = {
        56755u, 105965u, 205887u,
        46341u, 92682u,  138564u, 184245u
    };
    for (int i=0;i<RAF_STATE_DIM;i++) g_state->s[i] = seeds[i] & 0xFFFFu;
    g_state->coherence = 0x8000u;
    g_state->entropy   = 0x8000u;
    g_state->phase     = 0;
    g_state->step      = 0;
    g_state->crc       = 0;
    g_state->crc = _crc32(g_state, offsetof(raf_state_t, crc));
}
static void ensure_state_once(void) { ensure_state(); }

/* EMA update Q16.16 */
static uint32_t ema(uint32_t old, uint32_t in) {
    /* 0.75*old + 0.25*in — sem float */
    return (uint32_t)(((uint64_t)old*49152u + (uint64_t)in*16384u) >> 16);
}

/* ── processNative ────────────────────────────────────────────────────── */
/* Java: int processNative(ByteBuffer in, int inLen, ByteBuffer out)
 * Retorna: bytes escritos em out, ou -1 em erro
 * Zero malloc: opera diretamente nos DirectByteBuffer */
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_processNative(
    JNIEnv *env, jclass cls,
    jobject in_buf, jint in_len,
    jobject out_buf)
{
    (void)cls;

    uint8_t *in  = (uint8_t*)(*env)->GetDirectBufferAddress(env, in_buf);
    uint8_t *out = (uint8_t*)(*env)->GetDirectBufferAddress(env, out_buf);
    jlong in_cap = (*env)->GetDirectBufferCapacity(env, in_buf);
    jlong out_cap = (*env)->GetDirectBufferCapacity(env, out_buf);
    if (!in || !out || in_len <= 0 || out_cap < 8 || in_cap <= 0) return -1;
    if ((jlong)in_len > in_cap) return -1;

    pthread_once(&g_state_once, ensure_state_once);
    if (!g_state) return -2;
    pthread_mutex_lock(&g_state_mutex);

    /* Verifica integridade do estado */
    uint32_t saved_crc = g_state->crc;
    g_state->crc = 0;
    uint32_t check = _crc32(g_state, offsetof(raf_state_t, crc));
    g_state->crc = saved_crc;
    if (check != saved_crc) {
        /* rollback para init */
        g_state->coherence = 0x8000u;
        g_state->entropy   = 0x8000u;
        g_state->phase     = 0;
    }

    /* Processa: CRC do input como C_in, entropia como H_in */
    uint32_t c_in = _crc32(in, (size_t)in_len) & 0xFFFFu;
    /* H_in: shannon approximation — unique bytes / 256 * 65535 */
    uint8_t seen[256];
    memset(seen, 0, 256);
    int uniq = 0;
    for (int i=0; i<in_len && i<4096; i++)
        if (!seen[in[i]]) { seen[in[i]]=1; uniq++; }
    uint32_t h_in = (uint32_t)((uint64_t)uniq * 65535u / 256u);

    /* EMA update */
    g_state->coherence = ema(g_state->coherence, c_in);
    g_state->entropy   = ema(g_state->entropy,   h_in);

    /* phi = (1-H)*C Q16.16 */
    uint32_t H = g_state->entropy;
    uint32_t C = g_state->coherence;
    uint32_t phi = (uint32_t)(((uint64_t)(65535u-H)*C) >> 16);

    /* avança fase */
    g_state->phase = (g_state->phase+1u >= RAF_PERIOD) ? 0 : g_state->phase+1u;
    g_state->step++;

    /* atualiza CRC */
    g_state->crc = 0;
    g_state->crc = _crc32(g_state, offsetof(raf_state_t, crc));

    /* escreve resultado em out (8 bytes): crc(in) | phi | phase | step */
    if (out_cap >= 16) {
        uint32_t r[4] = {
            _crc32(in,(size_t)in_len),
            phi,
            g_state->phase,
            g_state->step
        };
        memcpy(out, r, 16);
        pthread_mutex_unlock(&g_state_mutex);
        return 16;
    }
    uint32_t r2[2] = { _crc32(in,(size_t)in_len), phi };
    memcpy(out, r2, 8);
    pthread_mutex_unlock(&g_state_mutex);
    return 8;
}

/* ── stepNative ───────────────────────────────────────────────────────── */
/* Java: int stepNative(ByteBuffer state, int cycle)
 * state: DirectByteBuffer de sizeof(raf_state_t) bytes — lê/escreve
 * cycle: 0..41
 * Retorna: phi Q16.16 */
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_stepNative(
    JNIEnv *env, jclass cls,
    jobject state_buf, jint cycle)
{
    (void)cls;

    raf_state_t *st = (raf_state_t*)(*env)->GetDirectBufferAddress(env, state_buf);
    jlong cap = (*env)->GetDirectBufferCapacity(env, state_buf);
    if (!st || cap < (jlong)sizeof(raf_state_t) || cycle < 0) return -1;

    /* verifica CRC */
    uint32_t sc = st->crc;
    st->crc = 0;
    if (_crc32(st, offsetof(raf_state_t,crc)) != sc) {
        /* corrupção detectada — reinit */
        st->coherence = 0x8000u;
        st->entropy   = 0x8000u;
        st->phase     = 0;
        st->step      = 0;
    }
    st->crc = sc;

    /* input sintético baseado no ciclo */
    uint32_t c_in = (uint32_t)((56755u * (uint32_t)cycle) >> 4) & 0xFFFFu;
    uint32_t h_in = (uint32_t)((65535u - c_in));

    st->coherence = ema(st->coherence, c_in);
    st->entropy   = ema(st->entropy,   h_in);

    /* atualiza 7D state: s[i] = (s[i]*SPIRAL + s[(i+1)%7]) mod 65536 */
    for (int i=0; i<RAF_STATE_DIM; i++) {
        uint32_t next = (uint32_t)(((uint64_t)st->s[i] * 56755u) >> 16);
        next += st->s[(i+1)%RAF_STATE_DIM];
        st->s[i] = next & 0xFFFFu;
    }

    st->phase = ((uint32_t)cycle) % RAF_PERIOD;
    st->step++;

    uint32_t phi = (uint32_t)(((uint64_t)(65535u-st->entropy)*st->coherence)>>16);

    st->crc = 0;
    st->crc = _crc32(st, offsetof(raf_state_t,crc));

    return (jint)phi;
}

/* ── profileNative ────────────────────────────────────────────────────── */
/* Java: long profileNative(ByteBuffer out, int cap)
 * Escreve JSON de hw_profile em out sem malloc
 * Retorna: bytes escritos */
JNIEXPORT jlong JNICALL
Java_com_termux_rafaelia_RafaeliaCore_profileNative(
    JNIEnv *env, jclass cls,
    jobject out_buf, jint cap)
{
    (void)cls;
    char *out = (char*)(*env)->GetDirectBufferAddress(env, out_buf);
    jlong out_cap = (*env)->GetDirectBufferCapacity(env, out_buf);
    if (!out || cap < 64 || out_cap < cap) return -1;

    /* lê dados sem malloc — buffers na stack */
    char cpu_online[64]={0}, freq0[32]={0}, freq1[32]={0};
    char pg[16]={0};

    int fd;
    ssize_t n;

    fd=open("/sys/devices/system/cpu/online",O_RDONLY|O_CLOEXEC);
    if(fd>=0){n=read(fd,cpu_online,63);close(fd);if(n>0)cpu_online[n]=0;}

    fd=open("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq",O_RDONLY|O_CLOEXEC);
    if(fd>=0){n=read(fd,freq0,31);close(fd);if(n>0)freq0[n]=0;}

    fd=open("/sys/devices/system/cpu/cpu4/cpufreq/cpuinfo_max_freq",O_RDONLY|O_CLOEXEC);
    if(fd>=0){n=read(fd,freq1,31);close(fd);if(n>0)freq1[n]=0;}

    long pgsz = sysconf(_SC_PAGESIZE);
    if (pgsz>0) {
        int pi=0; long v=pgsz;
        char tmp[12]; int tl=0;
        while(v){tmp[tl++]=(char)('0'+v%10);v/=10;}
        for(int k=tl-1;k>=0;k--) pg[pi++]=tmp[k];
        pg[pi]=0;
    }

    /* remove newlines */
    for(char*p=cpu_online;*p;p++) if(*p=='\n')*p=0;
    for(char*p=freq0;*p;p++) if(*p=='\n')*p=0;
    for(char*p=freq1;*p;p++) if(*p=='\n')*p=0;

    /* monta JSON sem snprintf */
    const char *arch =
#if defined(__aarch64__)
        "arm64-v8a";
#elif defined(__arm__)
        "armeabi-v7a";
#else
        "generic";
#endif

#define HAS_NEON_STR (defined(HAS_NEON) ? "true" : "false")

    /* escreve JSON manualmente no buffer out */
    int pos = 0;
#define WSTR(s) do { \
    const char *_s=(s); \
    while(*_s && pos<cap-1) out[pos++]=*_s++; \
} while(0)

    WSTR("{\"abi\":\""); WSTR(arch);
    WSTR("\",\"cpus\":\""); WSTR(cpu_online);
    WSTR("\",\"freq0\":\""); WSTR(freq0);
    WSTR("\",\"freq1\":\""); WSTR(freq1);
    WSTR("\",\"page_sz\":\""); WSTR(pg);
#ifdef HAS_NEON
    WSTR("\",\"neon\":true");
#else
    WSTR("\",\"neon\":false");
#endif
    WSTR("}");
    if (pos < cap) out[pos] = 0;
#undef WSTR

    return (jlong)pos;
}

/* ── arenaSizeNative ──────────────────────────────────────────────────── */
/* Retorna bytes usados na arena JNI */
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_arenaSizeNative(
    JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    return (jint)g_jni_bump;
}

/* ── crc32Native ──────────────────────────────────────────────────────── */
/* Java: int crc32Native(ByteBuffer buf, int len) */
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_crc32Native(
    JNIEnv *env, jclass cls,
    jobject buf, jint len)
{
    (void)cls;
    uint8_t *p = (uint8_t*)(*env)->GetDirectBufferAddress(env, buf);
    jlong cap = (*env)->GetDirectBufferCapacity(env, buf);
    if (!p || len <= 0 || cap <= 0) return 0;
    if ((jlong)len > cap) return -1;
    return (jint)_crc32(p, (size_t)len);
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_sendBitrafInstructionNative(
    JNIEnv *env, jclass cls, jlong lo32, jint hi10)
{
    (void)env; (void)cls;
    pthread_once(&g_state_once, ensure_state_once);
    if (!g_state) return -1;
    pthread_mutex_lock(&g_state_mutex);
    uint64_t instr = (((uint64_t)(hi10 & 0x3FF)) << 32) | ((uint64_t)lo32 & 0xFFFFFFFFULL);
    g_state->s[0] ^= (uint32_t)(instr & 0xFFFFu);
    g_state->s[1] ^= (uint32_t)((instr >> 7) & 0xFFFFu);
    g_state->phase = (g_state->phase + 1u) % RAF_PERIOD;
    g_state->crc = 0;
    g_state->crc = _crc32(g_state, offsetof(raf_state_t, crc));
    pthread_mutex_unlock(&g_state_mutex);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_readOscillatorStateNative(
    JNIEnv *env, jclass cls, jobject outState, jint oscCount)
{
    (void)cls;
    uint32_t *out = (uint32_t*)(*env)->GetDirectBufferAddress(env, outState);
    jlong cap = (*env)->GetDirectBufferCapacity(env, outState);
    if (!out || oscCount <= 0 || cap <= 0) return -1;
    size_t required_words = (size_t)oscCount * 7u;
    if (required_words / 7u != (size_t)oscCount) return -2;
    size_t required_bytes = required_words * sizeof(uint32_t);
    if (required_bytes / sizeof(uint32_t) != required_words) return -2;
    if ((uint64_t)required_bytes > (uint64_t)cap) return -2;
    pthread_once(&g_state_once, ensure_state_once);
    if (!g_state) return -3;
    pthread_mutex_lock(&g_state_mutex);
    for (int i = 0; i < oscCount; i++) {
        for (int k = 0; k < 7; k++) {
            out[i*7 + k] = (g_state->s[k] + (uint32_t)(i*131 + k*17)) & 0xFFFFu;
        }
    }
    pthread_mutex_unlock(&g_state_mutex);
    return oscCount;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_debugStepNative(
    JNIEnv *env, jclass cls, jobject state_buf, jint cycle, jobject outDebug, jint cap)
{
    (void)cls;
    char *dbg = (char*)(*env)->GetDirectBufferAddress(env, outDebug);
    jlong dbg_cap = (*env)->GetDirectBufferCapacity(env, outDebug);
    if (!dbg || cap < 96 || dbg_cap < cap) return -1;
    jint phi = Java_com_termux_rafaelia_RafaeliaCore_stepNative(env, cls, state_buf, cycle);
    raf_state_t *st = (raf_state_t*)(*env)->GetDirectBufferAddress(env, state_buf);
    if (!st) return -2;
    int n = snprintf(dbg, (size_t)cap,
        "cycle=%d phi=%d s=[%u,%u,%u,%u,%u,%u,%u] C=%u H=%u phase=%u\n",
        cycle, phi, st->s[0], st->s[1], st->s[2], st->s[3], st->s[4], st->s[5], st->s[6],
        st->coherence, st->entropy, st->phase);
    if (n < 0) return -3;
    if (n >= cap) return -4;
    return phi;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_initVcpuSchedulerNative(
    JNIEnv *env, jclass cls, jint targetHz)
{
    (void)env; (void)cls;
    if (targetHz <= 0 || targetHz > 200000) return -1;
    pthread_mutex_lock(&g_sched_mutex);
    g_scheduler_initialized = 0;
    ensure_scheduler_locked((uint32_t)targetHz);
    pthread_mutex_unlock(&g_sched_mutex);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_stepVcpuNative(
    JNIEnv *env, jclass cls, jint vcpuId)
{
    (void)env; (void)cls;
    if (vcpuId < 0 || vcpuId >= RAF_VCPU) return -1;
    pthread_mutex_lock(&g_sched_mutex);
    ensure_scheduler_locked(10);
    uint64_t now = raf_clock_now_ns();
    if (raf_clock_should_tick(&g_clock, now)) raf_clock_mark_tick(&g_clock, now);
    int rc = raf_vcpu_step((uint32_t)vcpuId, now);
    pthread_mutex_unlock(&g_sched_mutex);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_stepAllVcpusNative(
    JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    pthread_mutex_lock(&g_sched_mutex);
    ensure_scheduler_locked(10);
    uint64_t now = raf_clock_now_ns();
    if (raf_clock_should_tick(&g_clock, now)) raf_clock_mark_tick(&g_clock, now);
    for (int i = 0; i < RAF_VCPU; i++) {
        if (raf_vcpu_step((uint32_t)i, now) != 0) { pthread_mutex_unlock(&g_sched_mutex); return -1; }
    }
    pthread_mutex_unlock(&g_sched_mutex);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_getVcpuTelemetryNative(
    JNIEnv *env, jclass cls, jobject out_buf, jint cap)
{
    (void)cls;
    char *out = (char*)(*env)->GetDirectBufferAddress(env, out_buf);
    jlong out_cap = (*env)->GetDirectBufferCapacity(env, out_buf);
    if (!out || cap < 512 || out_cap < cap) return -1;
    pthread_mutex_lock(&g_sched_mutex);
    ensure_scheduler_locked(10);
    raf_memory_layers_t m; raf_memory_layers_get(&m);
    int pos = snprintf(out, (size_t)cap,
        "{\"vcpu_count\":%d,\"target_hz\":%u,\"actual_hz_q16\":%u,\"period_ns\":%llu,\"jitter_ns\":%llu,\"missed_ticks\":%llu,\"total_ticks\":%llu,\"arena_used\":%u,\"page_size\":%u,\"cache_line\":%u,\"vcpus\":[",
        RAF_VCPU, g_target_hz, raf_clock_actual_hz_q16(&g_clock), (unsigned long long)g_clock.period_ns,
        (unsigned long long)g_clock.jitter_ns, (unsigned long long)g_clock.missed_ticks,
        (unsigned long long)g_clock.total_ticks, (unsigned)g_jni_bump, m.page_size, m.cache_line);
    for (int i = 0; i < RAF_VCPU && pos > 0 && pos < cap; i++) {
        const raf_vcpu_t* v = raf_vcpu_get((uint32_t)i);
        pos += snprintf(out + pos, (size_t)(cap - pos),
            "%s{\"id\":%d,\"phase\":%u,\"step\":%u,\"crc\":%u}",
            (i == 0) ? "" : ",", i, v->state.phase, v->state.step, v->crc);
    }
    if (pos > 0 && pos < cap) pos += snprintf(out + pos, (size_t)(cap - pos), "]}");
    pthread_mutex_unlock(&g_sched_mutex);
    return (pos > 0 && pos < cap) ? pos : -2;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_getClockProfileNative(
    JNIEnv *env, jclass cls, jobject out_buf, jint cap)
{
    (void)cls;
    char *out = (char*)(*env)->GetDirectBufferAddress(env, out_buf);
    jlong out_cap = (*env)->GetDirectBufferCapacity(env, out_buf);
    if (!out || cap < 96 || out_cap < cap) return -1;
    pthread_mutex_lock(&g_sched_mutex);
    ensure_scheduler_locked(10);
    int n = snprintf(out, (size_t)cap,
        "{\"target_hz\":%u,\"period_ns\":%llu,\"actual_delta_ns\":%llu,\"actual_hz_q16\":%u,\"jitter_ns\":%llu,\"missed_ticks\":%llu,\"total_ticks\":%llu}",
        g_clock.target_hz, (unsigned long long)g_clock.period_ns, (unsigned long long)g_clock.actual_delta_ns,
        raf_clock_actual_hz_q16(&g_clock), (unsigned long long)g_clock.jitter_ns,
        (unsigned long long)g_clock.missed_ticks, (unsigned long long)g_clock.total_ticks);
    pthread_mutex_unlock(&g_sched_mutex);
    if (n < 0) return -2;
    if (n >= cap) return -3;
    return n;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_initVcpuNative(JNIEnv *env, jclass cls, jint targetHz) {
    return Java_com_termux_rafaelia_RafaeliaCore_initVcpuSchedulerNative(env, cls, targetHz);
}
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_stepAllVcpuNative(JNIEnv *env, jclass cls) {
    return Java_com_termux_rafaelia_RafaeliaCore_stepAllVcpusNative(env, cls);
}
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_getClockNative(JNIEnv *env, jclass cls, jobject out_buf, jint cap) {
    return Java_com_termux_rafaelia_RafaeliaCore_getClockProfileNative(env, cls, out_buf, cap);
}
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_getVcpuMapNative(
    JNIEnv *env, jclass cls, jobject out_buf, jint cap)
{
    (void)cls;
    char *out = (char*)(*env)->GetDirectBufferAddress(env, out_buf);
    jlong out_cap = (*env)->GetDirectBufferCapacity(env, out_buf);
    if (!out || cap < 512 || out_cap < cap) return -1;
    pthread_mutex_lock(&g_sched_mutex);
    ensure_scheduler_locked(10);
    int pos = snprintf(out, (size_t)cap,
      "{\"vcpu_count\":%d,\"target_hz\":%u,\"period_ns\":%llu,\"actual_hz_q16\":%u,\"jitter_ns\":%llu,\"missed_ticks\":%llu,\"total_ticks\":%llu,\"vcpus\":[",
      RAF_VCPU, g_target_hz, (unsigned long long)g_clock.period_ns, raf_clock_actual_hz_q16(&g_clock),
      (unsigned long long)g_clock.jitter_ns, (unsigned long long)g_clock.missed_ticks, (unsigned long long)g_clock.total_ticks);
    for (int i = 0; i < RAF_VCPU && pos > 0 && pos < cap; i++) {
      const raf_vcpu_t* v = raf_vcpu_get((uint32_t)i);
      pos += snprintf(out + pos, (size_t)(cap - pos),
        "%s{\"id\":%d,\"phase\":%u,\"step\":%u,\"crc\":%u}",
        (i == 0) ? "" : ",", i, v->state.phase, v->state.step, v->crc);
    }
    if (pos > 0 && pos < cap) pos += snprintf(out + pos, (size_t)(cap - pos), "]}");
    pthread_mutex_unlock(&g_sched_mutex);
    return (pos > 0 && pos < cap) ? pos : -2;
}
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaCore_getMemoryLayersNative(
    JNIEnv *env, jclass cls, jobject out_buf, jint cap)
{
    (void)cls;
    char *out = (char*)(*env)->GetDirectBufferAddress(env, out_buf);
    jlong out_cap = (*env)->GetDirectBufferCapacity(env, out_buf);
    if (!out || cap < 128 || out_cap < cap) return -1;
    raf_memory_layers_t m; raf_memory_layers_get(&m);
    int n = snprintf(out, (size_t)cap,
      "{\"L1\":%u,\"L2_IN\":%u,\"L2_OUT\":%u,\"L2_JNI\":%u,\"L2_BM\":%u,\"arena_used\":%u,\"page_size\":%u,\"cache_line\":%u}",
      m.l1_state_cap,m.l2_in_buf,m.l2_out_buf,m.l2_jni_arena,m.l2_bm_arena,(unsigned)g_jni_bump,m.page_size,m.cache_line);
    return (n > 0 && n < cap) ? n : -2;
}
