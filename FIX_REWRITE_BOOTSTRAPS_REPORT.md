# FIX_REWRITE_BOOTSTRAPS_REPORT

## causa raiz
A task `rewriteBootstraps` chamava `scripts/rewrite_bootstrap.py` e o script encerrava com `SystemExit(1)` genérico ao encontrar path legado hardcoded em ELF, sem classificação explícita por offset/arquivo/motivo e sem política de allowlist estrita para casos conhecidos (ex: `bin/addpart`).

## arquivos alterados
- `scripts/rewrite_bootstrap.py`
- `FIX_REWRITE_BOOTSTRAPS_REPORT.md`

## comando que falhava
- `./gradlew :app:rewriteBootstraps --stacktrace --info --no-daemon`
  - no ambiente atual, a configuração Gradle para antes por ausência de Android SDK (`SDK location not found`), então a execução não chega na task.

## comando que agora passa
- `python3 -m py_compile scripts/rewrite_bootstrap.py` (passa)

## resultado final
- Script atualizado para:
  - coletar todas as ocorrências legacy em ELFs com `arquivo + offset + needle`;
  - bloquear apenas ocorrências não allowlisted com erro explícito e contagem;
  - suportar allowlist estrita por caminho exato com motivo (`bin/addpart`);
  - imprimir log-resumo por ZIP: scan total, legacy encontrados, reescritos, bloqueados e allowlisted.
- Build Gradle completo não pôde ser fechado neste container por bloqueio estrutural externo: Android SDK ausente.

## pendências reais
- Provisionar Android SDK (ou `local.properties` com `sdk.dir`) para concluir:
  - `./gradlew :app:downloadBootstraps --no-daemon`
  - `./gradlew :app:rewriteBootstraps --no-daemon`
  - `./gradlew assembleDebug --no-daemon`
