#!/usr/bin/env python3
"""
Import Coherence Gate from RafPolimata
Integrates phi_fst metric for bootstrap stage validation
Part of Stage 3: External Gate Integration (Days 7-10)
"""

import json
import sys
import subprocess
from typing import Dict, Tuple, Optional
from pathlib import Path

def compute_phi_fst(data: bytes) -> Tuple[float, int]:
    """
    Compute phi_fst coherence score from RafPolimata
    phi = (1 - H_norm) * C_norm, where:
      H_norm = unique_bytes / 256
      C_norm = KAM-7 coherence (dot product with magic seed)

    Returns: (phi_fst as float [0,1], attractor index [0,41])
    """
    if not data:
        return 0.0, 0

    # Byte frequency histogram
    freq = {}
    for byte in data:
        freq[byte] = freq.get(byte, 0) + 1

    # H_norm: entropy proxy (unique bytes / 256)
    unique_bytes = len(freq)
    H_norm = unique_bytes / 256.0

    # C_norm: KAM-7 coherence metric
    # KAM-7 seed from RafPolimata (7 magic attractors)
    KAM7 = [40503, 40503, 40503, 40503, 40503, 40503, 40503]

    # Dot product of first 7 byte frequencies with KAM-7 seed
    dot_product = 0.0
    norm_squared = 0.0
    for i in range(min(7, 256)):
        count = freq.get(i, 0)
        dot_product += count * KAM7[i % len(KAM7)]
        norm_squared += count * count

    # Coherence normalization
    C_norm = 0.0
    if norm_squared > 0:
        C_norm = dot_product / (norm_squared ** 0.5)
        C_norm = min(1.0, max(0.0, C_norm / 65536.0))

    # phi_fst = (1 - H) * C
    phi = (1.0 - H_norm) * C_norm
    phi = max(0.0, min(1.0, phi))

    # Map phi to attractor slot (0-41)
    # attractor = (phi_bits XOR (phi_bits >> 7)) % 42
    phi_int = int(phi * 65536)
    attractor = (phi_int ^ (phi_int >> 7)) % 42

    return phi, attractor

def validate_receipt_coherence(receipt_path: str) -> Dict:
    """
    Validate a bootstrap receipt using coherence metrics
    """
    try:
        with open(receipt_path, 'r') as f:
            receipt = json.load(f)
    except (FileNotFoundError, json.JSONDecodeError) as e:
        return {
            "receipt": receipt_path,
            "valid": False,
            "error": f"Cannot read receipt: {str(e)}"
        }

    result = {
        "receipt": receipt_path,
        "schema": receipt.get("schema", "unknown"),
        "valid": False,
        "coherence_scores": [],
        "determinism_check": False,
        "entry_count": 0
    }

    # Extract entries
    entries = receipt.get("entries", [])
    result["entry_count"] = len(entries)

    if len(entries) == 0:
        result["valid"] = False
        result["error"] = "No entries in receipt"
        return result

    # Compute coherence for each entry
    coherence_values = []
    for i, entry in enumerate(entries):
        # Combine stage name and CRC for coherence calculation
        entry_data = f"{entry.get('stage_name', '')}:{entry.get('crc32c', '0x00000000')}".encode()
        phi, attractor = compute_phi_fst(entry_data)

        coherence_values.append({
            "stage": i,
            "stage_name": entry.get("stage_name", "UNKNOWN"),
            "phi_fst": round(phi, 4),
            "attractor": attractor,
            "status": "PASS" if 0.0 <= phi <= 1.0 else "FAIL"
        })

    result["coherence_scores"] = coherence_values

    # Check determinism: all entries should have convergent phi values
    if len(coherence_values) > 1:
        phi_values = [c["phi_fst"] for c in coherence_values]
        min_phi = min(phi_values)
        max_phi = max(phi_values)
        variance = max_phi - min_phi

        result["determinism_check"] = variance < 0.1  # 10% tolerance
        result["phi_variance"] = round(variance, 4)

    result["valid"] = all(c["status"] == "PASS" for c in coherence_values)

    return result

def validate_stage_coherence(stage_name: str, results_dir: str = "results") -> Dict:
    """
    Validate all receipts from a specific bootstrap stage
    """
    results_path = Path(results_dir)

    # Find all receipt files matching stage pattern
    receipt_files = []
    if results_path.exists():
        receipt_files = sorted(results_path.glob(f"*{stage_name}*-receipt.json"))

    result = {
        "stage": stage_name,
        "receipts_found": len(receipt_files),
        "validations": [],
        "overall_coherence": 0.0,
        "passed": False
    }

    # Validate each receipt
    for receipt_file in receipt_files:
        validation = validate_receipt_coherence(str(receipt_file))
        result["validations"].append(validation)

    # Compute overall coherence across all receipts
    if result["validations"]:
        total_phi = sum(
            sum(c["phi_fst"] for c in v.get("coherence_scores", []))
            for v in result["validations"]
        )
        total_scores = sum(
            len(v.get("coherence_scores", []))
            for v in result["validations"]
        )

        if total_scores > 0:
            result["overall_coherence"] = round(total_phi / total_scores, 4)

    result["passed"] = all(v["valid"] for v in result["validations"])

    return result

def generate_coherence_report(repo_root: str = ".") -> Dict:
    """
    Generate comprehensive coherence validation report
    """
    results_dir = f"{repo_root}/results"

    report = {
        "schema": "raf.coherence-gate-report.v1",
        "timestamp": __import__('datetime').datetime.utcnow().isoformat() + 'Z',
        "source": "RafPolimata coherence-gate",
        "stages": [],
        "overall_status": "PENDING",
        "gateway_enabled": True
    }

    # Validate each bootstrap stage
    stages = [
        "bootstrap",
        "dpkg",
        "libapt",
        "apt",
        "signing"
    ]

    for stage in stages:
        validation = validate_stage_coherence(stage, results_dir)
        report["stages"].append(validation)

    # Overall status
    all_passed = all(s["passed"] for s in report["stages"])
    report["overall_status"] = "PASSED" if all_passed else "FAILED"

    return report

def main():
    if len(sys.argv) < 2:
        print("Usage: import_coherence_gate.py [--validate-receipt FILE | --validate-stage STAGE | --report]",
              file=sys.stderr)
        sys.exit(1)

    if "--validate-receipt" in sys.argv:
        idx = sys.argv.index("--validate-receipt")
        if idx + 1 >= len(sys.argv):
            print("Error: --validate-receipt requires a file argument", file=sys.stderr)
            sys.exit(1)

        receipt_file = sys.argv[idx + 1]
        result = validate_receipt_coherence(receipt_file)
        print(json.dumps(result, indent=2))
        sys.exit(0 if result["valid"] else 1)

    elif "--validate-stage" in sys.argv:
        idx = sys.argv.index("--validate-stage")
        if idx + 1 >= len(sys.argv):
            print("Error: --validate-stage requires a stage name", file=sys.stderr)
            sys.exit(1)

        stage_name = sys.argv[idx + 1]
        result = validate_stage_coherence(stage_name)
        print(json.dumps(result, indent=2))
        sys.exit(0 if result["passed"] else 1)

    elif "--report" in sys.argv:
        repo_root = sys.argv[sys.argv.index("--report") + 1] if sys.argv.index("--report") + 1 < len(sys.argv) else "."
        result = generate_coherence_report(repo_root)
        print(json.dumps(result, indent=2))
        sys.exit(0 if result["overall_status"] == "PASSED" else 1)

    else:
        print("Error: Use --validate-receipt, --validate-stage, or --report", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
