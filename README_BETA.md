# Termux Rafacodephi Beta

**Moral operacional:** Beta não é a obra completa. Beta é a primeira lâmpada confiável.

Esta trilha fecha uma versão pequena, honesta e funcional do Termux Rafacodephi. A beta não depende de provar a teoria RAFAELIA completa; ela depende de abrir o app, abrir terminal, executar comando simples, encerrar a sessão, gerar artifact e documentar limites.

## Status beta

| Área | Status | Evidência mínima |
| --- | --- | --- |
| App abre | OK_BETA | Build Android configurado para `com.termux.rafacodephi`; validação em device/emulador ainda é etapa manual do release. |
| Terminal abre | OK_BETA | `TerminalSession.initializeEmulator()` cria pty/subprocess e agora trata falha nativa sem crash direto. |
| Comando básico | OK_BETA | Fluxo de pty/stdout permanece via filas `mProcessToTerminalIOQueue` e handler principal. |
| Terminal fecha | BUG_BETA corrigido | `finishIfRunning()` mata o grupo de processos e mantém fallback no pid do shell. |
| Processo zumbi | RISK_BETA mitigado | `JNI.waitFor()` reexecuta `waitpid()` em `EINTR` e evita status não inicializado. |
| Build APK | OK_BETA | `scripts/build_apk_matrix.sh` gera debug/release unsigned e assina cópias release locais de validação. |
| Logs de erro | OK_BETA | Falha de subprocesso e cleanup registram mensagens legíveis. |
| RMR / BitRAF | EXPERIMENTAL_NOT_BLOCKING | Presentes como módulos/low-level, sem promessa de performance sem benchmark. |
| Claims RAFAELIA | EXPERIMENTAL_NOT_BLOCKING | `sqrt(3)/2`, Fibonacci, Mandelbrot, Poincare, 42K, 10x10x10, BitOmega e ZipRAF ficam fora do critério de aceite beta. |

## Como testar localmente

```bash
./scripts/beta_selftest.py
./gradlew :app:testDebugUnitTest --no-daemon
./scripts/build_apk_matrix.sh
```

## Como gerar APKs com e sem assinatura

- Unsigned/debug e unsigned/release: gerados por `./scripts/build_apk_matrix.sh` em `dist/apk-matrix/unsigned/`.
- Signed/release local de validação: gerado pelo mesmo script em `dist/apk-matrix/signed/` com keystore local de teste em `dist/local-release.keystore`.
- Release oficial assinado continua exigindo secrets/variáveis explícitas; não use keystore local como release de produção.

## Critério de saída

- Sem `BLOCKER_BETA` conhecido no relatório.
- `docs/BETA_READINESS_REPORT.md` e `docs/BETA_KNOWN_LIMITATIONS.md` atualizados.
- `INVENTARIO.md` lista fontes de verdade, artifacts, scripts e riscos.
- Hashes dos APKs publicados em `dist/apk-matrix/SHA256SUMS.txt` quando o build é executado.


## Certification and audit claim notice

This repository does not claim ISO certification, formal ISO compliance, or accredited external audit status. Any ISO/IEC references are internal checklist references or methodological alignment notes only. Certification requires an external accredited audit process and is outside the scope of this repository.

Este repositório não declara certificação formal baseada em ISO, conformidade ISO formal nem auditoria externa acreditada. Qualquer referência a ISO/IEC é apenas checklist interno, referência metodológica ou alinhamento preliminar de boas práticas. Certificação exige processo externo acreditado e está fora do escopo deste repositório.

### Audit/benchmark/runtime trail
- `docs/AUDIT_CLAIMS_POLICY.md`
- `reports/vectra_grade_benchmarks.md`
- `reports/device_runtime_smoke.md`
- `reports/rmr_equivalence.md`
- CI validação não equivale a validação em device real.
- Benchmark definido não equivale a benchmark medido.
