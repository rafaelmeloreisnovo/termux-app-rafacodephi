/*
  RAFAELIA BENCH PACK v1.1 (C11) — anti-DCE edition
  - Fix: prevent dead-code elimination by consuming outputs into a volatile sink.
  - Also fix crc accumulator (use add, not XOR-cancel).

  Build:
    clang -O3 -DNDEBUG -std=c11 -Wall -Wextra -pedantic bench_rafaelia.c -o bench_rafaelia

  Optional I/O:
    ./bench_rafaelia --size 8388608 --rounds 20 --io ./bench.tmp
*/

#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile uint64_t g_sink = 0;

// -------------------- Timing --------------------
static inline uint64_t ns_now(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void print_rate(const char *label, uint64_t bytes, uint64_t ns) {
  double sec = (ns > 0) ? (double)ns / 1e9 : 0.0;
  double mb = (double)bytes / (1024.0 * 1024.0);
  double mbs = (sec > 0.0) ? (mb / sec) : 0.0;
  double ns_per_b = (bytes > 0) ? ((double)ns / (double)bytes) : 0.0;
  printf("%-18s  bytes=%12llu  time=%9.3f ms  %9.2f MB/s  %8.3f ns/B\n",
         label,
         (unsigned long long)bytes,
         (double)ns / 1e6,
         mbs,
         ns_per_b);
}

// -------------------- Alloc --------------------
static void *xmalloc(size_t n) {
  void *p = malloc(n);
  if (!p) { fprintf(stderr, "OOM allocating %zu bytes\n", n); exit(1); }
  return p;
}

// -------------------- Consume buffer (prevents optimization) --------------------
static uint64_t consume_bytes(const uint8_t *buf, size_t len) {
  // Sample ~64 bytes across buffer to create observable dependency
  if (len == 0) return 0;
  const size_t samples = 64;
  uint64_t acc = 0;
  for (size_t i = 0; i < samples; i++) {
    size_t idx = (i * 1315423911u) % len;
    acc = (acc * 131ull) + buf[idx];
  }
  return acc;
}

// -------------------- CRC32 (software) --------------------
static uint32_t crc32_table[256];
static int crc32_init_done = 0;

static void crc32_init(void) {
  if (crc32_init_done) return;
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t c = i;
    for (int j = 0; j < 8; j++) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
    crc32_table[i] = c;
  }
  crc32_init_done = 1;
}

static uint32_t crc32_sw(const uint8_t *buf, size_t len) {
  uint32_t c = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; i++) c = crc32_table[(c ^ buf[i]) & 0xFFu] ^ (c >> 8);
  return c ^ 0xFFFFFFFFu;
}

// -------------------- RAFAELIA kernel (hot loop) --------------------
static inline uint64_t rotl64(uint64_t x, unsigned n) {
  return (x << n) | (x >> (64u - n));
}

static inline void rafaelia_step(uint64_t m[4]) {
  uint64_t flux = rotl64(m[0], 13u) ^ rotl64(m[0], 20u);
  m[1] = flux;
  if (flux & m[3]) m[0] += ~flux;
  else            m[0] += (flux >> 1);
  m[0] ^= flux;
}

static uint64_t rafaelia_generate(uint64_t seed, uint64_t cycles, uint8_t *out, size_t out_cap) {
  uint64_t m[4] = { seed, 0u, cycles, 137u };
  uint64_t produced = 0;
  for (uint64_t i = 0; i < cycles; i++) {
    rafaelia_step(m);
    uint8_t ch = (uint8_t)(33u + ((m[0] ^ m[1]) % 93u));
    if (produced < out_cap) out[produced] = ch;
    produced++;
  }
  // Make kernel state observable
  g_sink ^= (m[0] + (m[1] << 1));
  return produced;
}

// -------------------- BitStack append-only microbench --------------------
typedef struct {
  uint32_t source_id;
  uint32_t flags;
  uint64_t t_seq;
  uint64_t crc32;
  uint64_t h64;
} bitrec_t;

static uint64_t xorshift64(uint64_t *s) {
  uint64_t x = *s;
  x ^= x << 13; x ^= x >> 7; x ^= x << 17;
  *s = x;
  return x;
}

static void bitstack_fill(bitrec_t *arr, size_t n, uint64_t seed) {
  uint64_t s = seed;
  for (size_t i = 0; i < n; i++) {
    uint64_t r = xorshift64(&s);
    arr[i].source_id = (uint32_t)(r & 0xFFFFu);
    arr[i].flags     = (uint32_t)((r >> 16) & 0xFFFFu);
    arr[i].t_seq     = (uint64_t)i;
    arr[i].crc32     = (uint32_t)r;
    arr[i].h64       = (r * 0x9E3779B97F4A7C15ull);
  }
  // Observe a few elements so compiler can't drop work
  if (n) g_sink ^= (arr[n/2].h64 + arr[n-1].crc32);
}

