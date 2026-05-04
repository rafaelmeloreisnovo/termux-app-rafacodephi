#!/usr/bin/env python3
"""RAFAELIA automatic calibration by lightweight genetic search."""
import numpy as np

PERIOD = 42
FREQ_Q16 = np.array([9804, 19608, 29412, 39216, 49020, 58824, 68628], dtype=np.float64)


def simulate_phi(pump: np.ndarray) -> float:
    C, H = 0.5, 0.5
    for t in range(PERIOD):
        cin = np.tanh(np.dot(pump, np.sin((t + 1) * FREQ_Q16 / 65536.0))) * 0.5 + 0.5
        hin = 1.0 - cin
        C = 0.75 * C + 0.25 * cin
        H = 0.75 * H + 0.25 * hin
    return (1.0 - H) * C


def optimize(pop=48, gens=60, seed=42):
    rng = np.random.default_rng(seed)
    population = rng.uniform(0.05, 3.0, size=(pop, 7))
    for _ in range(gens):
        scores = np.array([simulate_phi(ind) for ind in population])
        elite = population[np.argsort(scores)[-12:]]
        children = []
        for _ in range(pop - len(elite)):
            a = elite[rng.integers(0, len(elite))]
            b = elite[rng.integers(0, len(elite))]
            mask = rng.random(7) < 0.5
            child = np.where(mask, a, b) + rng.normal(0.0, 0.04, size=7)
            children.append(np.clip(child, 0.05, 3.0))
        population = np.vstack([elite, np.asarray(children)])
    final_scores = np.array([simulate_phi(ind) for ind in population])
    best_idx = int(np.argmax(final_scores))
    return population[best_idx], float(final_scores[best_idx])


if __name__ == "__main__":
    best, score = optimize()
    print("best_pump=", np.array2string(best, precision=6, separator=","))
    print("phi42=", f"{score:.8f}")
