/*
 * RAFAELIA – TEOROIDE + ONDA + MC^2 (C LOW LEVEL)
 *
 * Agora:
 *  - Cada "galáxia" i tem massa m[i] e spin s[i].
 *  - Campo de onda φ(i,t) e velocidade φ_vel(i,t) ao longo do filamento.
 *  - Direção local do filamento eF_local(i) é inclinada por φ(i).
 *
 * Energias:
 *  E_rest  = sum_i m[i] * c^2
 *  E_align = sum_i (1 - s_i · eF_local(i))^2      (desalinhamento)
 *  E_wave  = sum_i ( phi_i^2 + (phi_vel_i^2 / C_WAVE^2) )
 *
 *  E_total = E_rest + alpha_align * E_align + alpha_wave * E_wave
 *
 * Compilação:
 *    gcc -O2 -std=c11 -Wall -Wextra -o rafael_teoroid_wave_mc2 rafael_teoroid_wave_mc2.c -lm
 *
 * Execução:
 *    ./rafael_teoroid_wave_mc2 > energies.dat
 *
 * Saída (colunas):
 *    step  E_rest  E_align  E_wave  E_total
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ------------------------- PARÂMETROS GLOBAIS ------------------------- */

#define N_GALAXIES   64       /* número de "galáxias" */
#define N_STEPS      20000    /* passos de tempo */
#define DT           0.01     /* passo de tempo (Δt) */

/* Dinâmica do spin */
#define K_ALIGN      1.0      /* constante de alinhamento (drift) */
#define SIGMA_NOISE  0.05     /* ruído local (σ) */

/* Dinâmica da onda φ */
#define C_WAVE       1.0      /* velocidade da onda no filamento */
#define DAMP_WAVE    0.05     /* amortecimento da onda (γ) */
#define EPS_WAVE     0.2      /* acoplamento φ -> inclinação do eixo */

/* Constante da luz (unidade simbólica aqui) */
#define C_LIGHT      3.0e5    /* por exemplo, 3 x 10^5 em unidades arbitrárias */

/* Pesos para as parcelas de energia (escala relativa) */
#define ALPHA_ALIGN  1.0
#define ALPHA_WAVE   1.0

/* ------------------------- ESTRUTURA DE VETOR ------------------------- */

typedef struct {
    double x;
    double y;
    double z;
} Vec3;

static Vec3 vec_add(Vec3 a, Vec3 b) {
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

static Vec3 vec_sub(Vec3 a, Vec3 b) {
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

static Vec3 vec_scale(Vec3 v, double c) {
    Vec3 r;
    r.x = c * v.x;
    r.y = c * v.y;
    r.z = c * v.z;
    return r;
}

static double vec_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static double vec_norm(Vec3 v) {
    return sqrt(vec_dot(v, v));
}

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

static double randu() {
    return (double)rand() / (double)RAND_MAX;
}

static double randu_sym() {
    return 2.0 * randu() - 1.0;
}

/* Vetor aleatório unitário na esfera */
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
    /* Spins, onda e massas */
    Vec3  s[N_GALAXIES];
    double phi[N_GALAXIES];
    double phi_vel[N_GALAXIES];
    double m[N_GALAXIES];

    /* Direção base do filamento e vetor perpendicular */
    Vec3 eF_base = {0.0, 0.0, 1.0};
    Vec3 n_perp  = {1.0, 0.0, 0.0};

    const double C2 = C_LIGHT * C_LIGHT;
    const double sqrt_dt = sqrt(DT);

    int i, step;

    srand((unsigned int)time(NULL));

    /* Inicializa spins, onda e massas */
    for (i = 0; i < N_GALAXIES; i++) {
        s[i] = random_unit_vec();

        phi[i] = 0.0;
        phi_vel[i] = 0.0;

        /* Massa simbólica: pode ser tudo 1.0 ou variar um pouco */
        m[i] = 1.0 + 0.1 * randu_sym();  /* em torno de 1 com pequena variação */
    }

    /* Pulso inicial da onda em i=0 */
    phi[0] = 1.0;

    /* E_rest é constante no tempo (massa não muda) */
    double E_rest = 0.0;
    for (i = 0; i < N_GALAXIES; i++) {
        E_rest += m[i] * C2;
    }

    /* Loop temporal */
    for (step = 0; step <= N_STEPS; step++) {
        double E_align = 0.0;
        double E_wave  = 0.0;

        /* 1) Calcula E_align e E_wave no estado atual */
        for (i = 0; i < N_GALAXIES; i++) {
            Vec3 tilt = vec_scale(n_perp, EPS_WAVE * phi[i]);
            Vec3 eF_local = vec_add(eF_base, tilt);
            eF_local = vec_normalize(eF_local);

            double cospsi = vec_dot(s[i], eF_local);
            double diff   = 1.0 - cospsi;
            E_align += diff * diff;

            /* energia da onda em cada ponto (forma simples) */
            double term_wave = phi[i] * phi[i]
                             + (phi_vel[i] * phi_vel[i]) / (C_WAVE * C_WAVE);
            E_wave += term_wave;
        }

        double E_total = E_rest
                       + ALPHA_ALIGN * E_align
                       + ALPHA_WAVE  * E_wave;

        /* Saída: step  E_rest  E_align  E_wave  E_total */
        printf("%d %.10e %.10e %.10e %.10e\n",
               step, E_rest, E_align, E_wave, E_total);

        if (step == N_STEPS) {
            break;
        }

        /* 2) Atualiza onda φ (equação de onda discreta) */
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

        /* 3) Atualiza spins s[i] com base em eF_local e ruído */
        for (i = 0; i < N_GALAXIES; i++) {
            Vec3 si = s[i];

            Vec3 tilt = vec_scale(n_perp, EPS_WAVE * phi[i]);
            Vec3 eF_local = vec_add(eF_base, tilt);
            eF_local = vec_normalize(eF_local);

            double cospsi = vec_dot(si, eF_local);
            Vec3 ef_proj  = project_tangent(si, eF_local);
            double factor = 2.0 * K_ALIGN * (1.0 - cospsi) * DT;
            Vec3 drift    = vec_scale(ef_proj, factor);

            Vec3 eta = random_unit_vec();
            Vec3 eta_proj = project_tangent(si, eta);
            Vec3 noise = vec_scale(eta_proj, SIGMA_NOISE * sqrt_dt);

            Vec3 si_new = vec_add(si, drift);
            si_new = vec_add(si_new, noise);

            s[i] = vec_normalize(si_new);
        }
    }

    return 0;
}

