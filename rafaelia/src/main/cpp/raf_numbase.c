#include "raf_numbase.h"
#include "raf_compile_contract.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>

/* =========================================================================
 * Base Conversion
 * ========================================================================= */

char *raf_to_base(long long n, int base, char *buf, int buf_len) {
    if (!buf || buf_len < 2 || base < 2 || base > 36) return NULL;
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[68];
    int pos = 0, neg = 0;
    unsigned long long magnitude;

    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return buf; }

    /* Avoid signed overflow for LLONG_MIN. */
    if (n < 0) {
        neg = 1;
        magnitude = 0ULL - (unsigned long long)n;
    } else {
        magnitude = (unsigned long long)n;
    }

    while (magnitude > 0ULL && pos < 67) {
        tmp[pos++] = digits[(int)(magnitude % (unsigned int)base)];
        magnitude /= (unsigned int)base;
    }

    int out = 0;
    if (neg && out < buf_len - 1) buf[out++] = '-';
    for (int i = pos - 1; i >= 0 && out < buf_len - 1; i--) buf[out++] = tmp[i];
    buf[out] = '\0';
    return buf;
}

long long raf_from_base(const char *s, int base) {
    if (!s || base < 2 || base > 36) return 0;
    long long r = 0;
    int neg = 0, i = 0;
    if (s[0] == '-') { neg = 1; i = 1; }
    for (; s[i]; i++) {
        int d;
        if      (s[i] >= '0' && s[i] <= '9') d = s[i] - '0';
        else if (s[i] >= 'a' && s[i] <= 'z') d = s[i] - 'a' + 10;
        else if (s[i] >= 'A' && s[i] <= 'Z') d = s[i] - 'A' + 10;
        else break;
        if (d >= base) break;
        r = r * base + d;
    }
    return neg ? -r : r;
}

/* =========================================================================
 * Sequences
 * ========================================================================= */

long long raf_fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    long long a = 0, b = 1;
    for (int i = 2; i <= n; i++) { long long c = a + b; a = b; b = c; }
    return b;
}

/* T(0)=0, T(1)=0, T(2)=1, T(3)=1, T(4)=2, T(5)=4, T(6)=7, T(7)=13 */
long long raf_tribonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 0;
    if (n == 2) return 1;
    long long a = 0, b = 0, c = 1;
    for (int i = 3; i <= n; i++) { long long d = a + b + c; a = b; b = c; c = d; }
    return c;
}

static RAF_PURE int raf_is_prime(long long n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    /* i <= n / i avoids overflow from i*i near LLONG_MAX. */
    for (long long i = 3; i <= n / i; i += 2) if (n % i == 0) return 0;
    return 1;
}

static RAF_PURE long long raf_next_prime(long long n) {
    if (n < 2) return 2;
    long long p = (n % 2 == 0) ? n + 1 : n + 2;
    while (!raf_is_prime(p)) p += 2;
    return p;
}

/* P(0)=2, P(1)=3, P(n) = next prime >= P(n-2)+P(n-1) */
long long raf_primonacci(int n) {
    if (n <= 0) return 2;
    if (n == 1) return 3;
    long long a = 2, b = 3;
    for (int i = 2; i <= n; i++) {
        long long sum = a + b;
        long long p = raf_is_prime(sum) ? sum : raf_next_prime(sum - 1);
        a = b; b = p;
    }
    return b;
}

long long raf_seq_mod(int type, int n, int m) {
    if (m <= 0) return 0;
    long long v;
    switch (type) {
        case 0: v = raf_fibonacci(n);  break;
        case 1: v = raf_tribonacci(n); break;
        case 2: v = raf_primonacci(n); break;
        default: return 0;
    }
    return ((v % m) + m) % m;
}

/* =========================================================================
 * Pisano Period
 * Fibonacci mod m is periodic; P(10)=60, P(7)=16, P(14)=24, P(70)=120
 * ========================================================================= */

int raf_pisano_period(int m) {
    if (m <= 1) return 1;
    long long a = 0, b = 1;
    for (int i = 0; i < 6 * m; i++) {
        long long c = (a + b) % m;
        a = b; b = c;
        if (a == 0 && b == 1) return i + 1;
    }
    return 0;
}

/* =========================================================================
 * Base Efficiency
 * Radix economy = ceil(log_base(n_max)) * base — lower is better.
 * We return the reciprocal scaled so higher = more efficient.
 * ========================================================================= */

double raf_base_efficiency(int base, long long n_max) {
    if (base < 2 || n_max <= 0) return 0.0;
    double digits = ceil(log((double)n_max) / log((double)base));
    if (digits < 1.0) digits = 1.0;
    return digits * base; /* radix economy — caller compares across bases */
}

/* =========================================================================
 * JSON helpers (no heap allocation)
 * ========================================================================= */

static void jappend(char *buf, int *pos, int buf_len, const char *s) {
    while (*s && *pos < buf_len - 1) buf[(*pos)++] = *s++;
    if (*pos < buf_len) buf[*pos] = '\0';
}

