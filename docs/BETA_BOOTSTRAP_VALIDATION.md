# BETA_BOOTSTRAP_VALIDATION

## Gates
- **BOOTSTRAP_BUILD_READY**: READY
  - ordem de tarefas Gradle validada e build chain coerente.
- **BOOTSTRAP_NATIVE_EMBED_READY**: READY
  - bootstrap zip obrigatório para embedding nativo mantido.
- **BOOTSTRAP_BLAKE3_READY**: READY
  - BLAKE3 obrigatório e validado (4 ABIs) antes de assemble final.
- **BOOTSTRAP_RUNTIME_PENDING**: PENDING
  - depende de execução ADB real com `beta_runtime_smoke_adb.sh` + `beta_process_cleanup_probe.sh`.
- **BOOTSTRAP_ARTIFACT_HYGIENE_READY**: READY
  - upload sem keystore local sensível.

## Estado final
**BOOTSTRAP_BUILD_READY**, com runtime ainda pendente até teste ADB real.
