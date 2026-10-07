# Receipt — Termux health loopback port separation V1

Date: 2026-09-23
Parent finding: RafGitTools loopback-port separation audit
Base: `master@812f3c60a22441425f10d37e733de51de9a26e16`
claim_allowed: false

## Delta

`scripts/termux_health_server.py` default port changed:

```text
8765 -> 8766
```

Reason: `8765` is already the RafGitTools Raf Bridge/Kiwi listener. Concurrent local services must not share the same bind.

Tests now pin `DEFAULT_PORT == 8766`. Runtime remains loopback-only and read-only.

## Boundary

No job submission, shell, filesystem write, network-external bind or new capability was added.

SOURCE = IMPLEMENTED
CI = TOKEN_VAZIO until this branch runs
ANDROID_DEVICE_CONCURRENT_BIND = TOKEN_VAZIO

R3=<F_ok: producer default separated; F_gap: paired CI/device; F_next: run tests and simultaneous-listener smoke>.
