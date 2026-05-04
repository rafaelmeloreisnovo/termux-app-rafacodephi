#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * raf_bench_suite.c
 * Microbench mínimo para baseline RAFAELIA:
 *  - throughput CRC32C
 *  - custo de update EMA (alpha=0.25)
 *  - estimativa de entropia milli
 *
 * Build local (host):
 *   cc -O3 -std=c11 raf_bench_suite.c -o raf_bench_suite
 */

static uint32_t crc32c_u8(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    const uint32_t poly = 0x82F63B78u;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (poly & mask);
        }
    }
    return ~crc;
}

static int32_t ema_q16_16(int32_t prev, int32_t in) {
    // alpha=0.25 => prev*0.75 + in*0.25
    return (int32_t)(((int64_t)prev * 3 + (int64_t)in) / 4);
}

static int entropy_milli(const uint8_t *data, size_t len) {
    if (len == 0) return 0;
    int seen[256] = {0};
    int unique = 0;
    int transitions = 0;
    uint8_t prev = data[0];
    seen[prev] = 1;
    unique = 1;
    for (size_t i = 1; i < len; ++i) {
        if (!seen[data[i]]) {
            seen[data[i]] = 1;
            unique++;
        }
        if (data[i] != prev) transitions++;
        prev = data[i];
    }
    int a = (unique * 6000) / 256;
    int b = (len > 1) ? (transitions * 2000) / (int)(len - 1) : 0;
    return a + b;
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ((double)ts.tv_nsec / 1e9);
}

int main(void) {
    enum { N = 1 << 20, ITERS = 128 };
    static uint8_t buf[N];
    for (int i = 0; i < N; ++i) buf[i] = (uint8_t)((i * 131) ^ (i >> 3));

    double t0 = now_sec();
    volatile uint32_t acc = 0;
    for (int i = 0; i < ITERS; ++i) acc ^= crc32c_u8(buf, N);
    double t1 = now_sec();
    double mb = (double)N * (double)ITERS / (1024.0 * 1024.0);
    double mbps = mb / (t1 - t0);

    int32_t c = 0x00010000;
    int32_t in = 0x00018000;
    double t2 = now_sec();
    for (int i = 0; i < (ITERS * N); ++i) c = ema_q16_16(c, in);
    double t3 = now_sec();

    int ent = entropy_milli(buf, N);

    printf("{\"crc32c\":%u,\"crc_mibps\":%.2f,\"ema_final\":%d,\"ema_ops_sec\":%.2f,\"entropy_milli\":%d}\n",
           acc, mbps, c, (double)(ITERS * N) / (t3 - t2), ent);
    return 0;
}
