# RAFAELIA Termux Edge Sensor Matrix

Status: architecture baseline
Target reference: Motorola Moto E7 Power, Android 10, ARM32/armeabi-v7a
Repository: `termux-app-rafacodephi`

## 1. Purpose

This document defines the operational architecture for turning the existing Termux sensor API bridge into a governed edge-observation runtime.

The system must not infer that a sensor exists from a product page. Runtime inventory is canonical.

```text
device inventory
  -> available skills
  -> permission gate
  -> cycle capture
  -> normalization
  -> CPU/NEON or GPU route
  -> verify
  -> commit or rollback
  -> append-only evidence ledger
```

## 2. Existing repository baseline

The application already contains:

- custom dangerous permission `${TERMUX_PACKAGE_NAME}.permission.RAF_SENSOR_ACCESS`;
- exported `RafSensorApiService` protected by that permission;
- protocol-version validation;
- request IDs and duplicate-request rejection;
- sensor availability validation through `SensorManager`;
- asynchronous `PendingIntent` callbacks;
- timeout and cancellation paths;
- hardware profile fields returned with completed samples;
- integration contract tests for manifest/service wiring.

This baseline is preserved. The next implementation cycle extends it; it does not replace it.

## 3. Invariant

The architectural invariant is:

> Preserve provenance, state and relations while observations pass from Android hardware into an auditable cycle.

A sample is not treated as truth. It is an event:

```text
source + observation + timestamp + accuracy + device context
```

Knowledge or action may only follow validation.

## 4. Edge cycle

Clock measures duration. It does not authorize state transition.

```text
LOAD
  -> INVENTORY
  -> CAPTURE
  -> NORMALIZE
  -> RELATE
  -> VERIFY
  -> COMMIT
```

On failure:

```text
VERIFY failed
  -> reject candidate state
  -> preserve canonical state
  -> record failure evidence
  -> no next cycle
```

## 5. Skill families

### 5.1 Physical sensors

- `motion.accelerometer`
- `motion.gyroscope`
- `motion.gravity`
- `motion.rotation_vector`
- `motion.step_counter`
- `field.magnetic`
- `environment.light`
- `environment.proximity`
- `environment.pressure`

Each skill is `AVAILABLE`, `UNAVAILABLE`, `PERMISSION_REQUIRED`, `DEGRADED` or `TOKEN_VAZIO` after inventory.

### 5.2 Android observation skills

- `location.gnss`
- `radio.wifi`
- `radio.bluetooth`
- `audio.microphone`
- `vision.camera`
- `device.battery`
- `device.thermal`
- `device.cpu`
- `device.memory`
- `device.storage`
- `device.graphics`

These sources are not all Android `Sensor` objects. They require separate adapters and permission profiles.

## 6. Canonical frame

A future binary/native contract should preserve at least:

```c
typedef struct {
    uint32_t protocol_version;
    uint32_t skill_id;
    uint64_t cycle_id;
    int64_t  timestamp_ns;

    int32_t  value_q16[4];
    int32_t  expected_q16[4];
    int32_t  residual_q16[4];

    uint32_t accuracy;
    uint32_t confidence;
    uint32_t material_state;
    uint32_t semantic_state;
    uint32_t operational_gate;

    uint64_t source_hash;
    uint64_t frame_hash;
} RafEdgeFrame;
```

The current Intent/PendingIntent protocol remains the Android transport until this binary contract is implemented.

## 7. Material, semantic and operational states

Do not collapse empty, absent, unknown and blocked.

| Dimension | Values |
|---|---|
| Material | `absent`, `empty`, `partial`, `filled`, `sparse`, `special_runtime` |
| Semantic | `known`, `intentional`, `reserved`, `unknown`, `contradictory`, `not_applicable` |
| Operational | `processable`, `allocatable`, `no_op`, `sandbox_only`, `human_review`, `blocked` |

Examples:

- missing gyroscope: `absent + known + no_op`;
- unavailable permission: `partial + known + blocked`;
- empty ring slot: `empty + reserved + allocatable`;
- unresolved hardware field: `empty + unknown + human_review`.

## 8. CPU, NEON and GPU routes

### CPU scalar

Mandatory fallback for every algorithm.

### ARM32 NEON

Use only after runtime capability confirmation. Appropriate for:

- fixed-point filters;
- small matrix kernels;
- residual calculation;
- sensor correlation;
- FFT blocks;
- XOR/popcount binary networks.

### GPU

GPU access must use Android graphics APIs. No claim of direct register control is allowed without a different privilege model.

Route candidates:

- OpenGL ES for texture/grid processing and visualization;
- Vulkan compute only when runtime enumeration proves support;
- CPU/NEON fallback when driver support, thermal state or workload size makes GPU routing unsuitable.

## 9. Memory and cache discipline

The application cannot directly control L1/L2 replacement policy. It can control locality.

Requirements:

- static or bounded ring buffers in hot paths;
- contiguous storage;
- no JSON serialization inside acquisition callbacks;
- batch processing for NEON;
- explicit producer/consumer ownership;
- append-only binary ledger;
- JSON only for inventory and human-readable exports;
- no unbounded sensor request maps or queues.

## 10. Storage layout

```text
$HOME/.rafaelia-edge/
├── inventory/
│   ├── device.json
│   ├── sensors.json
│   ├── cpu.json
│   ├── graphics.json
│   ├── memory.json
│   └── permissions.json
├── ledger/
│   ├── active.wal
│   ├── checkpoints/
│   └── cycles/
├── calibration/
├── crashes/
├── logs/
└── exports/
```

## 11. Security rules

- keep `RafSensorApiService` permission-protected;
- do not export new components without a typed permission and explicit contract;
- request dangerous permissions at runtime;
- use minimum permission profiles instead of requesting all capabilities together;
- do not store raw microphone, camera or location data by default;
- redact identifiers in exported diagnostics;
- preserve consent and provenance in the ledger;
- keep debug-only interfaces disabled in release builds.

## 12. Definition of done for the first implementation cycle

The architecture reaches its first executable milestone when the APK can generate six truthful inventories:

```text
device.json
sensors.json
cpu.json
graphics.json
memory.json
permissions.json
```

Each file must distinguish:

- measured;
- declared by Android;
- inferred;
- unavailable;
- permission-blocked;
- unknown.

No optimization work begins before this physical/runtime baseline is committed.