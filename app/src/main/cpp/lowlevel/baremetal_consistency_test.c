#include <stdint.h>
#include <stddef.h>

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
    static float a_buf[2048], b_buf[2048], r_buf[2048];
    static unsigned char mem_buf[4096];

    const uint32_t sizes[] = {1, 2, 3, 4, 5, 15, 16, 17, 63, 64, 65, 255, 1024};
    const size_t count = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t t = 0; t < count; t++) {
        uint32_t n = sizes[t];
        if (n > 2048) break;
        float* a = a_buf;
        float* b = b_buf;
        float* r = r_buf;

        for (uint32_t i = 0; i < n; i++) {
            a[i] = (float)((int32_t)((i * 17u + 3u) % 97u) - 48);
            b[i] = (float)((int32_t)((i * 29u + 5u) % 89u) - 44);
        }

        vop_add(a, b, r, n);
        if (!ref_add_eq(a, b, r, n)) {
            return 3;
        }

        float got = vop_dot(a, b, n);
        float exp = ref_dot(a, b, n);
        if (got != exp) {
            return 4;
        }
    }

    for (size_t t = 0; t < count; t++) {
        size_t n = sizes[t];
        if (n + 64 > sizeof(mem_buf)) break;
        unsigned char* srcb = mem_buf;
        unsigned char* dstb = mem_buf + 2048;

        unsigned char* src = srcb + (t % 13u);
        unsigned char* dst = dstb + (t % 11u);

        for (size_t i = 0; i < n; i++) src[i] = (unsigned char)((i * 7u + 13u) & 0xFFu);
        for (size_t i = 0; i < n; i++) dst[i] = 0u;

        bmem_cpy(dst, src, n);
        if (!ref_mem_eq(dst, src, n)) {
            return 6;
        }
    }

    return 0;
}
