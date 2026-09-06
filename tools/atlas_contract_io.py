"""Bounded JSON and the assertions used by the Atlas V1 schemas (stdlib only)."""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import re


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def unique_object(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            raise ValueError("duplicate_json_key")
        value[key] = item
    return value


def decode_json(data):
    def reject_constant(_):
        raise ValueError("non_finite_json_number")
    return json.loads(data, object_pairs_hook=unique_object,
                      parse_constant=reject_constant)


def read_bytes(path: Path, limit: int) -> bytes:
    with path.open("rb") as stream:
        data = stream.read(limit + 1)
    if len(data) > limit:
        raise ValueError("input_byte_limit")
    return data


def load_json(path: Path, limit: int = 1 << 20):
    return decode_json(read_bytes(path, limit))


def canonical(value) -> bytes:
    return (json.dumps(value, ensure_ascii=False, sort_keys=True,
                       separators=(",", ":"), allow_nan=False) + "\n").encode("utf-8")


def write_new(path: Path, data: bytes) -> None:
    # Parent directory is a fresh, private run directory. Never overwrite.
    with path.open("xb") as stream:
        stream.write(data)


def validate_shape(value, schema, errors, root=None, path="$", depth=0):
    """Validate exactly the bounded assertion vocabulary used by our schemas.

    This is not a general Draft-07 validator. Unknown assertion vocabulary is
    rejected so a future schema cannot silently weaken this gate.
    """
    if depth > 64:
        errors.append(path + ": nesting_limit")
        return
    if root is None:
        root = schema
    supported = {"$schema", "$id", "$ref", "title", "description", "type",
                 "required", "properties", "additionalProperties", "items",
                 "definitions", "const", "enum", "pattern", "minimum",
                 "maximum", "minLength", "maxLength", "minItems", "maxItems",
                 "format", "default"}
    if not isinstance(schema, dict) or set(schema) - supported:
        errors.append(path + ": unsupported_schema_assertion")
        return
    if "$ref" in schema:
        ref = schema["$ref"]
        if not ref.startswith("#/definitions/"):
            errors.append(path + ": unsupported_ref")
            return
        target = root.get("definitions", {}).get(ref.split("/")[-1])
        if target is None:
            errors.append(path + ": unresolved_ref")
            return
        validate_shape(value, target, errors, root, path, depth + 1)
        return
    types = {"object": isinstance(value, dict), "array": isinstance(value, list),
             "string": isinstance(value, str), "boolean": type(value) is bool,
             "integer": type(value) is int, "null": value is None,
             "number": type(value) is int or (type(value) is float and math.isfinite(value))}
    wanted = schema.get("type", list(types))
    wanted = [wanted] if isinstance(wanted, str) else wanted
    if not any(types.get(t, False) for t in wanted):
        errors.append(path + ": type_mismatch")
        return
    if "const" in schema and canonical(value) != canonical(schema["const"]):
        errors.append(path + ": const_mismatch")
    if "enum" in schema and not any(canonical(value) == canonical(x) for x in schema["enum"]):
        errors.append(path + ": enum_mismatch")
    if isinstance(value, dict):
        props = schema.get("properties", {})
        for key in schema.get("required", []):
            if key not in value:
                errors.append(path + "." + key + ": required")
        if schema.get("additionalProperties") is False and set(value) - set(props):
            errors.append(path + ": additional_properties")
        for key, item in value.items():
            if key in props:
                validate_shape(item, props[key], errors, root, path + "." + key, depth + 1)
    if isinstance(value, list):
        if not schema.get("minItems", 0) <= len(value) <= schema.get("maxItems", 100000):
            errors.append(path + ": item_limit")
        for i, item in enumerate(value):
            validate_shape(item, schema.get("items", {}), errors, root, f"{path}[{i}]", depth + 1)
    if isinstance(value, str):
        if not schema.get("minLength", 0) <= len(value) <= schema.get("maxLength", 1 << 20):
            errors.append(path + ": string_limit")
        if "pattern" in schema and re.search(schema["pattern"], value) is None:
            errors.append(path + ": pattern_mismatch")
        if schema.get("format") == "date-time":
            from datetime import datetime
            try:
                parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
                if parsed.tzinfo is None:
                    raise ValueError("timezone_required")
            except ValueError:
                errors.append(path + ": invalid_datetime")
    if type(value) in (float, int):
        if not schema.get("minimum", -math.inf) <= value <= schema.get("maximum", math.inf):
            errors.append(path + ": numeric_limit")
