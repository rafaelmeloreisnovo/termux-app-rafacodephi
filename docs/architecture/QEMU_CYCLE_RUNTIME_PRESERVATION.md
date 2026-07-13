# QEMU Cycle Runtime Preservation Contract

## Purpose

This contract prevents the freestanding extraction work from accidentally removing or replacing the cycle-based RAFAELIA runtime already integrated into `qemu_rafaelia`.

## Existing runtime that must remain

```text
QEMUTimer
→ RAFAELIA logical tick
→ rafaelia_loop_step
→ cycle measurement
→ entropy/coherence state
→ reschedule
```

The hosted QEMU runtime owns:

- QEMU lifecycle callbacks;
- timer scheduling;
- pause/running policy;
- mutex and instance lifecycle;
- route/instrument collection;
- QMP/IPC integration;
- process-level observability.

It is not a defect and must not be moved wholesale into the Android app.

## Pure compute core boundary

Only reusable deterministic computation should be extracted behind a stable ABI:

```text
input bytes/state/config
→ deterministic transform
→ result/hash/status
```

The pure compute core must not contain:

- QEMU headers;
- Android/JNI APIs;
- process lifecycle;
- timers or clocks;
- filesystem;
- network;
- syscalls;
- platform logging.

## App boundary

The Android app owns:

- installation and validation of the pinned QEMU artifact;
- foreground service and process controller;
- UI and user lifecycle;
- VM configuration and persistent ledger;
- camera, HDR and sensor capture;
- QMP/Unix-socket client;
- optional bridge from Android frames to a guest virtual camera.

## Permanent versus transient

Permanent/versioned:

```text
QEMU binary artifact
source commit
SHA256SUMS
BUILD_INFO
qemu-exec contract
QMP client
service/controller
VM configuration and disks
```

Transient:

```text
running QEMU process
frame buffers
HDR/YUV buffers
session sockets
stdout/stderr streams
per-frame metrics
scratch memory
```

## Camera/HDR rule

Camera and HDR processing remain Android-first by default:

```text
CameraX/Camera2
→ Surface/ImageReader/AHardwareBuffer
→ app-side HDR/tone mapping
→ optional shared-memory/Unix-FD bridge
→ QEMU guest only when required
```

Frames must be timestamped and mapped to a `rafaelia_cycle_id`; the QEMU runtime must consume the latest complete frame without blocking its cycle waiting for camera delivery.

## Required invariant

```text
QEMU_CYCLE_RUNTIME_PRESERVED
AND PURE_COMPUTE_CORE_EXTRACTED
AND QEMU_ARTIFACT_EXTERNAL_AND_PINNED
AND CAMERA_HDR_ANDROID_FIRST
```
