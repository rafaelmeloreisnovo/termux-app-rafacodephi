#!/usr/bin/env python3
"""No-network materializer proof: raw Browser.sh is immutable.

Only host-side tests use Python/CC; generated BrowserRaf remains freestanding.
TLS stays non-operational; successful ClientHello serialization != HTTPS readiness.
"""
from __future__ import annotations

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Browser.sh"
MATERIALIZER = ROOT / "scripts/materialize_browser_fail_closed.py"

def require(value: bool, msg: str) -> None:
    if not value:
        raise AssertionError(msg)

def extract(script: str, name: str) -> str:
    marker = f'cat > "$D/{name}" << \x27F\x27\n'
    require(script.count(marker) == 1, f"missing/ambiguous materialized {name}")
    rest = script.split(marker, 1)[1]
    require("\nF\n" in rest, f"no heredoc end for {name}")
    return rest.split("\nF\n", 1)[0] + "\n"

def main() -> int:
    source_before = SOURCE.read_bytes()
    cc = shutil.which(os.environ.get("CC", "cc"))
    require(cc is not None, "host test compiler unavailable: not a PASS")
    with tempfile.TemporaryDirectory(prefix="raf_browser_entropy_") as td:
        out = Path(td) / "materialized.sh"
        r = subprocess.run(
            [sys.executable, str(MATERIALIZER), "--output", str(out)],
            cwd=ROOT, text=True, capture_output=True, timeout=20,
        )
        require(r.returncode == 0, "materializer failed: " + r.stdout + r.stderr)
        script = out.read_text(encoding="utf-8")
        require(SOURCE.read_bytes() == source_before, "raw historical Browser.sh mutated")
        require("ctx->port=80u;ctx->use_tls=0;" in script, "HTTP default lost")
        require("[FALLBACK] Usando HTTP para demo" not in script, "HTTPS downgrade!")
        require("if(TLS_INIT(&_TLS)!=0)" in script, "entropy failure not gated")
        require("if(!chlen)" in script, "invalid ClientHello is sent")
        require("return-3;" in script, "entropy/length error not terminal")

        for name in ("br_types.h", "br_sys.h", "br_entropy.h", "br_tls.h"):
            (Path(td) / name).write_text(extract(script, name), encoding="utf-8")
        entropy = (Path(td) / "br_entropy.h").read_text(encoding="utf-8")
        tls = (Path(td) / "br_tls.h").read_text(encoding="utf-8")
        for abi in ("384u", "278u", "318u"):
            require(f"BR_NR_GETRANDOM {abi}" in entropy, "getrandom ABI missing " + abi)
        for marker in ("BR_RANDOM_FILL", "MC0(p,n)", "if(got==-(s32)BR_EINTR)continue"):
            require(marker in entropy, "entropy guard missing " + marker)
        require("0xDEADBEEF" not in tls and "AI u32 PRNG" not in tls, "seeded PRNG retained")
        require('#include "br_entropy.h"' in tls, "entropy provider not wired")

        harness = Path(td) / "test.c"
        harness.write_text(r'''
#include "br_tls.h"
static u8 b[256];
int main(void){
    TLSCtx t;
    if(TLS_INIT(&t)!=0)return 1;
    for(u32 cap=0u;cap<103u;cap++){
        for(u32 i=0u;i<256u;i++)b[i]=0xA5u;
        if(TLS_BUILD_CLIENT_HELLO(&t,"example.com",b,cap)!=0u)return 2;
        for(u32 i=0u;i<256u;i++)if(b[i]!=0xA5u)return 3;
    }
    for(u32 i=0u;i<256u;i++)b[i]=0xA5u;
    if(TLS_BUILD_CLIENT_HELLO(&t,"example.com",b,256u)!=103u)return 4;
    if(b[0]!=0x16u||b[3]!=0u||b[4]!=98u)return 5;
    for(u32 i=0u;i<256u;i++)b[i]=0xA5u;
    if(TLS_BUILD_CLIENT_HELLO(&t,"bad\r\ninjected",b,256u)!=0u)return 6;
    for(u32 i=0u;i<256u;i++)if(b[i]!=0xA5u)return 7;
    return 0;
}
''', encoding="utf-8")
        executable = Path(td) / "test"
        built = subprocess.run(
            [cc, "-std=c11", "-O0", "-fno-builtin", "-I", td, str(harness), "-o", str(executable)],
            cwd=ROOT, capture_output=True, text=True, timeout=25,
        )
        require(built.returncode == 0, "native compile failed: " + built.stderr)
        ran = subprocess.run([str(executable)], capture_output=True, text=True, timeout=8)
        require(ran.returncode == 0, "native bounds/entropy harness failed: " + str(ran.returncode))
    print(json.dumps({
        "status":"PASS_BOUNDED_HOST","https_ready":False,
        "claim_allowed":False,"production_external_runtime_dependencies_added":0,
        "coverage":["historical_immutable","materializer_no_downgrade",
        "ABI_getrandom_syscall_numbers","TLS_INIT_error_gate",
        "ClientHello_cap_canary","SNI_CRLF_reject","host_getrandom"],
        "not_run":["ARM32_physical","ARM64_physical","TLS13_handshake","CA_hostname","AEAD","OpenAI_API"],
    },sort_keys=True))
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as e:
        print(json.dumps({"status":"FAIL","error":str(e),"claim_allowed":False}))
        raise SystemExit(1)
