#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# RAFAELIA HEX-ASM CORE [ARM64 Optimized]
# NO CLASSES. NO NUMPY. PURE BITWISE LOGIC. DIRECT I/O.

import sys, time, random, select

# --- [0] REGISTERS (CONSTANTS AS BYTES) ---
CLS = b"\x1b[2J\x1b[H"
RST = b"\x1b[0m"
# Clipper/Neon Palette
C_RED = b"\x1b[31m"; C_GRN = b"\x1b[32m"; C_YEL = b"\x1b[33m"
C_BLU = b"\x1b[34m"; C_CYN = b"\x1b[36m"; C_WHT = b"\x1b[37m"
C_BG  = b"\x1b[44m" 

# Physics (Integer Math 0-255)
E_CRIT = 200      
# Dissipation 0.7 approx 180/256
DISSIPATION = 180 

# --- [1] DIRECT KERNEL I/O (ASM STYLE) ---
STDOUT = sys.stdout.buffer # Bypass encoding overhead

def out(d): STDOUT.write(d)
def move(y, x): STDOUT.write(b"\x1b[%d;%dH" % (y, x))

# --- [2] FLAT MEMORY ALLOCATION ---
N = 40
SIZE = N * N
MEM = bytearray(SIZE) # The Matrix (Raw RAM)

# Pre-calc Hex Neighbors (Odd/Even Stagger offsets)
# Simply: Up, Down, Left, Right in linear memory
OFF_U = -N; OFF_D = N; OFF_L = -1; OFF_R = 1

# --- [3] PHYSICS ENGINE (BITWISE) ---
def tick(cycles=1):
    avalanche = 0
    for _ in range(cycles):
        # Entropy (Bit injection)
        idx = int(random.random() * SIZE)
        v = MEM[idx] + 16
        MEM[idx] = 255 if v > 255 else v

        # Linear Scan (Cache Friendly)
        for i in range(SIZE):
            if MEM[i] >= E_CRIT:
                avalanche += 1
                # Collapse: (Val * 180) >> 8 is fast division
                residue = MEM[i]
                MEM[i] = (residue * DISSIPATION) >> 8
                
                # Distribute (Bitwise shift >> 2 is / 4)
                share = residue >> 2
                if share > 0:
                    # Unrolled neighbor update (No loops)
                    # Boundary checks minimized for speed
                    if i >= N: 
                        v = MEM[i+OFF_U] + share
                        MEM[i+OFF_U] = 255 if v > 255 else v
                    if i < SIZE - N:
                        v = MEM[i+OFF_D] + share
                        MEM[i+OFF_D] = 255 if v > 255 else v
                    if i % N != 0:
                        v = MEM[i+OFF_L] + share
                        MEM[i+OFF_L] = 255 if v > 255 else v
                    if (i+1) % N != 0:
                        v = MEM[i+OFF_R] + share
                        MEM[i+OFF_R] = 255 if v > 255 else v
    return avalanche

# --- [4] HEX RENDERER (STAGGERED GRID) ---
def draw(step, avs, iops):
    out(CLS + C_BG + C_WHT)
    
    # Header
    move(1, 2); out(b"RAFAELIA HEX-ASM CORE [Auth: RAFCODE-PHI]")
    
    # Render Hex Grid
    y_base = 3
    for r in range(N):
        move(y_base + r, 2)
        if r & 1: out(b" ") # The Hexagonal Shift (Bitwise AND)
        
        # Row Buffer Construction
        buf = bytearray()
        off = r * N
        for c in range(N):
            val = MEM[off + c]
            # Contrast Logic (Thresholds)
            if val > E_CRIT: buf.extend(C_RED + b"#" + C_WHT)
            elif val > 128:  buf.extend(C_YEL + b"o" + C_WHT)
            elif val > 64:   buf.extend(C_CYN + b":" + C_WHT)
            elif val > 10:   buf.extend(C_BLU + b"." + C_WHT)
            else:            buf.extend(b" ")
            buf.extend(b" ") # Aspect Ratio
        out(buf)

    # Dashboard
    px = (N * 2) + 6
    move(4, px); out(b"--- METRICS (ASM) ---")
    move(5, px); out(b"Step:      %d" % step)
    move(6, px); out(b"Avalanche: %d" % avs)
    move(7, px); out(b"IOPS Est:  %d" % iops)
    
    # Energy Bar (Bitwise Math)
    tot = sum(MEM)
    fill = (tot * 20) // (SIZE * 255)
    bar = b"|" * fill
    move(9, px); out(b"Entropy: [" + C_GRN + bar + C_WHT + b"]")
    
    move(N+4, 1)
    STDOUT.flush()

# --- [5] KERNEL ---
def main():
    step = 0
    t_last = time.perf_counter()
    
    while True:
        # Watchdog Tick
        t_now = time.perf_counter()
        dt = t_now - t_last
        iops = int(1.0 / (dt + 1e-9))
        t_last = t_now
        
        # Physics Batch (5 ticks per frame for load)
        avs = tick(5)
        
        # Render
        draw(step, avs, iops)
        step += 1
        
        # Input Check (Non-blocking)
        if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
            break # Exit on keypress

if __name__ == "__main__":
    try: main()
    except: out(RST + CLS + b"\n[HALT]")
