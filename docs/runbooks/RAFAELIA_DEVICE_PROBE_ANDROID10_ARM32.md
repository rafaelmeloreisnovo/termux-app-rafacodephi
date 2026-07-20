# RAFAELIA Device Probe Runbook

Reference target: Motorola Moto E7 Power, Android 10, ARM32/armeabi-v7a

## 1. Goal

Freeze the real device state before changing sensor, NEON or GPU code.

Expected artifacts:

```text
device.json
sensors.json
cpu.json
graphics.json
memory.json
permissions.json
```

Unknown values must remain explicit. Never fill missing fields from marketing specifications.

## 2. Preconditions

- development build installed from the same signing lineage used by the project;
- USB debugging enabled;
- Termux package storage initialized when shared storage is required;
- no root assumption;
- battery above 30 percent or device connected to a stable charger;
- airplane mode state recorded before radio tests.

## 3. Termux package preparation

```sh
pkg update
pkg install termux-api clang make git jq procps coreutils
mkdir -p "$HOME/.rafaelia-edge/inventory"
mkdir -p "$HOME/.rafaelia-edge/logs"
```

If the custom APK already embeds the required API bridge, do not install an incompatible external add-on merely to satisfy this runbook. Verify package/signature compatibility first.

## 4. Device identity probe

```sh
OUT="$HOME/.rafaelia-edge/inventory"

{
  printf '{\n'
  printf '  "manufacturer": %s,\n' "$(getprop ro.product.manufacturer | jq -Rs .)"
  printf '  "model": %s,\n' "$(getprop ro.product.model | jq -Rs .)"
  printf '  "device": %s,\n' "$(getprop ro.product.device | jq -Rs .)"
  printf '  "android_release": %s,\n' "$(getprop ro.build.version.release | jq -Rs .)"
  printf '  "sdk": %s,\n' "$(getprop ro.build.version.sdk | jq -Rs .)"
  printf '  "abi": %s,\n' "$(getprop ro.product.cpu.abi | jq -Rs .)"
  printf '  "abilist": %s,\n' "$(getprop ro.product.cpu.abilist | jq -Rs .)"
  printf '  "hardware": %s,\n' "$(getprop ro.hardware | jq -Rs .)"
  printf '  "board_platform": %s,\n' "$(getprop ro.board.platform | jq -Rs .)"
  printf '  "kernel": %s\n' "$(uname -a | jq -Rs .)"
  printf '}\n'
} > "$OUT/device.json"

jq . "$OUT/device.json"
```

## 5. Sensor inventory

Preferred route:

```sh
termux-sensor -l > "$OUT/sensors.raw.json"
termux-sensor -a -n 1 > "$OUT/sensors.snapshot.json"
```

Then classify each source as:

```text
AVAILABLE
UNAVAILABLE
PERMISSION_REQUIRED
DEGRADED
TOKEN_VAZIO
```

The native `RafSensorApiService` must independently reject unavailable sensors with `ERR_SENSOR_UNAVAILABLE`. Shell output is inventory evidence, not a replacement for runtime validation.

## 6. CPU and cache-visible information

```sh
{
  printf '{\n'
  printf '  "cpuinfo": %s,\n' "$(cat /proc/cpuinfo | jq -Rs .)"
  printf '  "page_size": %s,\n' "$(getconf PAGESIZE 2>/dev/null || printf 0)"
  printf '  "processors_online": %s\n' "$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf 0)"
  printf '}\n'
} > "$OUT/cpu.json"
```

Do not claim direct L1/L2 control. Cache sizes reported by sysfs or native hardware probes are descriptive metadata. Optimization is performed through locality, alignment and bounded buffers.

## 7. Memory probe

```sh
{
  printf '{\n'
  printf '  "meminfo": %s,\n' "$(cat /proc/meminfo | jq -Rs .)"
  printf '  "pressure": %s\n' "$(cat /proc/pressure/memory 2>/dev/null | jq -Rs .)"
  printf '}\n'
} > "$OUT/memory.json"
```

Also capture the package view from ADB:

```sh
adb shell dumpsys meminfo com.termux.rafacodephi \
  > rafaelia-meminfo.txt
```

## 8. Graphics probe

Shell hints:

```sh
{
  printf '{\n'
  printf '  "gles_property": %s,\n' "$(getprop ro.opengles.version | jq -Rs .)"
  printf '  "egl_property": %s,\n' "$(getprop ro.hardware.egl | jq -Rs .)"
  printf '  "vulkan_property": %s,\n' "$(getprop ro.hardware.vulkan | jq -Rs .)"
  printf '  "surfaceflinger": %s\n' "$(dumpsys SurfaceFlinger 2>/dev/null | grep -iE 'GLES|GPU|vendor|renderer|version' | jq -Rs .)"
  printf '}\n'
} > "$OUT/graphics.json"
```

Canonical proof must later come from an APK probe using:

- `glGetString(GL_VENDOR)`;
- `glGetString(GL_RENDERER)`;
- `glGetString(GL_VERSION)`;
- Vulkan physical-device enumeration when available.

No GPU compute route is enabled from properties alone.

## 9. Permission probe

From ADB:

```sh
adb shell dumpsys package com.termux.rafacodephi \
  > rafaelia-package-permissions.txt
```

The generated `permissions.json` must separate:

```text
declared_in_manifest
runtime_granted
runtime_denied
special_access_required
not_applicable_on_sdk
```

## 10. Thermal and battery baseline

```sh
adb shell dumpsys battery > rafaelia-battery.txt
adb shell dumpsys thermalservice > rafaelia-thermal.txt 2>&1
adb shell dumpsys batterystats --charged > rafaelia-batterystats.txt
```

Battery temperature is device temperature evidence, not an ambient-temperature sensor.

## 11. Developer-options test profile

Enable only during laboratory runs:

- USB debugging;
- bug report capture;
- GPU/HWUI profiling when testing UI;
- Vulkan validation layers only in debug builds;
- stay awake while charging only for controlled tests.

Do not leave permanently enabled:

- disable HW overlays;
- force GPU rendering;
- validation layers;
- artificial background-process limits;
- do-not-keep-activities.

Each option changes the system being measured.

## 12. Native artifact audit

For every produced `.so` or ELF:

```sh
llvm-readelf -h -l -d path/to/artifact
llvm-nm -D --defined-only path/to/artifact
llvm-objdump -d path/to/artifact > artifact.disassembly.txt
sha256sum path/to/artifact
```

Record:

- ABI;
- dynamic dependencies;
- exported symbols;
- text/data size;
- build flags;
- source and artifact hashes.

## 13. Acceptance gates

The probe cycle passes only when:

- all six inventory files exist and parse;
- every field has provenance;
- unavailable sensors remain unavailable;
- runtime permissions are distinguished from manifest declarations;
- GPU support is not inferred solely from Android version;
- NEON is selected only after capability confirmation;
- no raw private audio, camera or location payload is retained by default;
- the result can be rerun and diffed.

## 14. Next implementation unit

After this runbook is validated on-device, implement `RafaeliaDeviceProbe` inside the APK so the six inventories are generated without relying on shell parsing. The Termux shell scripts then become an independent cross-check.