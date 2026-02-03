/* RAFAELIA OMEGA16 – RAW PTHREAD EDITION
 * Target: Android / Termux (ARM64)
 * Features: pthreads + LUT + Entropy + IOPS + Events
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define FP_SHIFT 8
#define FP_ONE   (1 << FP_SHIFT)
#define FP_MAX   (255 * FP_ONE)
#define LUT_SIZE 65536

typedef struct {
    uint16_t *T;
    uint16_t *Flow;
    uint8_t  *Residual;
    int N;
    long long total_cells;
    int iters;
} SystemState;

typedef struct {
    int thread_id;
    int num_threads;
    SystemState *sys;
    int threshold;
    int z_start;
    int z_end;
    long long local_avalanches;
    pthread_barrier_t *barrier;
} ThreadArg;

/* LUTs – matemática “atemporal” pré-calculada */
static uint16_t LUT_DECAY[LUT_SIZE];
static uint16_t LUT_FLOW[LUT_SIZE];

static void init_luts(void) {
    for (int i = 0; i < LUT_SIZE; i++) {
        LUT_DECAY[i] = i >> 2;   /* 25% de decaimento */
        LUT_FLOW[i]  = i >> 7;   /* fluxo ~ 1/128 */
    }
}

static inline double get_time_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static inline uint16_t fast_hash(int i, int step) {
    uint32_t x = (uint32_t)i ^ (uint32_t)(step * 0x9E3779B9u);
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (uint16_t)(x & 0x1Fu);
}

/* Worker de física + difusão */
static void *worker_func(void *arg) {
    ThreadArg *ta = (ThreadArg *)arg;
    SystemState *sys = ta->sys;
    int N = sys->N;
    int stride_z = N * N;
    int stride_y = N;

    uint16_t *T = sys->T;
    uint16_t *Flow = sys->Flow;
    uint8_t  *Res = sys->Residual;
    int threshold = ta->threshold;

    for (int step = 0; step < sys->iters; step++) {
        ta->local_avalanches = 0;

        /* FASE 1: UPDATE LOCAL */
        for (int z = ta->z_start; z < ta->z_end; z++) {
            if (z == 0 || z == N - 1) continue;
            int base_z = z * stride_z;
            for (int y = 1; y < N - 1; y++) {
                int base_y = base_z + y * stride_y;
                for (int x = 1; x < N - 1; x++) {
                    int i = base_y + x;
                    uint16_t val = T[i];

                    if ((val >> FP_SHIFT) > (threshold >> FP_SHIFT)) {
                        ta->local_avalanches++;

                        uint16_t val_full = (uint16_t)(val + Res[i]);
                        uint16_t decay = LUT_DECAY[val_full];
                        Res[i] = (uint8_t)(val_full & 0x03u);

                        uint16_t noise = fast_hash(i, step);
                        uint16_t leak  = val >> 3;

                        int32_t res = (int32_t)val_full - (int32_t)decay
                                      + (int32_t)noise - (int32_t)(leak * 6);
                        if (res < 0) res = 0;
                        if (res > FP_MAX) res = FP_MAX;

                        T[i]    = (uint16_t)res;
                        Flow[i] = LUT_FLOW[val];
                    } else {
                        Flow[i] = 0;
                    }
                }
            }
        }

        /* Espera todos terminarem Fase 1 */
        pthread_barrier_wait(ta->barrier);

        /* FASE 2: DIFUSÃO GLOBAL */
        for (int z = ta->z_start; z < ta->z_end; z++) {
            if (z == 0 || z == N - 1) continue;
            int base_z = z * stride_z;
            for (int y = 1; y < N - 1; y++) {
                int base_y = base_z + y * stride_y;
                for (int x = 1; x < N - 1; x++) {
                    int i = base_y + x;
                    uint16_t inflow =
                        Flow[i + 1] + Flow[i - 1] +
                        Flow[i + stride_y] + Flow[i - stride_y] +
                        Flow[i + stride_z] + Flow[i - stride_z];

                    if (inflow > 0) {
                        int32_t res = (int32_t)T[i] + (int32_t)inflow;
                        if (res > FP_MAX) res = FP_MAX;
                        T[i] = (uint16_t)res;
                    }
                }
            }
        }

        /* Espera todos terminarem Fase 2 */
        pthread_barrier_wait(ta->barrier);
    }

    return NULL;
}

