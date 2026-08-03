#!/usr/bin/env python3
import argparse,json,pathlib,sys
P=argparse.ArgumentParser();P.add_argument("--matrix",required=True);P.add_argument("--receipts",nargs="*");a=P.parse_args()
m=json.loads(pathlib.Path(a.matrix).read_text());e=[];receipts=[]
for p in a.receipts or []:receipts.append(json.loads(pathlib.Path(p).read_text()))
req=m["required_receipt_fields"]
for r in receipts:
 for k in req:
  if r.get(k) in (None,""):e.append(f"{r.get('role','?')}:missing:{k}")
roles={r.get("role") for r in receipts};fps={r.get("device_fingerprint") for r in receipts}
if len(roles)!=len(receipts):e.append("duplicate_role")
if len(fps)!=len(receipts):e.append("duplicate_device")
count=len(receipts);state=m["promotion"].get(str(count),"FAIL")
if state=="DUAL_ARM_DEVICE_PROOF" and roles!={"arm32-legacy","arm64-modern"}:e.append("dual_role_set_invalid")
if m.get("release_claim_allowed") is not False:e.append("release_claim_allowed_must_be_false")
print(json.dumps({"state":"FAIL" if e else state,"errors":e,"receipt_count":count,"claim_allowed":False,"release_claim_allowed":False},indent=2))
sys.exit(1 if e else 0)
