#include "raf_numbase.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int expect_int(const char *name, long long got, long long expected) {
    if (got == expected) return 0;
    fprintf(stderr, "%s: got=%lld expected=%lld\n", name, got, expected);
    return 1;
}

int main(void) {
    int failed = 0;
    char buf[96];

    if (!raf_to_base(LLONG_MIN, 10, buf, (int)sizeof(buf))) {
        fprintf(stderr, "raf_to_base(LLONG_MIN) returned NULL\n");
        failed++;
    } else if (strcmp(buf, "-9223372036854775808") != 0) {
        fprintf(stderr, "LLONG_MIN conversion mismatch: %s\n", buf);
        failed++;
    }

    failed += expect_int("fib(10)", raf_fibonacci(10), 55);
    failed += expect_int("trib(7)", raf_tribonacci(7), 13);
    failed += expect_int("pisano(7)", raf_pisano_period(7), 16);
    failed += expect_int("pisano(10)", raf_pisano_period(10), 60);
    failed += expect_int("pisano(14)", raf_pisano_period(14), 48);
    failed += expect_int("pisano(70)", raf_pisano_period(70), 240);

    if (fabs(raf_base_efficiency(10, 1) - 10.0) > 1e-12) {
        fprintf(stderr, "base efficiency for one digit must be 10\n");
        failed++;
    }

    if (failed != 0) return 1;
    puts("raf_numbase host invariants: PASS");
    return 0;
}