/* Entropia (single-thread) */
static double calc_entropy(const uint16_t *data, long long size) {
    uint64_t counts[256] = {0};
    for (long long i = 0; i < size; i++) {
        counts[data[i] >> FP_SHIFT]++;
    }

    double entropy = 0.0;
    double inv_size = 1.0 / (double)size;

    for (int i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            double p = counts[i] * inv_size;
            entropy -= p * log2(p);
        }
    }
    return entropy;
}

/* Runner principal */
static void run_omega(int N, int ITERS) {
    init_luts();

    long long total_cells = (long long)N * N * N;
    int num_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads < 1) num_threads = 1;
    if (num_threads > 16) num_threads = 16; /* segurança */

    printf("RAFAELIA OMEGA16 RAW\n");
    printf("Cores: %d | Grid: %dx%dx%d | Iters: %d\n",
           num_threads, N, N, N, ITERS);

    SystemState sys;
    sys.N = N;
    sys.total_cells = total_cells;
    sys.iters = ITERS;

    sys.T        = (uint16_t *)calloc((size_t)total_cells, sizeof(uint16_t));
    sys.Flow     = (uint16_t *)calloc((size_t)total_cells, sizeof(uint16_t));
    sys.Residual = (uint8_t  *)calloc((size_t)total_cells, sizeof(uint8_t));

    if (!sys.T || !sys.Flow || !sys.Residual) {
        printf("ERRO: falha de alocacao de memoria.\n");
        free(sys.T); free(sys.Flow); free(sys.Residual);
        return;
    }

    /* Init do campo */
    for (long long i = 0; i < total_cells; i++) {
        sys.T[i] = (uint16_t)((fast_hash((int)i, 0) % 200) * FP_ONE);
        sys.Residual[i] = 0;
    }

    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)num_threads);
    ThreadArg *targs   = (ThreadArg *)malloc(sizeof(ThreadArg) * (size_t)num_threads);
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, (unsigned)num_threads);

    int slice = N / num_threads;
    if (slice < 1) slice = 1;
    int threshold = 120 * FP_ONE;

    double t0 = get_time_now();

    /* Spawn threads */
    for (int t = 0; t < num_threads; t++) {
        targs[t].thread_id = t;
        targs[t].num_threads = num_threads;
        targs[t].sys = &sys;
        targs[t].threshold = threshold;
        targs[t].barrier = &barrier;
        targs[t].local_avalanches = 0;

        targs[t].z_start = t * slice;
        targs[t].z_end   = (t == num_threads - 1) ? N : (t + 1) * slice;

        if (targs[t].z_start >= N) targs[t].z_start = N - 1;
        if (targs[t].z_end > N)    targs[t].z_end   = N;

        pthread_create(&threads[t], NULL, worker_func, &targs[t]);
    }

    /* Espera todas terminarem */
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    double tf = get_time_now() - t0;

    /* Agrega eventos */
    long long total_events = 0;
    for (int t = 0; t < num_threads; t++) {
        total_events += targs[t].local_avalanches;
    }

    double iops = (double)total_cells * (double)ITERS / (tf + 1e-9);
    double ent  = calc_entropy(sys.T, total_cells);

    printf("\n--- OMEGA16 RESULTS ---\n");
    printf("Tempo:   %.4f s\n", tf);
    printf("IOPS:    %.2e\n", iops);
    printf("Entropia: %.4f bits\n", ent);
    printf("Eventos: %lld\n", total_events);

    FILE *log = fopen("rafaelia_omega16.log.jsonl", "a");
    if (log) {
        fprintf(log,
                "{\"ver\":\"16.0-RAW\",\"ts\":%.0f,\"n\":%d,\"iters\":%d,"
                "\"iops\":%.2e,\"entropy\":%.4f,\"events\":%lld}\n",
                get_time_now(), N, ITERS, iops, ent, total_events);
        fclose(log);
        printf("Log salvo em rafaelia_omega16.log.jsonl\n");
    }

    pthread_barrier_destroy(&barrier);
    free(threads);
    free(targs);
    free(sys.T);
    free(sys.Flow);
    free(sys.Residual);
}

int main(int argc, char *argv[]) {
    int N = 100;
    int ITERS = 500;

    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) ITERS = atoi(argv[2]);

    run_omega(N, ITERS);
    return 0;
}