// -------------------- Optional file I/O bench --------------------
static void bench_io(const char *path, const uint8_t *buf, size_t len, int rounds) {
  uint64_t t0 = ns_now();
  for (int i = 0; i < rounds; i++) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen(wb)"); return; }
    size_t w = fwrite(buf, 1, len, f);
    if (w != len) { perror("fwrite"); fclose(f); return; }
    fflush(f);
    fclose(f);
  }
  uint64_t t1 = ns_now();
  print_rate("io_write", (uint64_t)len * (uint64_t)rounds, t1 - t0);

  uint8_t *tmp = (uint8_t*)xmalloc(len);
  uint64_t t2 = ns_now();
  for (int i = 0; i < rounds; i++) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror("fopen(rb)"); free(tmp); return; }
    size_t r = fread(tmp, 1, len, f);
    if (r != len) { perror("fread"); fclose(f); free(tmp); return; }
    fclose(f);
    g_sink ^= consume_bytes(tmp, len);
  }
  uint64_t t3 = ns_now();
  print_rate("io_read", (uint64_t)len * (uint64_t)rounds, t3 - t2);
  free(tmp);
}

static void usage(const char *argv0) {
  printf("Usage: %s [--size BYTES] [--rounds N] [--io PATH]\n", argv0);
  printf("  --size   bytes buffer size (default: 16777216 = 16 MiB)\n");
  printf("  --rounds iterations per test (default: 50)\n");
  printf("  --io     optional file path to measure sequential IO (e.g. ./bench.tmp)\n");
}

int main(int argc, char **argv) {
  size_t size = 16u * 1024u * 1024u;
  int rounds = 50;
  const char *io_path = NULL;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
    else if (!strcmp(argv[i], "--size") && i + 1 < argc) { size = (size_t)strtoull(argv[++i], NULL, 10); }
    else if (!strcmp(argv[i], "--rounds") && i + 1 < argc) { rounds = atoi(argv[++i]); }
    else if (!strcmp(argv[i], "--io") && i + 1 < argc) { io_path = argv[++i]; }
    else { fprintf(stderr, "Unknown arg: %s\n", argv[i]); usage(argv[0]); return 1; }
  }

  printf("RAFAELIA_BENCH v1.1  size=%zu  rounds=%d\n", size, rounds);
  printf("------------------------------------------------------------\n");

  crc32_init();

  uint8_t *a = (uint8_t*)xmalloc(size);
  uint8_t *b = (uint8_t*)xmalloc(size);

  for (size_t i = 0; i < size; i++) a[i] = (uint8_t)(i * 131u + 17u);
  memset(b, 0, size);

  // 1) memcpy
  uint64_t t0 = ns_now();
  for (int i = 0; i < rounds; i++) memcpy(b, a, size);
  uint64_t t1 = ns_now();
  g_sink ^= consume_bytes(b, size);
  print_rate("memcpy", (uint64_t)size * (uint64_t)rounds, t1 - t0);

  // 2) CRC32
  uint64_t crc_acc = 0;
  uint64_t t2 = ns_now();
  for (int i = 0; i < rounds; i++) crc_acc += (uint64_t)crc32_sw(a, size);
  uint64_t t3 = ns_now();
  g_sink ^= crc_acc;
  print_rate("crc32_sw", (uint64_t)size * (uint64_t)rounds, t3 - t2);
  printf("crc32_acc=%016llx\n", (unsigned long long)crc_acc);

  // 3) RAFAELIA kernel generation (no stdout)
  uint64_t cycles = (uint64_t)size;
  uint64_t produced = 0;
  uint64_t t4 = ns_now();
  for (int i = 0; i < rounds; i++) produced += rafaelia_generate(0x9E3779B97F4A7C15ull ^ (uint64_t)i, cycles, b, size);
  uint64_t t5 = ns_now();
  g_sink ^= consume_bytes(b, size);
  print_rate("rafaelia_gen", produced, t5 - t4);

  // 4) BitStack fill
  size_t nrec = size / sizeof(bitrec_t);
  if (nrec < 1) nrec = 1;
  bitrec_t *recs = (bitrec_t*)xmalloc(nrec * sizeof(bitrec_t));

  uint64_t t6 = ns_now();
  for (int i = 0; i < rounds; i++) bitstack_fill(recs, nrec, 0xC0FFEEull + (uint64_t)i);
  uint64_t t7 = ns_now();
  uint64_t total_records = (uint64_t)nrec * (uint64_t)rounds;
  double sec = (double)(t7 - t6) / 1e9;
  double rps = (sec > 0.0) ? ((double)total_records / sec) : 0.0;
  printf("%-18s  records=%12llu  time=%9.3f ms  %9.2f rec/s  %8.3f ns/rec\n",
         "bitstack_fill",
         (unsigned long long)total_records,
         (double)(t7 - t6) / 1e6,
         rps,
         (total_records > 0) ? ((double)(t7 - t6) / (double)total_records) : 0.0);

  // Optional I/O
  if (io_path) {
    printf("------------------------------------------------------------\n");
    printf("I/O path: %s\n", io_path);
    bench_io(io_path, a, size, 5);
  }

  free(recs);
  free(a);
  free(b);

  printf("------------------------------------------------------------\n");
  printf("sink=%016llx\n", (unsigned long long)g_sink);
  printf("Done.\n");
  return 0;
}
