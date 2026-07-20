#!/usr/bin/env python3
"""Index loose/historical artifacts without promoting, moving or deleting them."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
POLICY_PATH = ROOT / "configs/loose-artifact-policy.json"


def classify(path: Path) -> tuple[str, str]:
    suffix = path.suffix.lower()
    if suffix in {".c", ".h"}:
        return "c_source", "CANDIDATE_SOURCE"
    if path.suffix in {".s", ".S"} or suffix in {".asm"}:
        return "asm_source", "CANDIDATE_SOURCE"
    if suffix in {".sh", ".py"}:
        return "script", "REVIEW_REQUIRED"
    if suffix in {".md", ".txt", ".html"}:
        return "document", "CANDIDATE_DOCUMENT"
    if suffix in {".json", ".yaml", ".yml"}:
        return "data", "REVIEW_REQUIRED"
    return "unknown", "QUARANTINE"


def content_hash(path: Path) -> tuple[str, int]:
    digest = hashlib.sha256()
    size = 0
    with path.open("rb") as handle:
        while True:
            chunk = handle.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
            size += len(chunk)
    return digest.hexdigest(), size


def discover(policy: dict[str, object]) -> list[Path]:
    skip_dirs = set(policy.get("skip_directories", []))
    paths: set[Path] = set()

    for root_name in policy.get("scan_roots", []):
        base = ROOT / str(root_name)
        if not base.is_dir():
            continue
        for path in base.rglob("*"):
            if not path.is_file():
                continue
            relative = path.relative_to(ROOT)
            if any(part in skip_dirs for part in relative.parts):
                continue
            paths.add(path)

    if policy.get("scan_root_files"):
        extensions = set(policy.get("root_extensions", []))
        exclusions = set(policy.get("root_exclusions", []))
        for path in ROOT.iterdir():
            if path.is_file() and path.name not in exclusions and path.suffix in extensions:
                paths.add(path)

    return sorted(paths, key=lambda item: item.relative_to(ROOT).as_posix())


def build_inventory(policy: dict[str, object]) -> dict[str, object]:
    artifacts: list[dict[str, object]] = []
    by_hash: defaultdict[str, list[int]] = defaultdict(list)
    targets = policy.get("targets", {})

    for path in discover(policy):
        relative = path.relative_to(ROOT).as_posix()
        digest, size = content_hash(path)
        object_type, status = classify(path)
        artifact_id = "LOOSE-" + hashlib.sha256(relative.encode("utf-8")).hexdigest()[:16].upper()
        record = {
            "artifact_id": artifact_id,
            "path": relative,
            "object_type": object_type,
            "content_sha256": digest,
            "size_bytes": size,
            "status": status,
            "build_consumer": "TOKEN_VAZIO",
            "integration_target": targets.get(object_type, targets.get("unknown", "archive/quarantine")),
            "evidence_state": "SOURCE_PRESENT_ONLY",
            "claim_allowed": False,
            "next_action": "REVIEW_PROVENANCE_LICENSE_AND_REFERENCES"
        }
        by_hash[digest].append(len(artifacts))
        artifacts.append(record)

    duplicate_groups: list[dict[str, object]] = []
    for digest, indexes in sorted(by_hash.items()):
        if len(indexes) < 2:
            continue
        group_paths = [artifacts[index]["path"] for index in indexes]
        duplicate_groups.append({"content_sha256": digest, "paths": group_paths})
        for index in indexes:
            artifacts[index]["status"] = "DUPLICATE_CONTENT"
            artifacts[index]["next_action"] = "SELECT_CANONICAL_OR_ARCHIVE_WITHOUT_DELETING_HISTORY"

    return {
        "schema": "raf.loose-artifact-inventory.v1",
        "repository": "rafaelmeloreisnovo/termux-app-rafacodephi",
        "policy": str(POLICY_PATH.relative_to(ROOT)),
        "invariant": policy.get("invariant"),
        "automatic_move": False,
        "automatic_delete": False,
        "automatic_claim_promotion": False,
        "artifact_count": len(artifacts),
        "duplicate_group_count": len(duplicate_groups),
        "artifacts": artifacts,
        "duplicate_groups": duplicate_groups
    }


def validate_inventory(inventory: dict[str, object]) -> list[str]:
    failures: list[str] = []
    artifacts = inventory.get("artifacts", [])
    ids = [item.get("artifact_id") for item in artifacts]
    paths = [item.get("path") for item in artifacts]

    if len(ids) != len(set(ids)):
        failures.append("duplicate artifact_id")
    if len(paths) != len(set(paths)):
        failures.append("duplicate path")
    for item in artifacts:
        required = {
            "artifact_id", "path", "object_type", "content_sha256", "size_bytes",
            "status", "build_consumer", "integration_target", "evidence_state",
            "claim_allowed", "next_action"
        }
        missing = sorted(required.difference(item))
        if missing:
            failures.append(f"{item.get('path')}: missing {missing}")
        if item.get("claim_allowed") is not False:
            failures.append(f"{item.get('path')}: loose artifact claim promoted")
        if item.get("evidence_state") != "SOURCE_PRESENT_ONLY":
            failures.append(f"{item.get('path')}: invalid evidence state")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    parser.add_argument("--validate", action="store_true")
    parser.add_argument("--summary", action="store_true")
    args = parser.parse_args()

    try:
        policy = json.loads(POLICY_PATH.read_text(encoding="utf-8"))
        inventory = build_inventory(policy)
        failures = validate_inventory(inventory) if args.validate else []
    except (OSError, json.JSONDecodeError) as exc:
        print(json.dumps({"schema": "raf.loose-artifact-inventory.v1", "status": "FAIL", "error": str(exc)}))
        return 1

    inventory["status"] = "PASS" if not failures else "FAIL"
    inventory["validation_failures"] = failures

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(inventory, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    if args.summary:
        summary = {
            "schema": inventory["schema"],
            "status": inventory["status"],
            "artifact_count": inventory["artifact_count"],
            "duplicate_group_count": inventory["duplicate_group_count"],
            "automatic_move": inventory["automatic_move"],
            "automatic_delete": inventory["automatic_delete"],
            "automatic_claim_promotion": inventory["automatic_claim_promotion"],
            "validation_failures": failures
        }
        json.dump(summary, sys.stdout, ensure_ascii=False, indent=2)
    else:
        json.dump(inventory, sys.stdout, ensure_ascii=False, indent=2)
    sys.stdout.write("\n")
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
