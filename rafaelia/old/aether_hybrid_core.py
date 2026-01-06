#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AETHER HYBRID CORE v4.1
Dual Pipeline:
 - AETHER_PY  (dados pequenos, simbólico)
 - AETHER_TURBO_BLAKE2B (dados grandes, delega BLAKE)
 + IRON_SHA256 e BLAKE2B_256 como referência

Compatível: Termux / Linux / Android 15
"""

import os
import time
import argparse
import hashlib

CHUNK_SMALL = 1024 * 1024       # 1MB
CHUNK_BIG   = 4 * 1024 * 1024   # 4MB

# ---------- AETHER (conceitual) ----------
MASK64 = (1 << 64) - 1
PRIME = 0x100000001b3

def aether_update(h: int, data: bytes) -> int:
    for b in data:
        h = (h ^ b) & MASK64
        h = (h * PRIME) & MASK64
        h ^= (h >> 33)
    return h

def aether_small_stream(stream, width_bits=256, chunk_size=CHUNK_SMALL):
    h = 0xcbf29ce484222325
    total = 0
    start = time.time()

    while True:
        chunk = stream.read(chunk_size)
        if not chunk: break
        h = aether_update(h, chunk)
        total += len(chunk)

    end = time.time()

    hex_full = f"{h:016x}"
    hex_len  = width_bits // 4
    hex_val = hex_full[:hex_len]

    return {
        "engine":    "AETHER_PY",
        "width_bits": width_bits,
        "bytes":     total,
        "seconds":   end - start,
        "hex":       hex_val,
    }

# ---------- IRON (SHA256) ----------
def hash_iron_stream(stream, chunk_size=CHUNK_BIG):
    ctx = hashlib.sha256()
    total = 0
    start = time.time()

    while True:
        chunk = stream.read(chunk_size)
        if not chunk: break
        ctx.update(chunk)
        total += len(chunk)

    end = time.time()

    return {
        "engine":    "IRON_SHA256",
        "width_bits": 256,
        "bytes":     total,
        "seconds":   end - start,
        "hex":       ctx.hexdigest(),
    }

# ---------- BLAKE2b ----------
def hash_blake_stream(stream, chunk_size=CHUNK_BIG):
    ctx = hashlib.blake2b(digest_size=32)
    total = 0
    start = time.time()

    while True:
        chunk = stream.read(chunk_size)
        if not chunk: break
        ctx.update(chunk)
        total += len(chunk)

    end = time.time()

    return {
        "engine":    "BLAKE2B_256",
        "width_bits": 256,
        "bytes":     total,
        "seconds":   end - start,
        "hex":       ctx.hexdigest(),
    }

# ---------- TURBO (delegado ao BLAKE) ----------
def aether_big_stream(stream, width_bits=256, chunk_size=CHUNK_BIG):
    if width_bits < 64: width_bits = 64
    if width_bits > 256: width_bits = 256

    base = hash_blake_stream(stream, chunk_size=chunk_size)
    base["engine"] = "AETHER_TURBO_BLAKE2B"
    base["width_bits"] = width_bits

    if width_bits < 256:
        hex_len = width_bits // 4
        base["hex"] = base["hex"][:hex_len]

    return base

# Seleção automática
def hash_aether_stream(stream, width_bits=256):
    pos = stream.seek(0, os.SEEK_END)
    stream.seek(0)

    if pos <= (8 * 1024 * 1024): # <= 8MB
        return aether_small_stream(stream, width_bits=width_bits)
    else:
        return aether_big_stream(stream, width_bits=width_bits)

# ---------- Seleção CLI ----------
def hash_file(target, engine="aether", width_bits=256):
    with open(target, "rb") as f:
        if engine == "iron": return hash_iron_stream(f)
        if engine == "blake": return hash_blake_stream(f)
        return hash_aether_stream(f, width_bits=width_bits)

def hash_text(text, engine="aether", width_bits=256):
    import io
    return hash_filelike(io.BytesIO(text.encode()), engine, width_bits)

def hash_filelike(stream, engine, width_bits):
    if engine == "iron": return hash_iron_stream(stream)
    if engine == "blake": return hash_blake_stream(stream)
    return hash_aether_stream(stream, width_bits)

# ---------- CLI ----------
def _main():
    ap = argparse.ArgumentParser()
    ap.add_argument("target", help="File or text input")
    ap.add_argument("-e", "--engine", default="aether", choices=["aether","iron","blake"])
    ap.add_argument("-w", "--width", type=int, default=256)
    ap.add_argument("-f", "--file", action="store_true", help="Interpret target as file")
    args = ap.parse_args()

    if args.file:
        if not os.path.exists(args.target):
            print(f"[ERROR] Arquivo não encontrado: {args.target}")
            return
        res = hash_file(args.target, engine=args.engine, width_bits=args.width)
    else:
        res = hash_text(args.target, engine=args.engine, width_bits=args.width)

    speed = (res["bytes"] / (1024*1024)) / res["seconds"] if res["seconds"]>0 else 0.0
    print(f"Engine     : {res['engine']}")
    print(f"Width      : {res['width_bits']} bits")
    print(f"Bytes      : {res['bytes']}  (~{res['bytes']/1048576:.4f} MB)")
    print(f"Time (s)   : {res['seconds']:.6f}")
    print(f"Speed      : {speed:.2f} MB/s")
    print(f"Hex digest : {res['hex']}")

if __name__ == "__main__":
    _main()
