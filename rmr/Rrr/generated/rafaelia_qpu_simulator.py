#!/usr/bin/env python3
"""RAFAELIA Classical QPU Simulator.
Implements T^7 continuous-variable dynamics with Q16.16 fixed-point arithmetic,
canonical period 42, BitStack parity/CRC checks, commit gate rollback, Mandelbrot
coupling modulation, and attractor collapse by Manhattan distance.
"""
from __future__ import annotations
import math
from dataclasses import dataclass
import numpy as np
import matplotlib.pyplot as plt

Q16 = 1 << 16
PERIOD = 42
N_OSC = 1000
DIM = 7
SPIRAL_Q16 = 56755
PHI_Q16 = 105965
PI_Q16 = 205887
Q16_2PI = 411774
CONST_TERM_Q16 = 3146
ALPHA_NUM = 16384  # 0.25 in Q16.16
ONE_MINUS_ALPHA_NUM = 49152  # 0.75

FREQ_Q16 = np.array([9804, 19608, 29412, 39216, 49020, 58824, 68628], dtype=np.int64)
W_INIT_Q16 = np.array([65536, 56755, 49157, 42573, 36877, 31940, 27671], dtype=np.int64)


def qmul(a: np.ndarray, b: np.ndarray | int) -> np.ndarray:
    return ((a.astype(np.int64) * np.asarray(b, dtype=np.int64)) >> 16).astype(np.int64)


def wrap16(a: np.ndarray) -> np.ndarray:
    return a & 0xFFFF


def taylor_sin_q16(theta_q16: np.ndarray) -> np.ndarray:
    x = (theta_q16.astype(np.float64) / Q16) * 2.0 * math.pi
    x3 = x * x * x
    x5 = x3 * x * x
    y = x - x3 / 6.0 + x5 / 120.0
    return np.clip((y * Q16).astype(np.int64), -65535, 65535)


def crc32c_soft(data: bytes) -> int:
    """CRC32C (Castagnoli) software implementation (poly=0x82F63B78)."""
    poly = 0x82F63B78
    crc = 0xFFFFFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ poly if (crc & 1) else (crc >> 1)
    return (~crc) & 0xFFFFFFFF


def mandelbrot_q16(u: int, v: int, max_iter: int = 21) -> int:
    c = complex((u / 65535.0) * 3.0 - 2.0, (v / 65535.0) * 3.0 - 1.5)
    z = 0j
    i = 0
    while i < max_iter and (z.real * z.real + z.imag * z.imag) <= 4.0:
        z = z * z + c
        i += 1
    return int((i * Q16) / max_iter)


def gen_42_attractors() -> np.ndarray:
    a = np.zeros((PERIOD, DIM), dtype=np.int64)
    seed = np.array([56755, 105965, 205887, 46341, 92682, 138564, 184245], dtype=np.int64) & 0xFFFF
    for i in range(PERIOD):
        seed = wrap16(qmul(seed, SPIRAL_Q16) + np.roll(seed, -1) + i)
        a[i] = seed
    return a


def apply_direction(state: np.ndarray, d: int) -> np.ndarray:
    s = state.copy()
    if d == 0:  # UP
        s[:, 0] = wrap16(s[:, 0] + s[:, 2])
    elif d == 1:  # DOWN
        s[:, 1] = wrap16(s[:, 1] - s[:, 3])
    elif d == 2:  # FORWARD recurrence S-box style
        s[:, 2] = wrap16(qmul(s[:, 2], SPIRAL_Q16) + CONST_TERM_Q16)
    elif d == 3:  # RECURSE
        s = np.roll(s, 1, axis=1)
    elif d == 4:  # COMPRESS
        s[:, 4] = wrap16((s[:, 4] + s[:, 5]) >> 1)
    elif d == 5:  # EXPAND
        s[:, 5] = wrap16(s[:, 5] + (s[:, 0] >> 2))
    elif d == 6:  # PHASE/XOR
        s[:, 6] = wrap16(s[:, 6] ^ s[:, 1])
    return s


@dataclass
class CommitGate:
    snapshot: np.ndarray
    rollbacks: int = 0

    def verify_and_commit(self, state: np.ndarray, cycle: int) -> np.ndarray:
        global_xor = int(np.bitwise_xor.reduce(state[:, 0] & 0xFF))
        crc = crc32c_soft(state.astype(np.uint16).tobytes())
        # VERIFY phase (every multiple of 7)
        ok = ((global_xor ^ (crc & 0xFF)) != 0)
        if cycle % 7 == 0:
            if ok:
                self.snapshot = state.copy()
            else:
                self.rollbacks += 1
                state = self.snapshot.copy()
        return state


def run() -> None:
    rng = np.random.default_rng(42)
    state = rng.integers(0, 65536, size=(N_OSC, DIM), dtype=np.int64)
    weights = np.tile(W_INIT_Q16, (N_OSC, 1)).astype(np.int64)
    commit = CommitGate(snapshot=state.copy())
    attractors = gen_42_attractors()

    C = 0.5
    H = 0.5
    coh_hist, ent_hist, phi_hist = [], [], []

    for cyc in range(PERIOD):
        state = apply_direction(state, cyc % 7)

        layer_sum = np.zeros(N_OSC, dtype=np.int64)
        for k in range(7):
            arg = wrap16(qmul(state[:, k], FREQ_Q16[k]))
            layer_sum += qmul(weights[:, k], taylor_sin_q16(arg))
            weights[:, k] = ((weights[:, k] * ONE_MINUS_ALPHA_NUM + W_INIT_Q16[k] * ALPHA_NUM) >> 16)

        mods = np.array([mandelbrot_q16(int(state[i, 0]), int(state[i, 1])) for i in range(N_OSC)], dtype=np.int64)
        state[:, 2] = wrap16(state[:, 2] + (layer_sum >> 4) + (mods >> 6))

        state = commit.verify_and_commit(state, cyc)

        # toroidal stride routing 7 (coprime with 1000)
        route = (np.arange(N_OSC, dtype=np.int64) * 7) % N_OSC
        state = state[route]

        c_in = float(np.mean(state[:, 0]) / 65535.0)
        h_in = float(np.std(state[:, 1]) / 65535.0)
        C = 0.75 * C + 0.25 * c_in
        H = 0.75 * H + 0.25 * h_in
        phi = (1.0 - H) * C

        coh_hist.append(C)
        ent_hist.append(H)
        phi_hist.append(phi)

    d = np.abs(state[:, None, :] - attractors[None, :, :]).sum(axis=2)
    closest = np.argmin(d, axis=1)
    print(f"rollbacks={commit.rollbacks} mean_attractor={closest.mean():.4f} phi_last={phi_hist[-1]:.6f}")

    plt.figure(figsize=(10, 4))
    plt.plot(coh_hist, label="coherence")
    plt.plot(ent_hist, label="entropy")
    plt.plot(phi_hist, label="phi")
    plt.title("RAFAELIA 42-cycle evolution")
    plt.xlabel("cycle")
    plt.grid(alpha=0.25)
    plt.legend()
    plt.tight_layout()
    plt.savefig("rmr/Rrr/generated/rafaelia_metrics.png", dpi=150)


if __name__ == "__main__":
    run()
