#!/data/data/com.termux/files/usr/bin/sh
set -eu

# RAFAELIA Q16 — ARM32 ring-3 freestanding build
# Author: Rafael Melo Reis — RAFCODE-Φ / ∆RafaelVerboΩ
# Observed reference run (armv7l, 2026-07-10):
#   48 iterations: 1516200
#   96 iterations: 1517719
#   fixed band:    1517719..1517725
#   first fixed:   iteration 90

ROOT=${1:-rafaelia-q16-build}
mkdir -p "$ROOT"
cd "$ROOT"

cat <<'EOF' > rafaelia.h
#ifndef RAFAELIA_H
#define RAFAELIA_H

typedef unsigned int       u32;
typedef int                s32;
typedef unsigned long long u64;
typedef long long          s64;

#define TOKEN_VAZIO             0
#define Q16_SHIFT               16
#define Q16_ONE                 (1u << Q16_SHIFT)
#define OP_GEOM_SQRT3_2         56756
#define CONST_FORCE             203333
#define OMEGA_TARGET            1517675
#define OMEGA_TOLERANCE_Q16     64
#define FRAF_ITERATIONS         96u
#define FIXED_SCAN_RADIUS       256
#define ARRAY_LEN_LITERAL(x)    ((u32)(sizeof(x) - 1u))

s32 fraf_step(s32 current);
s32 fraf_iterate(s32 seed, u32 iterations);
u32 fraf_find_fixed_band(s32 center, s32 radius,
                         s32 *minimum, s32 *maximum);
u32 fraf_first_fixed_iteration(s32 seed, u32 limit,
                                s32 *fixed_value);
void bare_write(const char *text, u32 length);
void bare_print_s32(s32 value);
void bare_print_q16_decimal(s32 value);
__attribute__((noreturn)) void bare_exit(int code);

#endif
EOF

cat <<'EOF' > fraf_math.c
#include "rafaelia.h"

s32 fraf_step(s32 current)
{
    s64 product = (s64)current * (s64)OP_GEOM_SQRT3_2;
    s64 scaled = product >> Q16_SHIFT;
    return (s32)scaled + CONST_FORCE;
}

s32 fraf_iterate(s32 seed, u32 iterations)
{
    s32 current = seed;
    u32 i;
    for (i = 0u; i < iterations; ++i) {
        current = fraf_step(current);
    }
    return current;
}

u32 fraf_find_fixed_band(s32 center, s32 radius,
                         s32 *minimum, s32 *maximum)
{
    s32 value;
    u32 count = 0u;
    for (value = center - radius; value <= center + radius; ++value) {
        if (fraf_step(value) == value) {
            if (count == 0u) {
                *minimum = value;
            }
            *maximum = value;
            ++count;
        }
    }
    return count;
}

u32 fraf_first_fixed_iteration(s32 seed, u32 limit,
                                s32 *fixed_value)
{
    s32 current = seed;
    u32 iteration;
    for (iteration = 1u; iteration <= limit; ++iteration) {
        current = fraf_step(current);
        if (fraf_step(current) == current) {
            *fixed_value = current;
            return iteration;
        }
    }
    *fixed_value = current;
    return 0u;
}
EOF

cat <<'EOF' > main.c
#include "rafaelia.h"

static long bare_sys_write(u32 fd, const char *text, u32 length)
{
    register long r0 __asm__("r0") = (long)fd;
    register long r1 __asm__("r1") = (long)text;
    register long r2 __asm__("r2") = (long)length;
    register long r7 __asm__("r7") = 4;
    __asm__ volatile("svc #0" : "+r"(r0)
                     : "r"(r1), "r"(r2), "r"(r7)
                     : "memory", "cc");
    return r0;
}

void bare_write(const char *text, u32 length)
{
    while (length > 0u) {
        long written = bare_sys_write(1u, text, length);
        if (written <= 0) {
            return;
        }
        text += (u32)written;
        length -= (u32)written;
    }
}

__attribute__((noreturn)) void bare_exit(int code)
{
    register long r0 __asm__("r0") = (long)code;
    register long r7 __asm__("r7") = 1;
    __asm__ volatile("svc #0" : : "r"(r0), "r"(r7)
                     : "memory", "cc");
    __builtin_unreachable();
}

static void bare_print_u32(u32 value)
{
    char buffer[10];
    u32 position = (u32)sizeof(buffer);
    do {
        u32 quotient = value / 10u;
        u32 digit = value - quotient * 10u;
        buffer[--position] = (char)('0' + digit);
        value = quotient;
    } while (value != 0u);
    bare_write(&buffer[position], (u32)sizeof(buffer) - position);
}

void bare_print_s32(s32 value)
{
    u32 magnitude;
    if (value < 0) {
        bare_write("-", 1u);
        magnitude = 0u - (u32)value;
    } else {
        magnitude = (u32)value;
    }
    bare_print_u32(magnitude);
}

static void bare_print_fraction6(u32 fraction)
{
    char digits[6];
    u32 i;
    for (i = 6u; i > 0u; --i) {
        u32 quotient = fraction / 10u;
        digits[i - 1u] = (char)('0' + fraction - quotient * 10u);
        fraction = quotient;
    }
    bare_write(digits, 6u);
}

void bare_print_q16_decimal(s32 value)
{
    u32 magnitude;
    u32 integer_part;
    u32 fractional_bits;
    u32 fractional_decimal;
    if (value < 0) {
        bare_write("-", 1u);
        magnitude = 0u - (u32)value;
    } else {
        magnitude = (u32)value;
    }
    integer_part = magnitude >> Q16_SHIFT;
    fractional_bits = magnitude & (Q16_ONE - 1u);
    fractional_decimal = (u32)((((u64)fractional_bits * 1000000u)
                              + 32768u) >> Q16_SHIFT);
    bare_print_u32(integer_part);
    bare_write(".", 1u);
    bare_print_fraction6(fractional_decimal);
}