static void jprintf(char *buf, int *pos, int buf_len, const char *fmt, ...) {
    char tmp[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    jappend(buf, pos, buf_len, tmp);
}

/* =========================================================================
 * Prime Fluid Graph
 * Nodes = primes; edge (p1,p2) when (p2-p1) % mod == 0.
 * "Fluid" weight = 1/diff (closer primes → stronger coupling).
 * ========================================================================= */

int raf_prime_fluid_graph(const int *primes, int n_primes, int mod,
                           char *buf, int buf_len) {
    if (!primes || n_primes <= 0 || !buf || buf_len < 64) return -1;
    int pos = 0;
    jappend(buf, &pos, buf_len, "{\"nodes\":[");
    for (int i = 0; i < n_primes; i++) {
        if (i > 0) jappend(buf, &pos, buf_len, ",");
        jprintf(buf, &pos, buf_len, "%d", primes[i]);
    }
    jprintf(buf, &pos, buf_len, "],\"mod\":%d,\"edges\":[", mod);
    int first = 1;
    for (int i = 0; i < n_primes; i++) {
        for (int j = i + 1; j < n_primes; j++) {
            int diff = primes[j] - primes[i];
            if (mod > 0 && diff % mod == 0) {
                if (!first) jappend(buf, &pos, buf_len, ",");
                jprintf(buf, &pos, buf_len,
                    "{\"from\":%d,\"to\":%d,\"diff\":%d,\"weight\":%.6f}",
                    primes[i], primes[j], diff, 1.0 / diff);
                first = 0;
            }
        }
    }
    jappend(buf, &pos, buf_len, "]}");
    return pos;
}

/* =========================================================================
 * Special Numbers Analysis
 * ========================================================================= */

int raf_analyze_special(const long long *nums, int n_nums,
                         const int *bases, int n_bases,
                         char *buf, int buf_len) {
    if (!nums || n_nums <= 0 || !buf || buf_len < 64) return -1;
    if (n_bases > 0 && !bases) return -1;
    static const int MODS[] = {7, 10, 14, 70};
    char tmp[68];
    int pos = 0;
    jappend(buf, &pos, buf_len, "[");
    for (int i = 0; i < n_nums; i++) {
        if (i > 0) jappend(buf, &pos, buf_len, ",");
        long long n = nums[i];
        jprintf(buf, &pos, buf_len, "{\"n\":%lld,\"bases\":{", n);
        for (int b = 0; b < n_bases; b++) {
            if (b > 0) jappend(buf, &pos, buf_len, ",");
            if (!raf_to_base(n, bases[b], tmp, (int)sizeof(tmp))) {
                tmp[0] = '?';
                tmp[1] = '\0';
            }
            jprintf(buf, &pos, buf_len, "\"%d\":\"%s\"", bases[b], tmp);
        }
        jappend(buf, &pos, buf_len, "},\"mod\":{");
        for (int m = 0; m < 4; m++) {
            if (m > 0) jappend(buf, &pos, buf_len, ",");
            long long r = ((n % MODS[m]) + MODS[m]) % MODS[m];
            jprintf(buf, &pos, buf_len, "\"%d\":%lld", MODS[m], r);
        }

        /* Single linear pass instead of recalculating Fibonacci from zero. */
        int fib_idx = -1;
        long long fib_a = 0;
        long long fib_b = 1;
        for (int k = 0; k <= 86; k++) {
            long long f = fib_a;
            if (f == n) { fib_idx = k; break; }
            if (f > n) break;
            long long next = fib_a + fib_b;
            fib_a = fib_b;
            fib_b = next;
        }
        jprintf(buf, &pos, buf_len, "},\"fib_index\":%d}", fib_idx);
    }
    jappend(buf, &pos, buf_len, "]");
    return pos;
}

/* =========================================================================
 * Zero Curve Dual
 * Z/aZ and Z/bZ: elements 0..a-1 and 0..b-1 arranged as circles.
 * They "coincide at zero" simultaneously at multiples of LCM(a,b).
 * Pisano period shows how Fibonacci orbits in each modular ring.
 * ========================================================================= */

int raf_zero_curve_dual(int base_a, int base_b, char *buf, int buf_len) {
    if (base_a < 2 || base_b < 2 || !buf || buf_len < 64) return -1;

    /* GCD/LCM via Euclidean */
    int g = base_a, r = base_b;
    while (r) { int t = r; r = g % r; g = t; }
    int lcm = (base_a / g) * base_b;

    int pos = 0;
    jprintf(buf, &pos, buf_len,
        "{\"base_a\":%d,\"base_b\":%d,\"lcm\":%d,"
        "\"pisano_a\":%d,\"pisano_b\":%d,",
        base_a, base_b, lcm,
        raf_pisano_period(base_a), raf_pisano_period(base_b));

    jappend(buf, &pos, buf_len, "\"ring_a\":[");
    for (int i = 0; i < base_a; i++) {
        if (i > 0) jappend(buf, &pos, buf_len, ",");
        jprintf(buf, &pos, buf_len, "%d", i);
    }
    jappend(buf, &pos, buf_len, "],\"ring_b\":[");
    for (int i = 0; i < base_b; i++) {
        if (i > 0) jappend(buf, &pos, buf_len, ",");
        jprintf(buf, &pos, buf_len, "%d", i);
    }
    jappend(buf, &pos, buf_len, "],\"coincidences\":[");
    int first = 1;
    for (int i = 0; i <= lcm; i++) {
        if (i % base_a == 0 && i % base_b == 0) {
            if (!first) jappend(buf, &pos, buf_len, ",");
            jprintf(buf, &pos, buf_len, "%d", i);
            first = 0;
        }
    }
    jappend(buf, &pos, buf_len, "]}");
    return pos;
}
