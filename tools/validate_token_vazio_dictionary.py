#!/usr/bin/env python3
"""Validate the canonical typed TOKEN_VAZIO dictionary.

Tooling only: stdlib, no runtime dependency. Unknown repository usages are
reported; they are not silently promoted or rewritten.
"""
from __future__ import annotations
import argparse
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DICT = ROOT / "configs" / "token-vazio-dictionary.v1.json"
SCHEMA = ROOT / "schemas" / "token-vazio-instance.v1.schema.json"
CODE_RE = re.compile(r"^TOKEN_VAZIO(?:_[A-Z0-9]+)+$")
SCAN_RE = re.compile(r"\bTOKEN_VAZIO(?:_[A-Z0-9]+)*\b")
SCAN_EXTS = {".md",".txt",".json",".jsonl",".yaml",".yml",".c",".h",".cc",".cpp",".hpp",".inc",".S",".s",".py",".sh"}

def load() -> tuple[dict, dict]:
    return json.loads(DICT.read_text(encoding="utf-8")), json.loads(SCHEMA.read_text(encoding="utf-8"))

def validate_dictionary(data: dict) -> list[str]:
    errors: list[str] = []
    if data.get("schema") != "rafaelia.token-vazio-dictionary/v1":
        errors.append("dictionary schema mismatch")
    types = data.get("types")
    if not isinstance(types, list) or not types:
        return errors + ["types must be a non-empty list"]
    seen: set[str] = set()
    required = {"code","family","definition","use_when","closure_gate","default_urgency","claim_allowed"}
    for idx, item in enumerate(types):
        if not isinstance(item, dict):
            errors.append(f"types[{idx}] is not an object")
            continue
        missing = sorted(required - set(item))
        if missing:
            errors.append(f"types[{idx}] missing: {','.join(missing)}")
            continue
        code = item["code"]
        if not isinstance(code, str) or not CODE_RE.fullmatch(code):
            errors.append(f"invalid code: {code!r}")
        elif code in seen:
            errors.append(f"duplicate code: {code}")
        seen.add(code)
        if item["claim_allowed"] is not False:
            errors.append(f"{code}: claim_allowed must be false")
        if item["default_urgency"] not in {"low","medium","high","critical"}:
            errors.append(f"{code}: invalid urgency")
        for key in ("family","definition","use_when","closure_gate"):
            if not isinstance(item[key], str) or not item[key].strip():
                errors.append(f"{code}: empty {key}")
    return errors

def scan_unknown(known: set[str]) -> dict:
    counts: Counter[str] = Counter()
    paths: dict[str, list[str]] = {}
    for path in ROOT.rglob("*"):
        if not path.is_file() or path.suffix not in SCAN_EXTS:
            continue
        if any(part in {".git","build",".gradle"} for part in path.parts):
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for token in SCAN_RE.findall(text):
            counts[token] += 1
            paths.setdefault(token, [])
            rel = path.relative_to(ROOT).as_posix()
            if rel not in paths[token] and len(paths[token]) < 8:
                paths[token].append(rel)
    unknown = {k:v for k,v in sorted(counts.items()) if k not in known and k != "TOKEN_VAZIO"}
    return {
        "legacy_untyped_occurrences": counts.get("TOKEN_VAZIO", 0),
        "unknown_typed_tokens": unknown,
        "sample_paths": {k:paths[k] for k in unknown},
    }

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--scan", action="store_true")
    ap.add_argument("--strict-unknown", action="store_true")
    args = ap.parse_args()
    data, schema = load()
    errors = validate_dictionary(data)
    if schema.get("$id") != "rafaelia://schemas/token-vazio-instance.v1.schema.json":
        errors.append("instance schema id mismatch")
    known = {x["code"] for x in data.get("types", []) if isinstance(x, dict) and "code" in x}
    report = {
        "schema":"rafaelia.token-vazio-dictionary-validation/v1",
        "dictionary_types":len(known),
        "dictionary_errors":errors,
        "claim_allowed":False,
    }
    if args.scan:
        report.update(scan_unknown(known))
        if args.strict_unknown and report["unknown_typed_tokens"]:
            errors.append("unknown typed TOKEN_VAZIO usages exist")
    print(json.dumps(report, indent=2, sort_keys=True))
    return 1 if errors else 0

if __name__ == "__main__":
    raise SystemExit(main())
