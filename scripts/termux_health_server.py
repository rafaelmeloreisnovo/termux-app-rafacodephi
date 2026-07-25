#!/usr/bin/env python3
"""Bounded, read-only health endpoint for the local Termux RAFCODEΦ runtime."""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import signal
import socket
import sys
import time
from dataclasses import asdict, dataclass
from http.server import BaseHTTPRequestHandler, HTTPServer
from typing import Callable, Mapping, Sequence

SCHEMA = "raf.termux-health.v1"
RUNTIME = "termux-rafcodephi"
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8765
ALLOWED_HOSTS = {"127.0.0.1", "localhost", "::1"}
ALLOWED_PATHS = {"/health", "/v1/health"}
COMMIT_RE = re.compile(r"^[0-9a-fA-F]{7,64}$")
CAPABILITIES = (
    "health.readonly",
    "job.submit.readonly",
    "artifact.inspect",
    "rafpolimata.status",
)


@dataclass(frozen=True)
class HealthSnapshot:
    schema: str
    status: str
    runtime: str
    abi: str
    pid: int
    uptime_ms: int
    capabilities: tuple[str, ...]
    commit: str


def sanitize_commit(raw: str | None) -> str:
    value = (raw or "").strip()
    return value.lower() if COMMIT_RE.fullmatch(value) else "TOKEN_VAZIO"


def sanitize_abi(raw: str | None) -> str:
    value = (raw or "unknown").strip().lower()
    value = re.sub(r"[^a-z0-9_.+-]", "_", value)
    return value[:64] or "unknown"


def build_snapshot(
    *,
    started_ns: int,
    now_ns: int | None = None,
    pid: int | None = None,
    abi: str | None = None,
    environment: Mapping[str, str] | None = None,
) -> HealthSnapshot:
    current_ns = time.monotonic_ns() if now_ns is None else now_ns
    uptime_ns = max(0, current_ns - started_ns)
    env = os.environ if environment is None else environment
    return HealthSnapshot(
        schema=SCHEMA,
        status="ok",
        runtime=RUNTIME,
        abi=sanitize_abi(platform.machine() if abi is None else abi),
        pid=os.getpid() if pid is None else max(0, int(pid)),
        uptime_ms=uptime_ns // 1_000_000,
        capabilities=CAPABILITIES,
        commit=sanitize_commit(env.get("RAF_TERMUX_COMMIT")),
    )


def snapshot_payload(snapshot: HealthSnapshot) -> bytes:
    payload = asdict(snapshot)
    payload["capabilities"] = list(snapshot.capabilities)
    return (json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n").encode("utf-8")


def error_payload(status: str, reason: str) -> bytes:
    return (
        json.dumps(
            {"schema": SCHEMA, "status": status, "reason": reason},
            ensure_ascii=False,
            sort_keys=True,
            separators=(",", ":"),
        )
        + "\n"
    ).encode("utf-8")


class HealthRequestHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    server_version = "RAFAELIAHealth/1"
    sys_version = ""

    def do_GET(self) -> None:  # noqa: N802 - stdlib handler contract
        if self.path not in ALLOWED_PATHS:
            self._send(404, error_payload("not_found", "path_not_allowed"))
            return
        snapshot = self.server.snapshot_factory()  # type: ignore[attr-defined]
        self._send(200, snapshot_payload(snapshot))

    def do_HEAD(self) -> None:  # noqa: N802
        if self.path not in ALLOWED_PATHS:
            self._send(404, b"", include_body=False)
            return
        self._send(200, b"", include_body=False)

    def do_POST(self) -> None:  # noqa: N802
        self._send(405, error_payload("method_not_allowed", "read_only_endpoint"))

    def do_PUT(self) -> None:  # noqa: N802
        self._send(405, error_payload("method_not_allowed", "read_only_endpoint"))

    def do_DELETE(self) -> None:  # noqa: N802
        self._send(405, error_payload("method_not_allowed", "read_only_endpoint"))

    def do_PATCH(self) -> None:  # noqa: N802
        self._send(405, error_payload("method_not_allowed", "read_only_endpoint"))

    def log_message(self, _format: str, *_args: object) -> None:
        # Avoid writing request material or accidental query values to logs.
        return

    def _send(self, code: int, body: bytes, *, include_body: bool = True) -> None:
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Connection", "close")
        self.send_header("Content-Length", str(len(body) if include_body else 0))
        self.end_headers()
        if include_body and body:
            self.wfile.write(body)


class HealthHTTPServer(HTTPServer):
    allow_reuse_address = True

    def __init__(
        self,
        server_address: tuple[str, int],
        snapshot_factory: Callable[[], HealthSnapshot],
    ) -> None:
        self.snapshot_factory = snapshot_factory
        super().__init__(server_address, HealthRequestHandler)


class HealthHTTPServerV6(HealthHTTPServer):
    address_family = socket.AF_INET6


def normalize_host(host: str) -> str:
    value = host.strip().lower()
    if value == "localhost":
        return DEFAULT_HOST
    if value not in ALLOWED_HOSTS:
        raise ValueError("host_not_loopback")
    return value


def validate_port(port: int, *, allow_ephemeral: bool = False) -> int:
    value = int(port)
    if allow_ephemeral and value == 0:
        return value
    if value < 1024 or value > 65535:
        raise ValueError("port_out_of_range")
    return value


def create_server(
    host: str = DEFAULT_HOST,
    port: int = DEFAULT_PORT,
    *,
    snapshot_factory: Callable[[], HealthSnapshot] | None = None,
    allow_ephemeral: bool = False,
) -> HealthHTTPServer:
    normalized_host = normalize_host(host)
    normalized_port = validate_port(port, allow_ephemeral=allow_ephemeral)
    started_ns = time.monotonic_ns()
    factory = snapshot_factory or (lambda: build_snapshot(started_ns=started_ns))
    server_type = HealthHTTPServerV6 if normalized_host == "::1" else HealthHTTPServer
    return server_type((normalized_host, normalized_port), factory)


def install_shutdown_handlers(server: HTTPServer) -> None:
    def request_shutdown(_signum: int, _frame: object) -> None:
        # shutdown() must run outside the serve_forever() callback context.
        import threading

        threading.Thread(target=server.shutdown, daemon=True).start()

    for signum in (signal.SIGINT, signal.SIGTERM):
        signal.signal(signum, request_shutdown)


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Serve a loopback-only Termux health endpoint.")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--once", action="store_true", help="Handle one request and exit.")
    parser.add_argument("--print-snapshot", action="store_true", help="Print JSON and exit without binding.")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    started_ns = time.monotonic_ns()

    if args.print_snapshot:
        sys.stdout.buffer.write(snapshot_payload(build_snapshot(started_ns=started_ns)))
        return 0

    try:
        server = create_server(
            args.host,
            args.port,
            snapshot_factory=lambda: build_snapshot(started_ns=started_ns),
        )
    except (OSError, ValueError) as error:
        print(f"FAIL termux-health-server: {error}", file=sys.stderr)
        return 2

    install_shutdown_handlers(server)
    bound_host, bound_port = server.server_address[:2]
    print(
        json.dumps(
            {
                "schema": SCHEMA,
                "state": "LISTENING",
                "host": bound_host,
                "port": bound_port,
                "mutating_commands": False,
            },
            sort_keys=True,
        ),
        flush=True,
    )

    try:
        if args.once:
            server.handle_request()
        else:
            server.serve_forever(poll_interval=0.25)
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
