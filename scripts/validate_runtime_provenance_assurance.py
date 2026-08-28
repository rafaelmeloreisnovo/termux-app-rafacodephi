#!/usr/bin/env python3
import json
from pathlib import Path

P = Path("docs/assurance/runtime-provenance-assurance.v1.json")

def die(msg):
    raise SystemExit(f"FAIL: {msg}")

m = json.loads(P.read_text(encoding="utf-8"))
if m.get("schema") != "rafaelia.android-runtime-provenance-assurance.v1":
    die("unexpected schema")
if m.get("claim_allowed") is not False:
    die("claim_allowed must remain false")
lic = m["license"]
if lic.get("flatten_component_licenses") is not False:
    die("component licenses cannot be flattened")
if not Path("LICENSE.md").is_file():
    die("LICENSE.md missing")
rt = m["runtime"]
if rt.get("ci_build_is_physical_runtime") is not False:
    die("CI build cannot equal physical runtime")
if rt.get("provider_dispatch_is_guest_execution") is not False:
    die("provider dispatch cannot equal guest execution")
if rt.get("armv7_required") is not True:
    die("armeabi-v7a invariant lost")
if not any(g["id"] == "PHYSICAL_DEVICE_RUNTIME" and g["state"] == "TOKEN_VAZIO" for g in m["gates"]):
    die("physical runtime must remain open until a successor receipt")
if not any(g["urgency"] == "P0" for g in m["gaps"]):
    die("P0 gaps unexpectedly absent")
if m["privacy"].get("state") != "FAIL_CLOSED":
    die("privacy must fail closed")
if not m["rollback"].get("available"):
    die("rollback required")
print("PASS: Termux runtime/provenance assurance remains fail-closed")
