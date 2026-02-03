/*
 * RAFAELIA – TEOROIDE DE ALINHAMENTO COM ONDA DE PERTURBAÇÃO (C LOW LEVEL)
 *
 * Agora temos:
 *  - N "galáxias", cada uma com vetor de spin s_i (|s_i| = 1).
 *  - Um filamento com direção base eF_base = (0,0,1).
 *  - Um campo de onda φ(i,t) correndo ao longo do anel (índice i).
 *  - A direção local do filamento é:
 *        eF_local(i) = normalize( eF_base + ε_wave * φ(i) * n_perp )
 *    onde n_perp é um vetor fixo perpendicular a eF_base.
 *
 *  Spins se alinham com eF_local(i), enquanto a onda se propaga e é amortecida.
 *
 * Compilação:
 *    gcc -O2 -std=c11 -Wall -Wextra -o rafael_teoroid_wave rafael_teoroid_wave.c -lm
 *
 * Execução:
 *    ./rafael_teoroid_wave > alinhamento_onda.dat
 *
 * Saída: passo   E_avg
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ------------------------- PARÂMETROS GLOBAIS ------------------------- */

#define N_GALAXIES   64       /* número de "galáxias" (spins e pontos da onda) */
#define N_STEPS      20000    /* passos de tempo */
#define DT           0.01     /* passo de tempo (Δt) */

/* Dinâmica do spin */
#define K_ALIGN      1.0      /* constante de alinhamento */
#define SIGMA_NOISE  0.05     /* ruído local (σ) – pode reduzir já que temos onda */

/* Dinâmica da onda φ */
#define C_WAVE       1.0      /* velocidade da onda */
#define DAMP_WAVE    0.05     /* amortecimento da onda (γ) */
#define EPS_WAVE     0.2      /* intensidade de acoplamento φ -> inclinação do eixo */

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
        Vec3 r = {1.0, 0.0, 0.0};
        return r;
    }
}

/* Projeção no plano tangente em s (|s|=1):
 * P_s(v) = v - (s·v) s
 */
static Vec3 project_tangent(Vec3 s, Vec3 v) {
    double sv = vec_dot(s, v);
    Vec3 term = vec_scale(s, sv);
    return vec_sub(v, term);
}

/* ------------------------- RNG BÁSICO ------------------------- */

/* double em [0,1) */
static double randu() {
    return (double)rand() / (double)RAND_MAX;
}

/* double em (-1,1) */
static double randu_sym() {
    return 2.0 * randu() - 1.0;
}

/* Vetor aleatório unitário "quase uniforme" na esfera */
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
    /* Spins s[i] e campo de onda φ[i], φ_vel[i] */
    Vec3 s[N_GALAXIES];
    double phi[N_GALAXIES];
    double phi_vel[N_GALAXIES];

    /* Direção base do filamento e vetor perpendicular */
    Vec3 eF_base = {0.0, 0.0, 1.0};
    Vec3 n_perp  = {1.0, 0.0, 0.0};  /* ortogonal a eF_base */

    int i, step;

    /* Semente RNG */
    srand((unsigned int)time(NULL));

    /* Inicializa spins aleatórios e onda inicial */
    for (i = 0; i < N_GALAXIES; i++) {
        s[i] = random_unit_vec();

        /* Onda inicial: pequena perturbação, ou um pulso em um ponto só, etc. */
        phi[i] = 0.0;
        phi_vel[i] = 0.0;
    }

    /* Exemplo: um "pulso" inicial no ponto 0 */
    phi[0] = 1.0;

    const double sqrt_dt = sqrt(DT);

    for (step = 0; step <= N_STEPS; step++) {
        /* 1) Calcula energia média de desalinhamento em relação ao eixo local */
        double E = 0.0;
        for (i = 0; i < N_GALAXIES; i++) {
            /* eixo local inclinado pela onda */
            Vec3 tilt = vec_scale(n_perp, EPS_WAVE * phi[i]);
            Vec3 eF_local = vec_add(eF_base, tilt);
            eF_local = vec_normalize(eF_local);

            double cospsi = vec_dot(s[i], eF_local);
            double diff   = 1.0 - cospsi;
            E += diff * diff;
        }
        double E_avg = E / (double)N_GALAXIES;

        printf("%d %.10f\n", step, E_avg);

        if (step == N_STEPS) {
            break;
        }

        /* 2) Atualiza a onda φ (equação de onda discreta com amortecimento) */
        for (i = 0; i < N_GALAXIES; i++) {
            int i_plus  = (i + 1) % N_GALAXIES;
            int i_minus = (i + N_GALAXIES - 1) % N_GALAXIES;

            double lap = phi[i_plus] - 2.0 * phi[i] + phi[i_minus];
            double acc = (C_WAVE * C_WAVE) * lap - DAMP_WAVE * phi_vel[i];

            phi_vel[i] += acc * DT;
        }
        for (i = 0; i < N_GALAXIES; i++) {
            phi[i] += phi_vel[i] * DT;
        }

        /* 3) Atualiza spins com base em eF_local e ruído */
        for (i = 0; i < N_GALAXIES; i++) {
            Vec3 si = s[i];

            /* eixo local inclinado pela onda em i */
            Vec3 tilt = vec_scale(n_perp, EPS_WAVE * phi[i]);
            Vec3 eF_local = vec_add(eF_base, tilt);
            eF_local = vec_normalize(eF_local);

            /* termo de alinhamento */
            double cospsi = vec_dot(si, eF_local);
            Vec3 ef_proj  = project_tangent(si, eF_local);
            double factor = 2.0 * K_ALIGN * (1.0 - cospsi) * DT;
            Vec3 drift    = vec_scale(ef_proj, factor);

            /* ruído local (pequeno) */
            Vec3 eta = random_unit_vec();
            Vec3 eta_proj = project_tangent(si, eta);
            Vec3 noise = vec_scale(eta_proj, SIGMA_NOISE * sqrt_dt);

            /* update */
            Vec3 si_new = vec_add(si, drift);
            si_new = vec_add(si_new, noise);

            s[i] = vec_normalize(si_new);
        }
    }

    return 0;
}
