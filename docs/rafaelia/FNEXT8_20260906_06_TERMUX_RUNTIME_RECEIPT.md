# RAFAELIA FNEXT8 — 06 — Termux Source/Build/Runtime Receipt Boundary

id: FNEXT8-20260906-06
state: IMPLEMENTED_ON_BRANCH
claim_allowed: false
authority: termux-app-rafacodephi / Android-Termux runtime

## Evidence ladder
`SOURCE_OBSERVED -> WIRED -> BUILD_PROVEN -> RUNTIME_PROVEN -> DEVICE_PROVEN -> REPRODUCED`

## Required device receipt
- `source_commit`
- `build_command_or_workflow`
- `toolchain_versions`
- `abi`
- `artifact_name`
- `artifact_sha256`
- `package_id`
- `device_model`
- `android_version`
- `installation_result`
- `launch_result`
- `runtime_log_hash`
- `ipc_vectras_receipt`
- `regression_suite`
- `token_vazio[]`

## Boundary
CI, source inspection and APK presence must remain separate evidence classes. Physical-device execution is `TOKEN_VAZIO` until a receipt is produced from the target device/run.

## Cross-repo binding
A Termux<->Vectras assertion must identify both immutable commits and the protocol/IPC version. A pointer to a moving branch is insufficient as provenance.

No runtime or device success is asserted by this contract itself.
