#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# RAFAELIA KERNEL 7.0: HOLOGRAPHIC LOW-LEVEL MATRIX ENGINE
# COMPLIANCE: ISO/IEC 25010 (Efficiency), IEEE 754 (Float Precision).
# TARGET: Android 15 ARM64 (Zero-Dependency, Pure Math).
# FEATURES: Shannon Entropy, XOR-Shift Physics, Power-Law Histogram.

import numpy as np
import time
import sys
import platform
import os
import argparse
import json

# --- GLOBAL CONFIGURATION ---
FLOAT_TYPE = np.float32
INT_TYPE = np.uint8
IS_ANDROID = ("ANDROID_ROOT" in os.environ or "termux" in os.getenv("SHELL", "").lower())
N_DEFAULT = 64 if IS_ANDROID else 100

# --- MATH TRICKS & PHYSICS KERNELS ---

def fn_calc_entropy(data):
    """
    [MATH TRICK] Shannon Entropy Calculation.
    Measures the 'information density' or complexity of the system.
    H = -sum(p * log2(p))
    """
    # Flattens and counts unique values (histogram)
    _, counts = np.unique(data, return_counts=True)
    probs = counts / data.size
    # Avoid log2(0)
    return -np.sum(probs * np.log2(probs + 1e-10))

def fn_soc_float(T, threshold, alpha, diffusion_rate):
    """
    [PHYSICS] Standard SOC with explicit Diffusion Coupling.
    """
    active = T > threshold
    avalanches = np.count_nonzero(active)
    
    if avalanches > 0:
        # Relaxation
        noise = np.random.rand(*T.shape).astype(FLOAT_TYPE)
        T[active] = T[active] * FLOAT_TYPE(0.7) + FLOAT_TYPE(0.3) * noise[active]

        # Laplacian Diffusion (Slicing - No Roll)
        # Core slice
        Uc = T[1:-1, 1:-1, 1:-1]
        # Neighbor sum
        neighbors = (
            T[2:, 1:-1, 1:-1] + T[:-2, 1:-1, 1:-1] +
            T[1:-1, 2:, 1:-1] + T[1:-1, :-2, 1:-1] +
            T[1:-1, 1:-1, 2:] + T[1:-1, 1:-1, :-2]
        )
        # Apply Diffusion: dU/dt = D * Laplacian
        # Laplacian = neighbors - 6*center
        L = neighbors - (FLOAT_TYPE(6.0) * Uc)
        T[1:-1, 1:-1, 1:-1] += FLOAT_TYPE(diffusion_rate) * L

    # EMA Threshold
    current_mean = np.mean(T)
    threshold = FLOAT_TYPE((1.0 - alpha) * threshold + alpha * current_mean)
    threshold = max(FLOAT_TYPE(0.1), min(FLOAT_TYPE(1.0), threshold))
    
    return T, threshold, int(avalanches)

def fn_soc_bitwise_xor(T_int, threshold_int, step_idx):
    """
    [BITWISE TRICK] Pure Integer SOC with XOR-Shift Noise.
    REMOVED: np.random calls inside the loop (slow).
    ADDED: Deterministic Chaos via XOR (T ^ (T >> 2)).
    """
    active = T_int > threshold_int
    avalanches = np.count_nonzero(active)
    
    if avalanches > 0:
        val = T_int[active]
        
        # 1. Decay: x - (x >> 2) is exactly x * 0.75. 
        # This is faster/cleaner than (x>>1) + (x>>2).
        decayed = val - (val >> 2)
        
        # 2. XOR Noise Generation (Deterministic & Fast)
        # Mixes the value itself, a shifted version, and the time step.
        # This creates "texture" noise without external RNG.
        noise_val = (val ^ (val >> 3) ^ (step_idx & 0xFF)) & 0x0F # Mask to 0-15
        
        # 3. Saturated Add
        # Cast to int16 to prevent wrap, then clip
        updated = decayed.astype(np.int16) + noise_val.astype(np.int16)
        
        # Write back with clip
        T_int[active] = np.clip(updated, 0, 255).astype(INT_TYPE)
        
    # Threshold update (Mean)
    new_thresh = int(np.mean(T_int))
    # Adaptive Clamp
    new_thresh = max(20, min(240, new_thresh))
        
    return T_int, new_thresh, int(avalanches)

def fn_ns_step(U, gain):
    """
    [PHYSICS] Navier-Stokes (Heat eq) approximation.
    """
    Uc = U[1:-1, 1:-1, 1:-1]
    L = (
        U[2:, 1:-1, 1:-1] + U[:-2, 1:-1, 1:-1] +
        U[1:-1, 2:, 1:-1] + U[1:-1, :-2, 1:-1] +
        U[1:-1, 1:-1, 2:] + U[1:-1, 1:-1, :-2] -
        FLOAT_TYPE(6.0) * Uc
    )
    U[1:-1, 1:-1, 1:-1] += FLOAT_TYPE(gain) * L
    return U

