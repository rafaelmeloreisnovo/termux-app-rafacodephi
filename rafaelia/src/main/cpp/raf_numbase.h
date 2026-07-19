#ifndef RAF_NUMBASE_H
#define RAF_NUMBASE_H

#include <stdint.h>

/* Base conversion (base 2–36). buf must be >= 68 bytes. */
char *raf_to_base(long long n, int base, char *buf, int buf_len);
long long raf_from_base(const char *s, int base);

/* Integer sequences (0-indexed).
 * fibonacci: 0,1,1,2,3,5,8,13,21,34,55,89,144,...
 * tribonacci: 0,0,1,1,2,4,7,13,24,...
 * primonacci: 2,3,5,11,17,29,...  (next prime >= prev+prev2) */
long long raf_fibonacci(int n);
long long raf_tribonacci(int n);
long long raf_primonacci(int n);

/* Sequence value mod m. type: 0=fibonacci, 1=tribonacci, 2=primonacci */
long long raf_seq_mod(int type, int n, int m);

/* Pisano period: Fibonacci mod m is periodic with period P(m).
 * P(7)=16, P(10)=60, P(14)=48, P(70)=240. */
int raf_pisano_period(int m);

/* Radix economy = ceil(log_base(n_max)) * base. Lower is more economical. */
double raf_base_efficiency(int base, long long n_max);

/* Prime fluid graph. Nodes=primes; edges where (p2-p1) % mod == 0.
 * Writes JSON to buf. Returns bytes written or -1 on error. */
int raf_prime_fluid_graph(const int *primes, int n_primes, int mod,
                           char *buf, int buf_len);

/* Analyze nums in multiple bases and mod {7,10,14,70}. Writes JSON array. */
int raf_analyze_special(const long long *nums, int n_nums,
                         const int *bases, int n_bases,
                         char *buf, int buf_len);

/* Show how Z/base_a Z and Z/base_b Z coexist (coincide at multiples of LCM).
 * Writes JSON describing both rings, coincidence points, Pisano periods. */
int raf_zero_curve_dual(int base_a, int base_b, char *buf, int buf_len);

#endif /* RAF_NUMBASE_H */
