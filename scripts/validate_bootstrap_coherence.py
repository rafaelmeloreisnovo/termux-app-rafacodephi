#!/usr/bin/env python3
"""
Bootstrap Coherence Validator (adapted from RafPolimata)
Validates bootstrap stage sequencing and state machine correctness
"""

import json
import sys
import hashlib
from typing import Dict, List, Tuple

# Stage order definition
EXPECTED_STAGES = [
    "PREFIX_EMPTY",
    "INITIALIZED",
    "PAYLOAD_EXTRACTED",
    "DPKG_INSTALLED",
    "APT_CONFIGURED",
    "USER_PACKAGES_READY"
]

def validate_stage_order(receipt_log: Dict) -> Tuple[bool, str]:
    """
    Validate that bootstrap stages appear in deterministic order.
    No out-of-order state changes allowed.
    """
    if "entries" not in receipt_log:
        return False, "Missing 'entries' in receipt log"

    entries = receipt_log["entries"]
    if not entries:
        return False, "Receipt log is empty"

    seen_stages = []
    for entry in entries:
        if "stage" not in entry:
            return False, f"Missing 'stage' in entry: {entry}"

        stage_name = entry.get("stage_name", str(entry.get("stage", "UNKNOWN")))

        # Verify stage appears in expected order
        if stage_name not in EXPECTED_STAGES:
            return False, f"Unknown stage: {stage_name}"

        # Check order constraint: no backward transitions except to PREFIX_EMPTY
        if seen_stages and stage_name != "PREFIX_EMPTY":
            current_index = EXPECTED_STAGES.index(stage_name)
            last_seen = seen_stages[-1]
            last_index = EXPECTED_STAGES.index(last_seen)

            if current_index <= last_index:
                return False, f"Out-of-order transition: {last_seen} → {stage_name}"

        seen_stages.append(stage_name)

    return True, f"Stage order valid: {' → '.join(seen_stages)}"

def validate_status_consistency(receipt_log: Dict) -> Tuple[bool, str]:
    """
    Validate that FAIL status implies rollback or retry.
    No failures should be followed by progression without explanation.
    """
    entries = receipt_log.get("entries", [])

    for i, entry in enumerate(entries):
        status = entry.get("status", 1)

        if status == 0:  # FAIL
            # After a failure, next entry should be either:
            # 1. Same stage (retry)
            # 2. PREFIX_EMPTY (rollback)
            if i + 1 < len(entries):
                next_entry = entries[i + 1]
                next_stage = next_entry.get("stage_name", str(next_entry.get("stage")))
                current_stage = entry.get("stage_name", str(entry.get("stage")))

                if next_stage != current_stage and next_stage != "PREFIX_EMPTY":
                    return False, f"Invalid transition after FAIL: {current_stage} → {next_stage}"

    return True, "Status consistency valid"

def validate_crc_checksums(receipt_log: Dict) -> Tuple[bool, str]:
    """
    Validate that CRC32C checksums are present and non-zero.
    """
    entries = receipt_log.get("entries", [])

    for i, entry in enumerate(entries):
        crc = entry.get("crc32c")

        if crc is None:
            return False, f"Entry {i} missing CRC32C"

        # CRC should be a hex string or integer
        if isinstance(crc, str):
            if not crc.startswith("0x"):
                return False, f"Entry {i} CRC format invalid: {crc}"
        elif isinstance(crc, int):
            if crc == 0:
                return False, f"Entry {i} CRC is zero (no state captured)"

    return True, "CRC32C checksums valid"

def validate_determinism(receipt_logs: List[Dict]) -> Tuple[bool, str]:
    """
    Validate that multiple bootstrap runs produce identical stage sequence and CRCs.
    Cross-run reproducibility check.
    """
    if not receipt_logs:
        return False, "No receipt logs to compare"

    if len(receipt_logs) < 2:
        return True, "Single run (determinism skipped)"

    # Compare all runs against first run
    reference_log = receipt_logs[0]
    reference_stages = [(e.get("stage_name", str(e.get("stage"))), e.get("crc32c"))
                        for e in reference_log.get("entries", [])]

    for run_idx, receipt_log in enumerate(receipt_logs[1:], start=1):
        current_stages = [(e.get("stage_name", str(e.get("stage"))), e.get("crc32c"))
                          for e in receipt_log.get("entries", [])]

        if len(current_stages) != len(reference_stages):
            return False, f"Run {run_idx}: Entry count mismatch: {len(current_stages)} vs {len(reference_stages)}"

        for i, (ref_stage, ref_crc) in enumerate(reference_stages):
            curr_stage, curr_crc = current_stages[i]

            if ref_stage != curr_stage:
                return False, f"Run {run_idx}, entry {i}: Stage mismatch: {ref_stage} vs {curr_stage}"

            if ref_crc != curr_crc:
                return False, f"Run {run_idx}, entry {i}: CRC mismatch: {ref_crc} vs {curr_crc}"

    return True, f"Determinism verified across {len(receipt_logs)} runs"

def validate_no_rollback_loops(receipt_log: Dict) -> Tuple[bool, str]:
    """
    Validate that rollback to PREFIX_EMPTY doesn't cause infinite loops.
    Max 2 rollbacks per stage allowed.
    """
    entries = receipt_log.get("entries", [])
    rollback_count = {}

    for entry in entries:
        stage = entry.get("stage_name", str(entry.get("stage")))

        # Track rollbacks (PREFIX_EMPTY transitions)
        if stage == "PREFIX_EMPTY":
            prev_stage = entry.get("previous_stage", "UNKNOWN")
            key = f"{prev_stage}→{stage}"
            rollback_count[key] = rollback_count.get(key, 0) + 1

            if rollback_count[key] > PROOT_MAX_RESTART_ATTEMPTS:
                return False, f"Rollback loop detected: {key} occurred {rollback_count[key]} times"

    return True, "No infinite rollback loops detected"

PROOT_MAX_RESTART_ATTEMPTS = 2

def validate_receipt_log(receipt_log: Dict, strict: bool = False) -> Dict:
    """
    Validate entire receipt log using all coherence checks.
    """
    results = {
        "schema": receipt_log.get("schema", "unknown"),
        "checks": {},
        "passed": True,
        "message": ""
    }

    # Run all checks
    checks = [
        ("stage_order", validate_stage_order),
        ("status_consistency", validate_status_consistency),
        ("crc_checksums", validate_crc_checksums),
        ("no_rollback_loops", validate_no_rollback_loops)
    ]

    for check_name, check_func in checks:
        passed, message = check_func(receipt_log)
        results["checks"][check_name] = {
            "passed": passed,
            "message": message
        }
        if not passed:
            results["passed"] = False

    if results["passed"]:
        results["message"] = "All coherence checks passed"
    else:
        failed_checks = [k for k, v in results["checks"].items() if not v["passed"]]
        results["message"] = f"Coherence validation failed: {', '.join(failed_checks)}"

    return results

def main():
    if len(sys.argv) < 2:
        print("Usage: validate_bootstrap_coherence.py <receipt.json> [--strict]", file=sys.stderr)
        sys.exit(1)

    receipt_path = sys.argv[1]
    strict = "--strict" in sys.argv

    try:
        with open(receipt_path, 'r') as f:
            receipt_log = json.load(f)
    except FileNotFoundError:
        print(f"Error: Receipt file not found: {receipt_path}", file=sys.stderr)
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON: {e}", file=sys.stderr)
        sys.exit(1)

    result = validate_receipt_log(receipt_log, strict=strict)

    # Output results
    print(json.dumps(result, indent=2))

    if result["passed"]:
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
