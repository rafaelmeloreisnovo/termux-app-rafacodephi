# Inventário Beta — Termux Rafacodephi

Data-base: 2026-05-05.

## Fonte de verdade operacional

| Item | Caminho | Papel beta |
| --- | --- | --- |
| App Android | `app/` | APK principal Termux Rafacodephi. |
| Terminal emulator | `terminal-emulator/` | Pty/JNI, sessão terminal, stdout/stdin, wait/cleanup. |
| Terminal view | `terminal-view/` | Renderização/interação terminal. |
| Shared | `termux-shared/` | Constantes, shell, preferências e utilitários Termux. |
| RMR | `rmr/` | Low-level experimental permitido, sem claim de performance beta. |
| RAFAELIA | `rafaelia/` | Pesquisa/integração experimental não bloqueante. |
| Build matrix | `scripts/build_apk_matrix.sh` | Gera APKs com e sem assinatura local e valida arm32/arm64. |
| Beta selftest | `scripts/beta_selftest.py` | Checa docs, contratos de lifecycle e hashes de artifacts presentes. |
| Readiness | `docs/BETA_READINESS_REPORT.md` | Estado, bugs, riscos e comandos da beta. |
| Limitações | `docs/BETA_KNOWN_LIMITATIONS.md` | Claims experimentais e limites conhecidos. |

## Artifacts esperados

Após `./scripts/build_apk_matrix.sh`:

- `dist/apk-matrix/unsigned/termux-app_*debug*armeabi-v7a*.apk`
- `dist/apk-matrix/unsigned/termux-app_*debug*arm64-v8a*.apk`
- `dist/apk-matrix/unsigned/termux-app_*release*armeabi-v7a*.apk`
- `dist/apk-matrix/unsigned/termux-app_*release*arm64-v8a*.apk`
- `dist/apk-matrix/signed/termux-app_*release*armeabi-v7a*-signed.apk`
- `dist/apk-matrix/signed/termux-app_*release*arm64-v8a*-signed.apk`
- `dist/apk-matrix/SHA256SUMS.txt`
- `dist/apk-matrix/ARTIFACT_MANIFEST.txt`

## Classificação beta atual

| Área | Classificação | Observação |
| --- | --- | --- |
| Inicialização app | RISK_BETA | Precisa confirmação em device/emulador. |
| Sessão terminal | OK_BETA | Contrato de criação e erro defensivo presente. |
| Encerramento terminal | OK_BETA | Grupo de processos + fallback pid. |
| Cleanup de recursos | OK_BETA | Idempotente, fd inválido protegido. |
| Zumbi/orfão | RISK_BETA | Mitigado por `waitpid()` robusto; medir em device. |
| Build unsigned | OK_BETA | Via `scripts/build_apk_matrix.sh`. |
| Build signed local | OK_BETA | Via keystore local de validação, não produção. |
| Release oficial assinado | RISK_BETA | Depende de secrets oficiais. |
| RAFAELIA teórico | EXPERIMENTAL_NOT_BLOCKING | sqrt(3)/2, Fibonacci, Mandelbrot, Poincare, 42K, 10x10x10. |
| BitOmega/ZipRAF | EXPERIMENTAL_NOT_BLOCKING | Não bloqueia Termux beta. |

## Comandos canônicos

```bash
./scripts/beta_selftest.py
./gradlew :app:testDebugUnitTest --no-daemon
./scripts/build_apk_matrix.sh
```
