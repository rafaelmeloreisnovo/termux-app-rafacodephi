# RAFAELIA Sensor Permission Profiles

## 1. Objective

Expose the maximum useful device capability without converting the application into an all-permissions runtime.

The rule is:

```text
capability inventory
  -> selected skill profile
  -> minimum permission set
  -> explicit user grant
  -> governed capture
```

Manifest declaration does not equal runtime authorization.

## 2. Existing protected bridge

The repository already defines:

```text
${TERMUX_PACKAGE_NAME}.permission.RAF_SENSOR_ACCESS
```

and protects `RafSensorApiService` with it.

This typed permission remains the inter-application boundary for sensor requests. New exported sensor components must not bypass it.

## 3. Profiles

### PROFILE_BASE

No new dangerous permission.

Capabilities:

- basic Android `SensorManager` sensors that require no dangerous permission;
- CPU/ABI metadata;
- page size and cache-line metadata exposed by the existing hardware profile;
- RAM and storage capacity available to the app;
- application-local diagnostics.

### PROFILE_MOTION

Permission when required by platform/API level:

```xml
<uses-permission android:name="android.permission.ACTIVITY_RECOGNITION" />
```

Capabilities:

- step counter;
- step detector;
- activity-related virtual sensors when present.

Accelerometer and gyroscope availability must still be discovered dynamically.

### PROFILE_LOCATION

```xml
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
```

Capabilities:

- GNSS/location;
- location-dependent Wi-Fi/Bluetooth discovery behavior on Android 10;
- spatial context for sensor frames.

Rules:

- no background-location request in the first cycle;
- no raw long-term location history by default;
- quantized or redacted exports unless exact position is required by an approved experiment.

### PROFILE_AUDIO

```xml
<uses-permission android:name="android.permission.RECORD_AUDIO" />
```

Rules:

- disabled by default;
- visible active-state notification;
- prefer derived metrics over retained waveform;
- explicit retention setting;
- no silent background capture.

### PROFILE_VISION

```xml
<uses-permission android:name="android.permission.CAMERA" />
```

Rules:

- disabled by default;
- retain derived frame statistics when raw images are unnecessary;
- no hidden capture;
- camera lifecycle must be tied to an explicit foreground operation.

### PROFILE_RADIO

Existing network permissions may support state and connectivity. Discovery and scans must be reviewed against Android 10 location requirements.

Capabilities:

- Wi-Fi state;
- connectivity state;
- Bluetooth state/discovery when explicitly enabled;
- signal and link metrics exposed to normal applications.

This profile does not authorize access to modem/baseband internals.

### PROFILE_LAB_DEBUG

Debug-only capabilities may include:

- detailed diagnostics;
- graphics validation;
- symbol-rich native artifacts;
- expanded logs;
- controlled benchmark endpoints.

Requirements:

- `BuildConfig.DEBUG` or equivalent build gate;
- non-exported debug components;
- no debug permission or interface in release packages;
- diagnostics redaction before sharing.

## 4. Feature declarations

Physical sensors should normally be optional so the APK remains installable on devices without them:

```xml
<uses-feature android:name="android.hardware.sensor.accelerometer" android:required="false" />
<uses-feature android:name="android.hardware.sensor.gyroscope" android:required="false" />
<uses-feature android:name="android.hardware.sensor.compass" android:required="false" />
<uses-feature android:name="android.hardware.sensor.barometer" android:required="false" />
<uses-feature android:name="android.hardware.sensor.light" android:required="false" />
<uses-feature android:name="android.hardware.sensor.proximity" android:required="false" />
<uses-feature android:name="android.hardware.camera.any" android:required="false" />
```

Do not add a feature solely because a documentation page claims the reference device contains it.

## 5. Service exposure

Current principle:

```xml
<service
    android:name="com.termux.app.api.sensor.RafSensorApiService"
    android:exported="true"
    android:permission="${TERMUX_PACKAGE_NAME}.permission.RAF_SENSOR_ACCESS" />
```

Additional requirements:

- verify caller package and permission;
- validate protocol version;
- reject duplicate request IDs;
- bound sampling periods and report latency;
- apply timeout;
- make cancellation idempotent;
- return explicit unavailable/denied/error states;
- never trust client-provided package identity without platform verification in the implementation cycle.

## 6. Special Android access

The following are not ordinary runtime permissions and must not be silently conflated with sensor access:

- battery optimization exemption;
- overlay permission;
- all-files access;
- exact alarms;
- package installation;
- usage access;
- notification policy or accessibility services.

Each requires an independent justification and should not be made a dependency of the sensor matrix unless technically necessary.

## 7. Privacy classes

| Class | Examples | Default handling |
|---|---|---|
| P0 | CPU ABI, page size, sensor presence | retain locally |
| P1 | accelerometer, light, battery metrics | bounded local ledger |
| P2 | Wi-Fi/Bluetooth identifiers | hash/redact |
| P3 | precise location | opt-in, minimize retention |
| P4 | microphone/camera raw data | opt-in, no retention by default |

## 8. Operational gates

```text
DECLARED
  -> REQUESTED
  -> GRANTED or DENIED
  -> AVAILABLE or UNAVAILABLE
  -> CAPTURE_ALLOWED or BLOCKED
```

A missing permission is:

```text
partial + known + blocked
```

A physically absent sensor is:

```text
absent + known + no_op
```

Neither state should produce invented values.

## 9. Manifest review checklist

Before merging permission changes:

- Is the permission required by a committed skill?
- Is it needed on Android 10?
- Is it dangerous, special or signature-level?
- Is runtime request logic present?
- Is denial handled without crash or false output?
- Is the component exported?
- Is a typed permission protecting it?
- Is collection visible to the user?
- Is retention documented?
- Is the capability removed from release when debug-only?

## 10. First implementation recommendation

Do not expand the manifest with camera, microphone and location in the same commit as the device inventory probe.

Sequence:

1. inventory current sensors and system capabilities;
2. generate permission-state inventory;
3. implement PROFILE_BASE;
4. add one dangerous-permission profile per reviewed change;
5. test denial, revocation, service restart and process death;
6. benchmark only after correctness and governance pass.