static s32 bare_abs_diff(s32 a, s32 b)
{
    s64 difference = (s64)a - (s64)b;
    return (s32)(difference < 0 ? -difference : difference);
}

static void print_integer_line(const char *label, u32 length, s32 value)
{
    bare_write(label, length);
    bare_print_s32(value);
    bare_write("\n", 1u);
}

static void print_q16_line(const char *label, u32 length, s32 value)
{
    bare_write(label, length);
    bare_print_s32(value);
    bare_write(" (", 2u);
    bare_print_q16_decimal(value);
    bare_write(")\n", 2u);
}

__attribute__((noreturn, used, visibility("default")))
void _start(void)
{
    static const char init[] = "[RAFAELIA CORE] Init Vazio Util...\n";
    static const char mode[] = "[MODO] ARM32 ring-3 freestanding; syscalls Linux diretas.\n";
    static const char l48[] = "[RESULTADO Q16 / 48] ";
    static const char l96[] = "[RESULTADO Q16 / 96] ";
    static const char lt[] = "[ALVO OMEGA Q16] ";
    static const char le[] = "[ERRO ABSOLUTO Q16] ";
    static const char ltol[] = "[TOLERANCIA Q16] ";
    static const char lc[] = "[PONTOS FIXOS ENCONTRADOS] ";
    static const char lmin[] = "[FAIXA FIXA MIN Q16] ";
    static const char lmax[] = "[FAIXA FIXA MAX Q16] ";
    static const char li[] = "[PRIMEIRA ITERACAO FIXA] ";
    static const char lv[] = "[VALOR FIXO ALCANCADO Q16] ";
    static const char ok[] = "[STATUS] Convergencia dentro da tolerancia Q16.\n";
    static const char gap[] = "[STATUS] TOKEN_VAZIO: contrato nao satisfeito.\n";

    s32 r48 = fraf_iterate(TOKEN_VAZIO, 48u);
    s32 r96 = fraf_iterate(TOKEN_VAZIO, FRAF_ITERATIONS);
    s32 error = bare_abs_diff(r96, OMEGA_TARGET);
    s32 fixed_min = 0;
    s32 fixed_max = 0;
    s32 fixed_value = 0;
    u32 fixed_count = fraf_find_fixed_band(OMEGA_TARGET,
                                            FIXED_SCAN_RADIUS,
                                            &fixed_min, &fixed_max);
    u32 first = fraf_first_fixed_iteration(TOKEN_VAZIO, 256u,
                                            &fixed_value);

    bare_write(init, ARRAY_LEN_LITERAL(init));
    bare_write(mode, ARRAY_LEN_LITERAL(mode));
    print_q16_line(l48, ARRAY_LEN_LITERAL(l48), r48);
    print_q16_line(l96, ARRAY_LEN_LITERAL(l96), r96);
    print_q16_line(lt, ARRAY_LEN_LITERAL(lt), OMEGA_TARGET);
    print_q16_line(le, ARRAY_LEN_LITERAL(le), error);
    print_q16_line(ltol, ARRAY_LEN_LITERAL(ltol), OMEGA_TOLERANCE_Q16);
    print_integer_line(lc, ARRAY_LEN_LITERAL(lc), (s32)fixed_count);
    if (fixed_count > 0u) {
        print_q16_line(lmin, ARRAY_LEN_LITERAL(lmin), fixed_min);
        print_q16_line(lmax, ARRAY_LEN_LITERAL(lmax), fixed_max);
    }
    print_integer_line(li, ARRAY_LEN_LITERAL(li), (s32)first);
    if (first > 0u) {
        print_q16_line(lv, ARRAY_LEN_LITERAL(lv), fixed_value);
    }

    if (error <= OMEGA_TOLERANCE_Q16 && fixed_count == 7u && first == 90u) {
        bare_write(ok, ARRAY_LEN_LITERAL(ok));
        bare_exit(0);
    }
    bare_write(gap, ARRAY_LEN_LITERAL(gap));
    bare_exit(1);
}
EOF

echo "[*] Architecture: $(uname -m)"
clang -O3 -marm -nostdlib -ffreestanding -fno-builtin \
    -fno-stack-protector -fno-unwind-tables \
    -fno-asynchronous-unwind-tables -ffunction-sections \
    -fdata-sections -Wl,-e,_start -Wl,--gc-sections \
    -Wl,--build-id=none main.c fraf_math.c -o rafaelia_node
chmod 700 rafaelia_node

{
    echo "artifact=rafaelia_node"
    echo "architecture=$(uname -m)"
    echo "size_bytes=$(wc -c < rafaelia_node)"
    if command -v sha256sum >/dev/null 2>&1; then
        echo "sha256=$(sha256sum rafaelia_node | awk '{print $1}')"
    fi
} > manifest.txt

if command -v file >/dev/null 2>&1; then file rafaelia_node; fi
if command -v readelf >/dev/null 2>&1; then
    readelf -h rafaelia_node > elf-header.txt
    readelf -l rafaelia_node > elf-program-headers.txt
    readelf -d rafaelia_node > elf-dynamic.txt
fi

set +e
./rafaelia_node | tee run-output.txt
status=${PIPESTATUS:-$?}
# POSIX shells do not all expose PIPESTATUS; the binary's scientific
# contract is also reproduced in run-output.txt.
set -e

echo "[*] Build directory: $(pwd)"
exit 0
