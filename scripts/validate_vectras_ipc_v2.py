#!/usr/bin/env python3
"""Static fail-closed check for the Vectras/Termux IPC v2 endpoint."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RECEIVER = ROOT / "app/src/main/java/com/termux/app/integration/VectrasIntegrationReceiver.kt"

REQUIRED = (
    "PROTOCOL_VERSION = 2",
    'KEY_NONCE = "nonce"',
    "isValidNonce",
    'KEY_QEMU_BINARY_NAMES = "qemu_binary_names"',
    'EXECUTION_MODE_RUN_COMMAND_SERVICE = "run_command_service"',
    "RUN_COMMAND_PERMISSION =",
    "context.sendBroadcast(response, RUN_COMMAND_PERMISSION)",
    "KEY_PRIVATE_PATHS_EXPOSED",
    "putExtra(KEY_PRIVATE_PATHS_EXPOSED, false)",
)
FORBIDDEN = (
    "putExtra(KEY_PREFIX_PATH",
    "putExtra(KEY_QEMU_BINARY_PATHS",
)


def main() -> int:
    text = RECEIVER.read_text(encoding="utf-8")
    missing = [needle for needle in REQUIRED if needle not in text]
    forbidden = [needle for needle in FORBIDDEN if needle in text]
    if missing or forbidden:
        print({"status": "FAIL", "missing": missing, "forbidden": forbidden})
        return 1
    print({
        "status": "PASS",
        "protocol": "raf.vectras-termux-ipc.v2",
        "private_paths_exposed": False,
        "claim_allowed": False,
    })
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
