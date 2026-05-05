# BETA_CI_ARTIFACTS

- Workflow: `.github/workflows/beta-build.yml`
- Trigger: `push`, `workflow_dispatch`
- Build command: `./gradlew --no-daemon assembleDebug`
- Artifact esperado: APK debug(s) em `app/build/outputs/apk/**`
- Hash esperado: `SHA256SUMS.txt` gerado por `sha256sum`
- Limitações: sem execução de testes instrumentados (device/emulator fora de escopo inicial).
