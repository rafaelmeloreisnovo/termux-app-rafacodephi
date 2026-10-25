#!/usr/bin/env python3
"""S16 bootstrap custody gate: detect diagnostic bridges, NOT real apt/pkg."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import sys
from zipfile import ZipFile, BadZipFile

ARCHES=("aarch64","arm","i686","x86_64")
EXPECTED_PACKAGE="com.termux.rafacodephi"
EXPECTED_PAGE="16384"

def check(folder):
    receipts=[]
    for arch in ARCHES:
        originals=folder / f"bootstrap-{arch}.zip"
        embedded=folder / f"rewritten-bootstrap-{arch}.zip"
        if not originals.is_file() or not embedded.is_file():
            raise RuntimeError("BOOTSTRAP_PAIR_MISSING: " + arch)
        ab=originals.read_bytes()
        bb=embedded.read_bytes()
        if ab != bb:
            raise RuntimeError("EMBEDDED_SOURCE_BYTES_MISMATCH: " + arch)
        with ZipFile(embedded) as z:
            bad=z.testzip()
            if bad:
                raise RuntimeError("ZIP_CRC_MISMATCH: "+ arch)
            names=set(z.namelist())
            required={"BOOTSTRAP_INFO","bin/pkg","bin/apt","bin/proot"}
            if required-names:
                raise RuntimeError("MISSING_DIAGNOSTIC_BRIDGE_FILES: "+arch)
            metadata=z.read("BOOTSTRAP_INFO").decode("utf-8")
            details={}
            for line in metadata.splitlines():
                if "=" in line and not line.startswith("#"):
                    key,val=line.split("=",1)
                    details[key.strip()]=val.strip()
            if details.get("TERMUX_PACKAGE_NAME")!=EXPECTED_PACKAGE:
                raise RuntimeError("PACKAGE_METADATA_MISMATCH: "+arch)
            if details.get("TERMUX_PAGE_SIZE")!=EXPECTED_PAGE:
                raise RuntimeError("PAGE_SIZE_METADATA_MISMATCH: "+arch)
            if details.get("RAFCODEPHI_BOOTSTRAP")!="local-ci":
                raise RuntimeError("PROVENANCE_NOT_DIAGNOSTIC: "+arch)
            stub=z.read("bin/apt").decode("utf-8")
            if "real apt backend is not installed yet" not in stub:
                raise RuntimeError("UNRECOGNIZED_APT_BRIDGE: "+arch)
        receipts.append({"arch":arch,"bytes":len(ab),"sha256":sha256(ab).hexdigest(),
                         "source_embedded_byte_equal":True,
                         "real_apt_backend_verified":False})
    return {"schema":"rafaelia.s16.bootstrap_diagnostic_custody.v1",
            "status":"PASS_BRIDGE_ONLY_DIAGNOSTIC",
            "package":EXPECTED_PACKAGE,"expected_page_size":16384,
            "source_built_package_runtime":False,
            "pkg_apt_proot_physical_tested":False,
            "claim_allowed":False,"receipts":receipts}

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--folder",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    a=p.parse_args()
    if a.output.exists():raise RuntimeError("APPEND_ONLY_RECEIPT_EXISTS")
    try: r=check(a.folder)
    except (OSError,UnicodeError,KeyError,ValueError,BadZipFile) as e:
        raise RuntimeError("BOOTSTRAP_READ_FAIL "+type(e).__name__) from e
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.open("x",encoding="utf-8") as f:
        json.dump(r,f,indent=2,sort_keys=True);f.write("\n")
    print("PASS_BRIDGE_ONLY_DIAGNOSTIC: full apt/pkg/proot runtime NOT ATTESTED")

if __name__=="__main__":
    try:main()
    except RuntimeError as e:
        print("FAIL_CLOSED "+str(e),file=sys.stderr)
        raise SystemExit(2)
