#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import threading
import unittest
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "termux_health_server", ROOT / "scripts/termux_health_server.py"
)
assert SPEC and SPEC.loader
HEALTH = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(HEALTH)


class TermuxHealthServerTests(unittest.TestCase):
    def test_snapshot_is_bounded_sanitized_and_deterministic(self) -> None:
        snapshot = HEALTH.build_snapshot(
            started_ns=1_000_000_000,
            now_ns=1_125_000_000,
            pid=42,
            abi="ARMv7 L",
            environment={
                "RAF_TERMUX_COMMIT": "ABCDEF1234567",
                "SECRET_TOKEN": "must-not-leak",
                "HOME": "/private/path",
            },
        )

        self.assertEqual(snapshot.schema, "raf.termux-health.v1")
        self.assertEqual(snapshot.status, "ok")
        self.assertEqual(snapshot.runtime, "termux-rafcodephi")
        self.assertEqual(snapshot.abi, "armv7_l")
        self.assertEqual(snapshot.pid, 42)
        self.assertEqual(snapshot.uptime_ms, 125)
        self.assertEqual(snapshot.commit, "abcdef1234567")

        payload_a = HEALTH.snapshot_payload(snapshot)
        payload_b = HEALTH.snapshot_payload(snapshot)
        self.assertEqual(payload_a, payload_b)
        self.assertNotIn(b"must-not-leak", payload_a)
        self.assertNotIn(b"/private/path", payload_a)

    def test_invalid_commit_becomes_token_vazio(self) -> None:
        self.assertEqual(HEALTH.sanitize_commit("not-a-sha"), "TOKEN_VAZIO")
        self.assertEqual(HEALTH.sanitize_commit(""), "TOKEN_VAZIO")

    def test_server_rejects_non_loopback_and_privileged_port(self) -> None:
        with self.assertRaisesRegex(ValueError, "host_not_loopback"):
            HEALTH.create_server("0.0.0.0", 8765)
        with self.assertRaisesRegex(ValueError, "host_not_loopback"):
            HEALTH.create_server("192.168.1.2", 8765)
        with self.assertRaisesRegex(ValueError, "port_out_of_range"):
            HEALTH.create_server("127.0.0.1", 80)

    def test_get_health_returns_canonical_json(self) -> None:
        snapshot = HEALTH.build_snapshot(
            started_ns=0,
            now_ns=42_000_000,
            pid=7,
            abi="armv7l",
            environment={},
        )
        with RunningServer(lambda: snapshot) as base_url:
            with urllib.request.urlopen(f"{base_url}/health", timeout=2) as response:
                self.assertEqual(response.status, 200)
                self.assertEqual(response.headers["Cache-Control"], "no-store")
                self.assertEqual(response.headers["X-Content-Type-Options"], "nosniff")
                payload = json.loads(response.read())

        self.assertEqual(payload["schema"], "raf.termux-health.v1")
        self.assertEqual(payload["status"], "ok")
        self.assertEqual(payload["runtime"], "termux-rafcodephi")
        self.assertEqual(payload["abi"], "armv7l")
        self.assertEqual(payload["pid"], 7)
        self.assertEqual(payload["uptime_ms"], 42)
        self.assertEqual(payload["commit"], "TOKEN_VAZIO")
        self.assertIn("health.readonly", payload["capabilities"])

    def test_unknown_path_is_404_without_query_logging(self) -> None:
        snapshot = HEALTH.build_snapshot(started_ns=0, now_ns=0, environment={})
        with RunningServer(lambda: snapshot) as base_url:
            with self.assertRaises(urllib.error.HTTPError) as raised:
                urllib.request.urlopen(f"{base_url}/health?secret=1", timeout=2)
            self.assertEqual(raised.exception.code, 404)
            payload = json.loads(raised.exception.read())
        self.assertEqual(payload["reason"], "path_not_allowed")

    def test_mutating_method_is_rejected(self) -> None:
        snapshot = HEALTH.build_snapshot(started_ns=0, now_ns=0, environment={})
        with RunningServer(lambda: snapshot) as base_url:
            request = urllib.request.Request(
                f"{base_url}/health",
                method="POST",
                data=b"{}",
                headers={"Content-Type": "application/json"},
            )
            with self.assertRaises(urllib.error.HTTPError) as raised:
                urllib.request.urlopen(request, timeout=2)
            self.assertEqual(raised.exception.code, 405)
            payload = json.loads(raised.exception.read())
        self.assertEqual(payload["reason"], "read_only_endpoint")

    def test_head_health_has_no_body(self) -> None:
        snapshot = HEALTH.build_snapshot(started_ns=0, now_ns=0, environment={})
        with RunningServer(lambda: snapshot) as base_url:
            request = urllib.request.Request(f"{base_url}/v1/health", method="HEAD")
            with urllib.request.urlopen(request, timeout=2) as response:
                self.assertEqual(response.status, 200)
                self.assertEqual(response.read(), b"")
                self.assertEqual(response.headers["Content-Length"], "0")


class RunningServer:
    def __init__(self, snapshot_factory):
        self.snapshot_factory = snapshot_factory
        self.server = None
        self.thread = None

    def __enter__(self) -> str:
        self.server = HEALTH.create_server(
            "127.0.0.1",
            0,
            snapshot_factory=self.snapshot_factory,
            allow_ephemeral=True,
        )
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        host, port = self.server.server_address[:2]
        return f"http://{host}:{port}"

    def __exit__(self, exc_type, exc, tb) -> None:
        assert self.server is not None
        assert self.thread is not None
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=2)
        if self.thread.is_alive():
            raise AssertionError("health server thread did not terminate")


if __name__ == "__main__":
    unittest.main()
