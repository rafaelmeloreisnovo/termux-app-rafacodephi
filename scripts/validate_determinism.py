#!/usr/bin/env python3
"""
Bootstrap Determinism Validator (RafPolimata phi_fst pattern)
Ensures reproducibility: multiple runs → identical CRCs and state hashes
"""

import json
import sys
import hashlib
from typing import Dict, List, Tuple

def phi_fst_compute(data: bytes) -> float:
    """
    Simplified phi_fst (coherence metric from RafPolimata)
    Computes normalized entropy and KAM-7 coherence.
    Returns value in [0, 1] representing determinism score.
    """
    if not data:
        return 0.0

    # Entropy calculation: unique bytes / 256
    unique_bytes = len(set(data))
    entropy = unique_bytes / 256.0

    # Frequency analysis (simplified KAM coherence)
    freq = {}
    for byte in data:
        freq[byte] = freq.get(byte, 0) + 1

    # KAM-7 seed (7 magic attractors from RafPolimata)
    kam_seed = [0x9e3779b97f4a7c15, 0xbf58476d1ce4e5b9,
                0x94d049bb133111eb, 0x2b992ddfa23249d6,
                0x1211498541221912, 0x0c15aba6a8406e7e,
                0xdc0a4d3de6e4d5db]

    # Coherence: dot product of freq vector against kam_seed
    coherence = 0.0
    for i, count in freq.items():
        if i < len(kam_seed):
            coherence += (count * kam_seed[i % len(kam_seed)])

    coherence = abs(coherence) if coherence != 0 else 1.0
    coherence_norm = coherence / (len(data) ** 2 + 1)

    # phi_fst = (1 - entropy_norm) * coherence_norm
    phi = (1.0 - entropy) * coherence_norm
    return max(0.0, min(1.0, phi))

def compute_receipt_crc_hash(receipt: Dict) -> Tuple[str, str]:
    """
    Compute SHA256 hash of receipt entry for comparison.
    Returns (crc32c_value, sha256_hash).
    """
    crc = receipt.get("crc32c", "0x00000000")

    # Combine stage name and CRC for determinism check
    combined = f"{receipt.get('stage_name', '')}:{crc}".encode()
    sha256 = hashlib.sha256(combined).hexdigest()

    return str(crc), sha256

def compare_receipt_logs(run1: Dict, run2: Dict) -> Tuple[bool, str, Dict]:
    """
    Compare two bootstrap receipt logs for deterministic reproducibility.
    """
    result = {
        "entries_match": False,
        "crc_match": False,
        "hash_match": False,
        "stage_sequence_match": False,
        "differences": []
    }

    entries1 = run1.get("entries", [])
    entries2 = run2.get("entries", [])

    # Check entry count
    if len(entries1) != len(entries2):
        result["differences"].append(f"Entry count mismatch: {len(entries1)} vs {len(entries2)}")
        return False, "Entry count mismatch", result

    result["entries_match"] = True

    # Compare each entry
    crcs_match = True
    hashes_match = True
    stages_match = True

    for i, (e1, e2) in enumerate(zip(entries1, entries2)):
        stage1 = e1.get("stage_name", str(e1.get("stage")))
        stage2 = e2.get("stage_name", str(e2.get("stage")))

        if stage1 != stage2:
            result["differences"].append(f"Entry {i}: Stage mismatch: {stage1} vs {stage2}")
            stages_match = False

        crc1 = str(e1.get("crc32c", "0x00000000"))
        crc2 = str(e2.get("crc32c", "0x00000000"))

        if crc1 != crc2:
            result["differences"].append(f"Entry {i}: CRC mismatch: {crc1} vs {crc2}")
            crcs_match = False

        # Hash comparison
        _, hash1 = compute_receipt_crc_hash(e1)
        _, hash2 = compute_receipt_crc_hash(e2)

        if hash1 != hash2:
            hashes_match = False

    result["crc_match"] = crcs_match
    result["hash_match"] = hashes_match
    result["stage_sequence_match"] = stages_match

    if crcs_match and stages_match:
        return True, "Determinism verified: runs are identical", result
    else:
        return False, "Determinism violated: runs differ", result

def validate_determinism_gate(receipt_logs: List[Dict], strict: bool = False) -> Dict:
    """
    Main determinism validation gate.
    Validates multiple bootstrap runs produce identical results.
    """
    result = {
        "schema": "raf.determinism-gate.v1",
        "run_count": len(receipt_logs),
        "deterministic": False,
        "comparisons": [],
        "phi_fst_scores": [],
        "message": ""
    }

    if len(receipt_logs) < 2:
        result["message"] = "Single run (determinism skipped)"
        result["deterministic"] = True
        return result

    # Compute phi_fst scores for each run
    for i, receipt_log in enumerate(receipt_logs):
        entries_json = json.dumps(receipt_log.get("entries", []))
        phi = phi_fst_compute(entries_json.encode())
        result["phi_fst_scores"].append({
            "run": i,
            "phi_fst": phi,
            "interpretation": "high coherence" if phi > 0.7 else "low coherence"
        })

    # Compare all runs pairwise
    reference = receipt_logs[0]
    all_deterministic = True

    for i in range(1, len(receipt_logs)):
        comparison_result = {
            "run1": 0,
            "run2": i,
            "deterministic": False,
            "message": ""
        }

        match, msg, details = compare_receipt_logs(reference, receipt_logs[i])
        comparison_result["deterministic"] = match
        comparison_result["message"] = msg
        comparison_result["details"] = details

        result["comparisons"].append(comparison_result)

        if not match:
            all_deterministic = False

    result["deterministic"] = all_deterministic

    if all_deterministic:
        result["message"] = f"✓ Determinism verified across {len(receipt_logs)} runs"
    else:
        failed_comparisons = [c for c in result["comparisons"] if not c["deterministic"]]
        result["message"] = f"✗ Determinism violated in {len(failed_comparisons)} comparison(s)"

    return result

def main():
    if len(sys.argv) < 2:
        print("Usage: validate_determinism.py <run1.json> [run2.json] [...] [--strict]",
              file=sys.stderr)
        sys.exit(1)

    strict = "--strict" in sys.argv
    receipt_paths = [arg for arg in sys.argv[1:] if not arg.startswith("--")]

    if len(receipt_paths) < 1:
        print("Error: At least one receipt file required", file=sys.stderr)
        sys.exit(1)

    # Load all receipt logs
    receipt_logs = []
    for path in receipt_paths:
        try:
            with open(path, 'r') as f:
                receipt_logs.append(json.load(f))
        except FileNotFoundError:
            print(f"Error: Receipt file not found: {path}", file=sys.stderr)
            sys.exit(1)
        except json.JSONDecodeError as e:
            print(f"Error: Invalid JSON in {path}: {e}", file=sys.stderr)
            sys.exit(1)

    # Run validation
    result = validate_determinism_gate(receipt_logs, strict=strict)

    # Output results
    print(json.dumps(result, indent=2))

    if result["deterministic"]:
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
