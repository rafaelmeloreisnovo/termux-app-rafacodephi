/*
 * RAFAELIA – TEOROIDE DE ALINHAMENTO DE SPINS (VERSÃO C LOW LEVEL)
 *
 * Modelo numérico simples:
 *  - N "galáxias", cada uma com um vetor de spin s_i (norma 1).
 *  - Um filamento com direção fixa e_F (aqui, eixo Z).
 *  - Energia de desalinhamento:
 *        E = sum_i (1 - s_i · e_F)^2
 *  - Dinâmica (esquema de Euler + ruído):
 *        s_i <- s_i
 *               + 2 k_align (1 - s_i·e_F) P_{s_i}(e_F) dt
 *               + sigma * P_{s_i}(eta_i) * sqrt(dt)
 *        depois normaliza s_i para norma 1.
 *
 *   P_{s}(v) = v - (s·v) s  (projeção no plano tangente da esfera em s)
 *
 * Compile em Termux, Linux, etc.:
 *    gcc -O2 -std=c11 -Wall -Wextra -o rafael_teoroid_align rafael_teoroid_align.c -lm
 *
 * Execute:
 *    ./rafael_teoroid_align > alinhamento.dat
 *
 * As duas colunas de saída:
 *    passo   energia_media
 * onde energia_media = E / N_GALAXIES.
 *
 * Você pode depois plotar (gnuplot, etc.) para ver E(t) caindo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ------------------------- PARÂMETROS GLOBAIS ------------------------- */

#define N_GALAXIES  64        /* número de "galáxias" (spins) */
#define N_STEPS     20000     /* número de passos de tempo */
#define DT          0.01      /* passo de tempo (Δt) */

#define K_ALIGN     1.0       /* constante de alinhamento (k) */
#define SIGMA_NOISE 0.10      /* intensidade do ruído (σ) */

/* ------------------------- ESTRUTURA DE VETOR ------------------------- */

typedef struct {
    double x;
    double y;
    double z;
} Vec3;

/* Soma de vetores: a + b */
static Vec3 vec_add(Vec3 a, Vec3 b) {
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

/* Subtração: a - b */
static Vec3 vec_sub(Vec3 a, Vec3 b) {
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

/* Multiplicação por escalar: c * v */
static Vec3 vec_scale(Vec3 v, double c) {
    Vec3 r;
    r.x = c * v.x;
    r.y = c * v.y;
    r.z = c * v.z;
    return r;
}

/* Produto interno: a · b */
static double vec_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* Norma: |v| */
static double vec_norm(Vec3 v) {
    return sqrt(vec_dot(v, v));
}

/* Normalização: v / |v| (se |v| > 0) */
static Vec3 vec_normalize(Vec3 v) {
    double n = vec_norm(v);
    if (n > 0.0) {
        double inv = 1.0 / n;
        return vec_scale(v, inv);
    } else {
        /* Em caso degenerado, retorna um vetor fixo */
        Vec3 r = {1.0, 0.0, 0.0};
        return r;
    }
}

/* Projeção P_s(v) = v - (s·v) s, no plano tangente em s (|s|=1) */
static Vec3 project_tangent(Vec3 s, Vec3 v) {
    double sv = vec_dot(s, v);
    Vec3 term = vec_scale(s, sv);
    return vec_sub(v, term);
}

/* ------------------------- RNG BÁSICO (UNIFORME) ------------------------- */

/* retorna número double em [0,1) */
static double randu() {
    return (double)rand() / (double)RAND_MAX;
}

/* retorna número double em (-1,1) */
static double randu_sym() {
    return 2.0 * randu() - 1.0;
}

/* Gera um vetor aleatório "quase" uniforme na esfera:
 *  - Gera (x,y,z) ~ U(-1,1)^3
 *  - Rejeita se muito próximo de 0
 *  - Normaliza
 */
static Vec3 random_unit_vec() {
    Vec3 v;
    double n;
    do {
        v.x = randu_sym();
        v.y = randu_sym();
        v.z = randu_sym();
        n = vec_norm(v);
    } while (n < 1e-6);
    return vec_scale(v, 1.0 / n);
}

/* ------------------------- MAIN SIMULAÇÃO ------------------------- */

int main(void) {
    /* s[i] = spin da galáxia i (vetor unitário) */
    Vec3 s[N_GALAXIES];

    /* Direção do filamento (e_F).
     * Aqui escolhemos o eixo Z puro: (0, 0, 1).
     * Em uma versão mais rica, você poderia variar ao longo de γ(s).
     */
    Vec3 eF = {0.0, 0.0, 1.0};

    int i, step;

    /* Semente do RNG */
    srand((unsigned int)time(NULL));

    /* Inicializa spins aleatórios na esfera */
    for (i = 0; i < N_GALAXIES; i++) {
        s[i] = random_unit_vec();
    }

    /* Pré-cálculo: sqrt(dt) para ruído */
    const double sqrt_dt = sqrt(DT);

    /* Loop temporal principal */
    for (step = 0; step <= N_STEPS; step++) {
        /* Calcula energia média de desalinhamento */
        double E = 0.0;
        for (i = 0; i < N_GALAXIES; i++) {
            double cospsi = vec_dot(s[i], eF);    /* cos(ψ_i) = s_i · e_F */
            double diff   = 1.0 - cospsi;
            E += diff * diff;
        }
        double E_avg = E / (double)N_GALAXIES;

        /* Imprime passo e energia média (pode redirecionar para arquivo) */
        printf("%d %.10f\n", step, E_avg);

        /* Último passo: não precisa atualizar */
        if (step == N_STEPS) {
            break;
        }

        /* Atualiza spins (Esquema de Euler com ruído) */
        for (i = 0; i < N_GALAXIES; i++) {
            Vec3 si = s[i];

            /* Produto interno s_i · e_F */
            double cospsi = vec_dot(si, eF);

            /* Drift (alinhamento): 2 k_align (1 - s·e_F) P_s(e_F) dt */
            Vec3 ef_proj = project_tangent(si, eF);
            double factor = 2.0 * K_ALIGN * (1.0 - cospsi) * DT;
            Vec3 drift = vec_scale(ef_proj, factor);

            /* Ruído: sigma * P_s(eta) * sqrt(dt) */
            Vec3 eta = random_unit_vec();       /* ruído bruto */
            Vec3 eta_proj = project_tangent(si, eta);
            Vec3 noise = vec_scale(eta_proj, SIGMA_NOISE * sqrt_dt);

            /* Atualiza s_i */
            Vec3 si_new = vec_add(si, drift);
            si_new = vec_add(si_new, noise);

            /* Renormaliza para ficar na esfera (|s_i| = 1) */
            s[i] = vec_normalize(si_new);
        }
    }

    return 0;
}
