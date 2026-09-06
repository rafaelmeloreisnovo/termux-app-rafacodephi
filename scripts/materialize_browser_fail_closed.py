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