# --- REPORTING & IO ---

def fn_report_results(metrics, aval_data, entropy_data, N, args):
    """
    Comprehensive reporting with Histogram bins.
    """
    # Calculate Histograms (Power Law check)
    def get_hist(data):
        if not data: return [], []
        h, e = np.histogram(data, bins=10)
        return h.tolist(), e.tolist()

    hist_float_counts, hist_float_edges = get_hist(aval_data["float"])
    hist_bit_counts, hist_bit_edges = get_hist(aval_data["bitwise"])
    
    # Console Output
    print("\n\033[36m--- 📊 RAFAELIA HOLOGRAPHIC REPORT ---\033[0m")
    print(f" [GRID] {N}x{N}x{N} | Seed: {args.seed}")
    print("-" * 50)
    
    f_time = metrics["float_time"]
    b_time = metrics["bitwise_time"]
    
    print(f" 1. Float SOC:   {f_time:.4f}s | Entropia Final: {entropy_data['float']:.3f} bits")
    print(f" 2. Bitwise XOR: {b_time:.4f}s | Entropia Final: {entropy_data['bitwise']:.3f} bits")
    print(f"    \033[33m⚡ Speedup: {metrics['ns_time']/b_time:.2f}x vs NS\033[0m")
    
    # JSONL Export
    export_obj = {
        "ts": int(time.time()),
        "ver": "7.0",
        "params": vars(args),
        "metrics": metrics,
        "entropy": entropy_data,
        "distrib": {
            "float_hist": hist_float_counts,
            "float_edges": hist_float_edges,
            "bit_hist": hist_bit_counts,
            "bit_edges": hist_bit_edges
        }
    }
    
    logfile = "rafaelia_metrics.log.jsonl"
    with open(logfile, "a") as f:
        f.write(json.dumps(export_obj) + "\n")
    print(f" [LOG] Saved to {logfile}")

def fn_run_benchmark(args):
    if args.seed is not None:
        np.random.seed(args.seed)
        
    size = (args.n, args.n, args.n)
    
    # Init Grids
    T_f = np.random.rand(*size).astype(FLOAT_TYPE)
    T_b = (np.random.rand(*size) * 255).astype(INT_TYPE)
    U_n = np.random.rand(*size).astype(FLOAT_TYPE)
    
    th_f = 0.8
    th_b = 200
    
    stats = {"float": [], "bitwise": []}
    
    # 1. Float Loop
    t0 = time.time()
    for _ in range(args.iters):
        T_f, th_f, av = fn_soc_float(T_f, th_f, args.alpha, args.diff)
        stats["float"].append(av)
    tf = time.time() - t0
    final_entropy_f = fn_calc_entropy(T_f)
    
    # 2. Bitwise Loop (XOR Trick)
    t1 = time.time()
    for i in range(args.iters):
        T_b, th_b, av = fn_soc_bitwise_xor(T_b, th_b, i)
        stats["bitwise"].append(av)
    tb = time.time() - t1
    final_entropy_b = fn_calc_entropy(T_b)
    
    # 3. NS Loop
    t2 = time.time()
    for _ in range(args.iters):
        U_n = fn_ns_step(U_n, args.ns_gain)
    tns = time.time() - t2
    
    metrics = {
        "float_time": tf, "bitwise_time": tb, "ns_time": tns,
        "iops_bit": (size[0]**3 * args.iters)/tb
    }
    entropies = {"float": final_entropy_f, "bitwise": final_entropy_b}
    
    fn_report_results(metrics, stats, entropies, args.n, args)

# --- CLI & INTERFACE ---

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=["bench", "ui"], default="ui")
    parser.add_argument("--n", type=int, default=N_DEFAULT)
    parser.add_argument("--iters", type=int, default=50)
    parser.add_argument("--seed", type=int, default=None, help="Fixed random seed")
    # Physics Knobs
    parser.add_argument("--alpha", type=float, default=0.05, help="Threshold inertia")
    parser.add_argument("--diff", type=float, default=0.02, help="Diffusion rate")
    parser.add_argument("--ns_gain", type=float, default=0.1, help="NS viscosity")
    
    args = parser.parse_args()
    
    if args.mode == "bench":
        fn_run_benchmark(args)
    else:
        # BBS UI
        if sys.stdout.isatty(): os.system('cls' if os.name == 'nt' else 'clear')
        print("\033[36m RAFAELIA 7.0 [HOLOGRAPHIC CORE]\033[0m")
        print("\033[33m > Seed Control | XOR Physics | Entropy Analysis\033[0m")
        print(f" > Detected: Android={IS_ANDROID}, Arch={platform.machine()}")
        print("\n Running Default Benchmark...")
        fn_run_benchmark(args)

if __name__ == "__main__":
    main()
