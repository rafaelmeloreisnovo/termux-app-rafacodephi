/* RAFAELIA Ω / native integer-reduction microbenchmark V1
 * Boundary: Android/Linux userland (hosted clock_gettime+stdio), NOT bare-metal.
 * No root, no MMIO, no privileged registers, no emulation required.
 * License: Apache-2.0, standalone authored measurement harness.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>

static uint64_t elapsed_ns(const struct timespec *a, const struct timespec *b) {
    return (uint64_t)(b->tv_sec-a->tv_sec)*1000000000u
         + (uint64_t)((int64_t)b->tv_nsec-a->tv_nsec);
}

/* Inline compiler barrier prevents replacing the LOOP with n*(n+1)/2.
 * It is NOT an ARM instruction to access a privileged hardware register.
 * Keep source and disassembly attached to any performance claim.
 */
static uint64_t run_sum(uint32_t iterations) {
    uint64_t sum=0;
    for (uint32_t i=1; i<=iterations; ++i) {
        __asm__ __volatile__("" : "+r"(i) :: "memory");
        sum+=(uint64_t)i;
    }
    return sum;
}

int main(int argc, char **argv) {
    unsigned long count=10000000ul;
    unsigned long rounds=7ul;
    if (argc>3) { fputs("usage: rafsum [iterations<=100000000] [rounds<=30]\n",stderr); return 64; }
    if(argc>1) {
        char *end=NULL;errno=0;count=strtoul(argv[1],&end,10);
        if(errno || !end || *end || count<1 || count>100000000ul) return 64;
    }
    if(argc>2) {
        char *end=NULL;errno=0;count=strtoul(argv[2],&end,10);
        if(errno || !end || *end || rounds<1 || rounds>30) return 64;
    }
    const uint64_t expected=((uint64_t)count*((uint64_t)count+1u))/2u;
    for(unsigned long r=0;r<rounds+1;r++) {
        struct timespec w0,w1,c0,c1;
        if(clock_gettime(CLOCK_MONOTONIC,&w0) || clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&c0)) return 70;
        const uint64_t actual=run_sum((uint32_t)count);
        if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&c1) || clock_gettime(CLOCK_MONOTONIC,&w1)) return 70;
        if(actual!=expected){ fprintf(stderr,"CHECKSUM_FAIL expected=%" PRIu64 " observed=%" PRIu64 "\n",expected,actual);return 1;}
        if(r==0) continue; /* warm-up, never silently folded into measured runs */
        const uint64_t wall=elapsed_ns(&w0,&w1), cpu=elapsed_ns(&c0,&c1);
        if(!wall || !cpu) return 70;
        printf("{\"schema\":\"rafaelia.native-integer-sum-bench/v1\",\"round\":%lu,"
               "\"iterations\":%lu,\"wall_ns\":%" PRIu64 ",\"cpu_ns\":%" PRIu64 ","
               "\"checksum\":%" PRIu64 ",\"compiler_barrier\":true,"
               "\"timer\":\"clock_gettime\",\"claim_allowed\":false}\n",
               r,count,wall,cpu,actual);
    }
    return 0;
}
