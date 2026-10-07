# RAFCODEΦ NetGuard Lite V1 — no-root runtime boundary

Status: `SOURCE_IMPLEMENTED / RUNTIME_UNTESTED_THIS_BRANCH`  
Authority: `termux-app-rafacodephi`  
Mode: defensive, user-launched process scope only.

## Purpose

Provide an urgent minimum network-control plane for RAFCODEΦ without pretending that a normal Termux process is a kernel firewall.

```text
USER AUTHORITY
  -> choose command
  -> AUDIT or AIRGAP
  -> metadata receipt
  -> hash receipt
  -> review
```

The first implementation has two concrete adapters:

1. `scripts/raf_netguard_exec.sh audit ...` — traces connection-establishment syscalls of the command tree with `strace`, without tracing packet payload syscalls.
2. `scripts/raf_netguard_exec.sh airgap ...` — installs a seccomp filter that returns `EPERM` for network syscalls and closes inherited file descriptors before executing the command.

`audit-airgap` composes both so denied network attempts are observable while remaining blocked.

## Security properties

The airgap launcher:

- requires no root;
- sets `PR_SET_NO_NEW_PRIVS`;
- verifies the seccomp architecture and fails closed on ABI mismatch;
- denies socket/socketpair/connect/bind/listen/accept/send/receive message families;
- denies io_uring setup/enter/register to avoid a second asynchronous network route;
- refuses to claim isolation when stdin/stdout/stderr are already sockets;
- closes inherited descriptors >= 3 before the filter is installed;
- returns `EPERM` for denied network syscalls so the child can fail cleanly.

## What it does NOT prove

```text
TERMUX_USERSPACE != DEVICE_WIDE_FIREWALL
STRACE_AUDIT != PACKET_CAPTURE
SECCOMP_AIRGAP != SELECTIVE_NAT_POLICY
APP_TUPLE != POST_NAT_TUPLE
SOURCE_IMPLEMENTED != DEVICE_PROVEN
```

NAT/PAT remains `TOKEN_VAZIO` unless an adapter can observe both the original tuple and the translated tuple. The current no-root child-process adapters generally cannot.

## UDP

The strace audit intentionally excludes `sendto/recvfrom/sendmsg/recvmsg` because those calls can expose payload bytes in human-readable traces. Therefore UDP destination coverage in audit mode is incomplete and MUST be labeled `TOKEN_VAZIO_UDP_DESTINATION`.

The airgap mode still blocks those UDP-capable syscalls.

A future metadata-only adapter may inspect sockaddr + byte count while hashing or discarding payload. Candidate adapters, in increasing scope:

- libc/LD_PRELOAD adapter for processes intentionally launched by RAFCODEΦ;
- Frida debug adapter for an explicitly authorized attached process;
- Android `VpnService` adapter for user-consented device traffic;
- privileged kernel adapter only where the platform owner explicitly authorizes it.

## Frida boundary

Frida is treated as one adapter, not as the policy authority. It may feed the same Lite Contract only for processes the operator is authorized to inspect. No stealth attachment, credential bypass, remote injection or third-party process targeting is part of this route.

## Commands

```sh
./scripts/raf_netguard_exec.sh build
./scripts/raf_netguard_exec.sh selftest

# Observe connection-establishment metadata.
./scripts/raf_netguard_exec.sh audit curl https://example.org

# Execute a command with networking denied.
./scripts/raf_netguard_exec.sh airgap sh -c 'echo local-only'

# Observe attempted networking while blocking it.
./scripts/raf_netguard_exec.sh audit-airgap curl https://example.org
```

Audit files are SHA-256 hashed; BLAKE3 receipts are added when `b3sum` is already present. Missing BLAKE3 does not silently upgrade evidence.

## Lite event contract

Every higher-scope adapter should converge on:

```text
event_id
sequence
wall_time + monotonic_time
adapter
pid/uid/process
direction
protocol
pre_tuple(src_ip,src_port,dst_ip,dst_port)
post_tuple(...)
decision(rule_id,allow|deny,reason)
bytes
payload_hash(optional)
payload=REDACTED_METADATA_ONLY
nat_state
pat_state
errno
source_commit
```

The same event id should correlate PRE and POST observations. If a field is not observable at that layer, use `TOKEN_VAZIO_<reason>`.

## Operational priority

P0: prevent credential/payload release before destination authorization.  
P1: record metadata before and after the adapter decision.  
P2: selective policy for own process/child processes.  
P3: user-consented `VpnService` packet plane with explicit NAT/PAT transformation receipts.  
P4: Frida adapter only after the existing Phase 7 blockers and authorization gates are cleared.

## R3

- F_ok: source-level no-root airgap + metadata audit composition is implemented on this branch.
- F_gap: provider CI/device execution, selective egress rules, UDP metadata-only adapter and VpnService remain open.
- F_next: run `selftest` on armv7/aarch64 Termux, then run one denied network attempt under `audit-airgap` and bind its hashes to the exact commit.
