#!/usr/bin/env python3
"""Materialize the RAFAELIA text browser with HTTPS fail-closed semantics.

Browser.sh is a historical source generator. Its incomplete TLS prototype used to
rewrite an HTTPS request to port 80. This tool extracts the generated build
script and replaces that exact downgrade block with a terminal error path.

It does not implement TLS. It guarantees only that the canonical materialized
browser never converts an HTTPS request into plaintext HTTP.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT / "Browser.sh"

START_MARKER = "cat > /tmp/browser.txt << 'OUTER_SCRIPT'\n"
END_MARKER = "\nOUTER_SCRIPT\n"

OLD_BLOCK = """            PS(\"  [TLS] NOTA: crypto não implementado — usando HTTP para demo\\n\");
        }
        /* Fallback: fecha e reconecta em HTTP para demonstração */
        CLOSE(ctx->fd);
        ctx->port=80u;ctx->use_tls=0;
        ctx->fd=SOCKET();
        if(ctx->fd<0){FF_SET(ctx->flags,FL_ERROR);GRS();return-1;}
        if(CONNECT(ctx->fd,&sa)!=0){FF_SET(ctx->flags,FL_ERROR);CLOSE(ctx->fd);GRS();return-1;}
        FF_CLR(ctx->flags,FL_TLS_HS);
        PS(\"  [FALLBACK] Usando HTTP para demo\\n\");
""".rstrip("\n")

NEW_BLOCK = """            PS(\"  [TLS] NOTA: crypto incompleto — HTTPS será recusado\\n\");
        } else {
            PS(\"  [TLS] resposta insuficiente para ServerHello\\n\");
        }
        /* Fail closed: HTTPS nunca pode ser rebaixado para HTTP plaintext. */
        _TLS.state=TLS_ERROR;
        ctx->tls=TLS_ERROR;
        FF_SET(ctx->flags,FL_ERROR);
        STATUS(ctx->flags,"HTTPS bloqueado: TLS criptográfico ainda não implementado");
        CLOSE(ctx->fd);
        GRS();
        return-2;
""".rstrip("\n")

# A configuração inicial `port=80/use_tls=0` é válida para URLs HTTP. O que é
# proibido é a sequência de *downgrade após uma tentativa HTTPS* e suas marcas.
FORBIDDEN = (
    "[FALLBACK] Usando HTTP para demo",
    "crypto não implementado — usando HTTP para demo",
)

REQUIRED = (
    "HTTPS nunca pode ser rebaixado para HTTP plaintext",
    "ctx->tls=TLS_ERROR;",
    "HTTPS bloqueado: TLS criptográfico ainda não implementado",
    "return-2;",
)


def extract_inner(source: str) -> str:
    if source.count(START_MARKER) != 1:
        raise ValueError("Browser.sh must contain exactly one OUTER_SCRIPT start marker")
    after = source.split(START_MARKER, 1)[1]
    if after.count(END_MARKER) != 1:
        raise ValueError("Browser.sh must contain exactly one OUTER_SCRIPT end marker")
    return after.split(END_MARKER, 1)[0]


def materialize(source_path: Path, output_path: Path) -> dict[str, object]:
    source = source_path.read_text(encoding="utf-8")
    inner = extract_inner(source)

    if inner.count(OLD_BLOCK) != 1:
        raise ValueError("expected exactly one known plaintext downgrade block")

    secured = inner.replace(OLD_BLOCK, NEW_BLOCK, 1)
    secured = secured.replace(
        "Browser text-mode ARM32/ARM64/x86-64 · TLS 1.3 · HTTP/1.1",
        "Browser text-mode ARM32/ARM64/x86-64 · TLS prototype fail-closed · HTTP/1.1",
        1,
    )

    # Fix two historical HTTP literal lengths in the materialized build only.
    # Copy exactly the payload bytes; never place a C terminating NUL on the wire.
    # Keep Browser.sh immutable as a historical predecessor.
    wire_fixes = (
        (
            r'MC(p,(const u8*)"Accept: text/html,text/plain;q=0.9,*/*;q=0.8\r\n",47u);p+=47;',
            r'MC(p,(const u8*)"Accept: text/html,text/plain;q=0.9,*/*;q=0.8\r\n",46u);p+=46;',
        ),
        (
            r'MC(p,(const u8*)"Accept-Language: pt-BR,pt;q=0.9,en;q=0.8\r\n",43u);p+=43;',
            r'MC(p,(const u8*)"Accept-Language: pt-BR,pt;q=0.9,en;q=0.8\r\n",42u);p+=42;',
        ),
    )
    for old, new in wire_fixes:
        if secured.count(old) != 1:
            raise ValueError("expected exactly one historical HTTP header length pattern")
        secured = secured.replace(old, new, 1)

    # Historical 'usize' was 32-bit on x86_64/AArch64: truncates syscall pointers.
    # Preserve the raw generator; emit ABI-specific pointer width only in successor.
    old_usize = "typedef unsigned int       usize;"
    if secured.count(old_usize) != 1:
        raise ValueError("historical pointer-width contract source drift")
    secured = secured.replace(
        old_usize,
        "#if defined(__aarch64__) || defined(__x86_64__)\n"
        "typedef unsigned long long usize;\n"
        "#else\n"
        "typedef unsigned int       usize;\n"
        "#endif", 1
    )

    # Bounded, explicit successor to the immutable historical Browser.sh source.
    # Input-specific patterns must match exactly once or the materializer fails.
    entropy_source = r'''#pragma once
/* br_entropy.h — entropy boundary for freestanding BrowserRaf
 *
 * Security contract:
 *   - Linux getrandom(2) is the only entropy source.
 *   - short reads and EINTR are handled explicitly.
 *   - any other error fails closed and clears the destination buffer.
 *   - there is no deterministic, timestamp, address or LFSR fallback.
 */
#include "br_sys.h"

#define BR_EINTR 4
#define BR_GRND_DEFAULT 0u

#if defined(__arm__)
#define BR_NR_GETRANDOM 384u
AI s32 BR_GETRANDOM(void*buf,u32 n,u32 flags){
    return _sc3(BR_NR_GETRANDOM,(u32)(usize)buf,n,flags);
}
#elif defined(__aarch64__)
#define BR_NR_GETRANDOM 278u
AI s32 BR_GETRANDOM(void*buf,u32 n,u32 flags){
    return (s32)_sc3(BR_NR_GETRANDOM,(u64)(usize)buf,(u64)n,(u64)flags);
}
#elif defined(__x86_64__)
#define BR_NR_GETRANDOM 318u
AI s32 BR_GETRANDOM(void*buf,u32 n,u32 flags){
    return (s32)_sc3(BR_NR_GETRANDOM,(u64)(usize)buf,(u64)n,(u64)flags);
}
#else
#error "BrowserRaf entropy boundary supports only ARM32, ARM64 and x86-64"
#endif

AI s32 BR_RANDOM_FILL(void*buf,u32 n){
    u8*p=(u8*)buf;
    u32 off=0u;

    if(!p&&n!=0u)return-1;

    while(off<n){
        s32 got=BR_GETRANDOM(p+off,n-off,BR_GRND_DEFAULT);
        if(got<0){
            if(got==-(s32)BR_EINTR)continue;
            MC0(p,n);
            return-1;
        }
        if(got==0||(u32)got>n-off){
            MC0(p,n);
            return-1;
        }
        off+=(u32)got;
    }
    return 0;
}
'''
    entropy_marker = 'cat > "$D/br_tls.h" << \'F\''
    if secured.count(entropy_marker) != 1:
        raise ValueError("TLS heredoc not uniquely located")
    entropy_script = 'cat > "$D/br_entropy.h" << \'F\'\n' + entropy_source + 'F\nok "br_entropy.h"\n\n'
    secured = secured.replace(entropy_marker, entropy_script + entropy_marker, 1)

    old_include = '#include "br_sys.h"\n\n/* ── TLS 1.3 RECORD LAYER'
    if secured.count(old_include) != 1:
        raise ValueError("TLS include drift")
    secured = secured.replace(
        old_include,
        '#include "br_entropy.h"\n\n/* ── TLS 1.3 RECORD LAYER', 1
    )

    random_begin = "/* Gerador pseudo-aleatório determinístico para random[]"
    random_end = "/* ── CONSTRUTOR DE ClientHello"
    if secured.count(random_begin) != 1 or secured.count(random_end) != 1:
        raise ValueError("historical TLS entropy source drift")
    beg = secured.index(random_begin)
    end = secured.index(random_end, beg)
    secured = secured[:beg] + r'''/* Inicialização de contexto sem fallback pseudo-aleatório.
 * A ausência de entropia do kernel bloqueia a criação do ClientHello. */
static s32 TLS_INIT(TLSCtx*t){
    MC0(t,sizeof(*t));
    t->state=TLS_IDLE;
    if(BR_RANDOM_FILL(t->random,(u32)sizeof(t->random))!=0){
        t->alert=TLS_ALERT_INTERNAL_ERROR;
        return-1;
    }
    return 0;
}
''' + secured[end:]

    # TLS entropy errors must carry a defined alert on every ABI.
    alert_marker = "#define TLS_HT_FINISHED       0x14u"
    if secured.count(alert_marker) != 1:
        raise ValueError("TLS alert insertion source drift")
    secured = secured.replace(
        alert_marker,
        alert_marker + "\n#define TLS_ALERT_INTERNAL_ERROR 80u", 1
    )

    # The obsolete descriptor must not contradict the materialized source.
    old_comment = "client random (pseudo-random Q16-based)"
    if secured.count(old_comment) != 1:
        raise ValueError("TLSCtx random comment source drift")
    secured = secured.replace(old_comment, "client random: getrandom only", 1)

    old_hello = 'static u32 TLS_BUILD_CLIENT_HELLO(TLSCtx*t,const char*host,u8*buf,u32 cap){\n    /* Usamos ponteiro p que avança — sem nomes de variável adicionais */\n'
    new_hello = r'''static u32 TLS_BUILD_CLIENT_HELLO(TLSCtx*t,const char*host,u8*buf,u32 cap){
    /* Wire serialization ONLY. HTTPS/TLS authentication remains disabled. */
    if(!t||!host||!buf)return 0u;
    u32 hlen=0u;
    while(hlen<254u&&host[hlen])hlen++;
    if(!hlen||hlen>253u||cap<92u+hlen)return 0u;
    for(u32 i=0u;i<hlen;i++){
        u8 c=(u8)host[i];
        if(c<=0x20u||c>=0x7Fu)return 0u;
    }
    /* Usamos ponteiro p que avança — sem nomes de variável adicionais */
'''
    if secured.count(old_hello) != 1:
        raise ValueError("ClientHello bounds source drift")
    secured = secured.replace(old_hello, new_hello, 1)

    old_sni = '    u32 hlen=SL(host);\n    *p++=0x00;*p++=0x00; /* type */'
    if secured.count(old_sni) != 1:
        raise ValueError("ClientHello SNI read source drift")
    secured = secured.replace(old_sni, '    *p++=0x00;*p++=0x00; /* type */', 1)

    init_call = '        TLS_INIT(&_TLS);'
    if secured.count(init_call) != 1:
        raise ValueError("TLS_INIT call source drift")
    secured = secured.replace(init_call, '''        if(TLS_INIT(&_TLS)!=0){
            FF_SET(ctx->flags,FL_ERROR);
            STATUS(ctx->flags,"TLS entropy unavailable: aborting before ClientHello");
            CLOSE(ctx->fd);GRS();return-3;
        }''', 1)

    hello_call = '        u32 chlen=TLS_BUILD_CLIENT_HELLO(&_TLS,ctx->host,_NB,NET_BUF);'
    if secured.count(hello_call) != 1:
        raise ValueError("ClientHello call source drift")
    secured = secured.replace(hello_call, '''        u32 chlen=TLS_BUILD_CLIENT_HELLO(&_TLS,ctx->host,_NB,NET_BUF);
        if(!chlen){
            FF_SET(ctx->flags,FL_ERROR);
            STATUS(ctx->flags,"TLS ClientHello length invalid");
            CLOSE(ctx->fd);GRS();return-3;
        }''', 1)

    if "0xDEADBEEF" in secured or "AI u32 PRNG" in secured or "LFSR + PHI64" in secured:
        raise ValueError("materialized browser retains historical deterministic TLS entropy")
    if '#include "br_entropy.h"' not in secured or "BR_RANDOM_FILL(t->random" not in secured:
        raise ValueError("materialized browser does not enforce CSPRNG source")

    downgrade_block_present = OLD_BLOCK in secured
    forbidden_present = [token for token in FORBIDDEN if token in secured]
    required_missing = [token for token in REQUIRED if token not in secured]
    if downgrade_block_present or forbidden_present or required_missing:
        raise ValueError(
            "fail-closed materialization invariant failed: "
            f"downgrade_block_present={downgrade_block_present}, "
            f"forbidden={forbidden_present}, missing={required_missing}"
        )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(secured, encoding="utf-8")
    output_path.chmod(output_path.stat().st_mode | 0o111)

    return {
        "schema": "raf.browser.fail-closed-materialization.v1",
        "source": str(source_path),
        "output": str(output_path),
        "plaintext_downgrade_removed": True,
        "http_default_preserved": True,
        "tls_implemented": False,
        "https_policy": "FAIL_CLOSED",
        "claim_allowed_tls": False,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--execute",
        action="store_true",
        help="execute the materialized build script after writing it",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        report = materialize(args.source.resolve(), args.output.resolve())
        if args.execute:
            env = os.environ.copy()
            subprocess.run(["bash", str(args.output.resolve())], check=True, env=env)
            report["build_script_executed"] = True
        else:
            report["build_script_executed"] = False
        json.dump(report, sys.stdout, ensure_ascii=False, indent=2)
        sys.stdout.write("\n")
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        json.dump(
            {
                "schema": "raf.browser.fail-closed-materialization.v1",
                "status": "FAIL",
                "error": str(exc),
            },
            sys.stdout,
            ensure_ascii=False,
            indent=2,
        )
        sys.stdout.write("\n")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
