#!/usr/bin/env python3
"""
Import Determinism Gate from RafPolimata
Validates reproducibility across bootstrap runs using phi_fst metric
Part of Stage 3: External Gate Integration (Days 7-10)
"""

import json
import sys
import hashlib
from typing import Dict, List, Tuple
from pathlib import Path

def compute_receipt_crc_hash(receipt: Dict) -> Tuple[str, str]:
    """
    Compute SHA256 hash of receipt entry for determinism comparison
    Returns (crc32c_value, sha256_hash)
    """
    crc = receipt.get("crc32c", "0x00000000")
    combined = f"{receipt.get('stage_name', '')}:{crc}".encode()
    sha256 = hashlib.sha256(combined).hexdigest()
    return str(crc), sha256

def compare_receipt_logs(run1: Dict, run2: Dict) -> Tuple[bool, str, Dict]:
    """
    Compare two bootstrap receipt logs for deterministic reproducibility
    Returns (is_deterministic, message, details)
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
    Main determinism validation gate from RafPolimata
    Validates multiple bootstrap runs produce identical results
    """
    result = {
        "schema": "raf.determinism-gate.v1",
        "run_count": len(receipt_logs),
        "deterministic": False,
        "comparisons": [],
        "reproducibility_score": 0.0,
        "message": ""
    }

    if len(receipt_logs) < 2:
        result["message"] = "Single run (determinism skipped)"
        result["deterministic"] = True
        result["reproducibility_score"] = 1.0  # No comparison needed
        return result

    # Compare all runs pairwise
    reference = receipt_logs[0]
    all_deterministic = True
    successful_comparisons = 0

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

        if match:
            successful_comparisons += 1
        else:
            all_deterministic = False

    result["deterministic"] = all_deterministic

    # Compute reproducibility score
    if len(receipt_logs) > 1:
        result["reproducibility_score"] = successful_comparisons / (len(receipt_logs) - 1)

    if all_deterministic:
        result["message"] = f"✓ Determinism verified across {len(receipt_logs)} runs"
    else:
        failed_comparisons = [c for c in result["comparisons"] if not c["deterministic"]]
        result["message"] = f"✗ Determinism violated in {len(failed_comparisons)} comparison(s)"

    return result

def load_receipt_logs(receipt_paths: List[str]) -> List[Dict]:
    """
    Load multiple bootstrap receipt logs
    """
    receipt_logs = []

    for path in receipt_paths:
        try:
            with open(path, 'r') as f:
                receipt_logs.append(json.load(f))
        except FileNotFoundError:
            print(f"Warning: Receipt file not found: {path}", file=sys.stderr)
        except json.JSONDecodeError as e:
            print(f"Warning: Invalid JSON in {path}: {e}", file=sys.stderr)

    return receipt_logs

def find_receipt_files(results_dir: str = "results", pattern: str = "*-receipt.json") -> List[str]:
    """
    Find all receipt files in results directory
    """
    results_path = Path(results_dir)
    if not results_path.exists():
        return []

    return sorted(str(p) for p in results_path.glob(pattern))

def generate_determinism_report(repo_root: str = ".") -> Dict:
    """
    Generate determinism validation report for all bootstrap stages
    """
    results_dir = f"{repo_root}/results"

    report = {
        "schema": "raf.determinism-validation-report.v1",
        "timestamp": __import__('datetime').datetime.utcnow().isoformat() + 'Z',
        "source": "RafPolimata determinism-gate",
        "stages": [],
        "overall_deterministic": False,
        "reproducibility_index": 0.0
    }

    # Stages to validate
    stage_patterns = {
        "bootstrap": "bootstrap-*-receipt.json",
        "dpkg": "dpkg-*-receipt.json",
        "libapt": "libapt-*-receipt.json",
        "apt": "apt-*-receipt.json",
        "signing": "package-signing-*-receipt.json"
    }

    reproducibility_scores = []

    for stage_name, pattern in stage_patterns.items():
        receipt_files = find_receipt_files(results_dir, pattern)

        if not receipt_files:
            report["stages"].append({
                "stage": stage_name,
                "receipt_count": 0,
                "deterministic": True,
                "message": "No receipts found (skipped)"
            })
            continue

        receipt_logs = load_receipt_logs(receipt_files)

        if not receipt_logs:
            report["stages"].append({
                "stage": stage_name,
                "receipt_count": 0,
                "deterministic": False,
                "message": "Could not load receipt files"
            })
            continue

        validation = validate_determinism_gate(receipt_logs)
        validation["stage"] = stage_name
        report["stages"].append(validation)

        if validation["deterministic"]:
            reproducibility_scores.append(validation["reproducibility_score"])

    # Overall status
    all_deterministic = all(s.get("deterministic", False) for s in report["stages"])
    report["overall_deterministic"] = all_deterministic

    if reproducibility_scores:
        report["reproducibility_index"] = sum(reproducibility_scores) / len(reproducibility_scores)

    return report

def main():
    if len(sys.argv) < 2:
        print("Usage: import_determinism_gate.py [--logs LOG1 LOG2 ... | --report]",
              file=sys.stderr)
        sys.exit(1)

    if "--logs" in sys.argv:
        idx = sys.argv.index("--logs")
        log_files = sys.argv[idx + 1:]

        if not log_files:
            print("Error: --logs requires at least one file", file=sys.stderr)
            sys.exit(1)

        receipt_logs = load_receipt_logs(log_files)

        if not receipt_logs:
            print("Error: Could not load any receipt logs", file=sys.stderr)
            sys.exit(1)

        result = validate_determinism_gate(receipt_logs)
        print(json.dumps(result, indent=2))
        sys.exit(0 if result["deterministic"] else 1)

    elif "--report" in sys.argv:
        repo_root = "."
        idx = sys.argv.index("--report")
        if idx + 1 < len(sys.argv) and not sys.argv[idx + 1].startswith("--"):
            repo_root = sys.argv[idx + 1]

        result = generate_determinism_report(repo_root)
        print(json.dumps(result, indent=2))
        sys.exit(0 if result["overall_deterministic"] else 1)

    else:
        print("Error: Use --logs or --report", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
