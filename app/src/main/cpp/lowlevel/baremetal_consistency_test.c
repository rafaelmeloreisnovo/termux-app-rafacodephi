#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

float vop_dot(const float* a, const float* b, uint32_t n);
void vop_add(const float* a, const float* b, float* r, uint32_t n);
void* bmem_cpy(void* d, const void* s, size_t n);

/* Test-only stand-ins for ASM kernels (same contracts). */
float bm_dot_neon(const float* a, const float* b, uint32_t n) {
    float s = 0.0f;
    uint32_t i = 0;
    for (; i + 4 <= n; i += 4) {
        s += a[i] * b[i];
        s += a[i + 1] * b[i + 1];
        s += a[i + 2] * b[i + 2];
        s += a[i + 3] * b[i + 3];
    }
    for (; i < n; i++) s += a[i] * b[i];
    return s;
}

void bm_vadd_neon(const float* a, const float* b, float* r, uint32_t n) {
    uint32_t i = 0;
    for (; i + 4 <= n; i += 4) {
        r[i] = a[i] + b[i];
        r[i + 1] = a[i + 1] + b[i + 1];
        r[i + 2] = a[i + 2] + b[i + 2];
        r[i + 3] = a[i + 3] + b[i + 3];
    }
    for (; i < n; i++) r[i] = a[i] + b[i];
}

void* bm_memcpy_neon(void* d, const void* s, size_t n) {
    unsigned char* pd = (unsigned char*)d;
    const unsigned char* ps = (const unsigned char*)s;
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
        for (size_t j = 0; j < 16; j++) pd[i + j] = ps[i + j];
    }
    for (; i < n; i++) pd[i] = ps[i];
    return d;
}

static float ref_dot(const float* a, const float* b, uint32_t n) {
    float s = 0.0f;
    for (uint32_t i = 0; i < n; i++) s += a[i] * b[i];
    return s;
}

static int ref_add_eq(const float* a, const float* b, const float* r, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        if (r[i] != (a[i] + b[i])) return 0;
    }
    return 1;
}

static int ref_mem_eq(const unsigned char* a, const unsigned char* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

int main(void) {
    const uint32_t sizes[] = {1, 2, 3, 4, 5, 15, 16, 17, 63, 64, 65, 255, 1024, 1027};
    const size_t count = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t t = 0; t < count; t++) {
        uint32_t n = sizes[t];
        float* a = (float*)malloc(n * sizeof(float));
        float* b = (float*)malloc(n * sizeof(float));
        float* r = (float*)malloc(n * sizeof(float));
        if (!a || !b || !r) return 2;

        for (uint32_t i = 0; i < n; i++) {
            a[i] = (float)((int32_t)((i * 17u + 3u) % 97u) - 48);
            b[i] = (float)((int32_t)((i * 29u + 5u) % 89u) - 44);
        }

        vop_add(a, b, r, n);
        if (!ref_add_eq(a, b, r, n)) {
            fprintf(stderr, "vop_add mismatch at n=%u\n", n);
            return 3;
        }

        float got = vop_dot(a, b, n);
        float exp = ref_dot(a, b, n);
        if (got != exp) {
            fprintf(stderr, "vop_dot mismatch at n=%u got=%f exp=%f\n", n, got, exp);
            return 4;
        }

        free(a); free(b); free(r);
    }

    for (size_t t = 0; t < count; t++) {
        size_t n = sizes[t];
        unsigned char* srcb = (unsigned char*)malloc(n + 64);
        unsigned char* dstb = (unsigned char*)malloc(n + 64);
        if (!srcb || !dstb) return 5;

        unsigned char* src = srcb + (t % 13u);
        unsigned char* dst = dstb + (t % 11u);

        for (size_t i = 0; i < n; i++) src[i] = (unsigned char)((i * 7u + 13u) & 0xFFu);
        for (size_t i = 0; i < n; i++) dst[i] = 0u;

        bmem_cpy(dst, src, n);
        if (!ref_mem_eq(dst, src, n)) {
            fprintf(stderr, "bmem_cpy mismatch at n=%zu\n", n);
            return 6;
        }

        free(srcb); free(dstb);
    }

    puts("baremetal_consistency_test: ok");
    return 0;
}
