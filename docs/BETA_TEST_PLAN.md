# BETA_TEST_PLAN

| Teste | Comando | Esperado | Status | Observação |
|---|---|---|---|---|
| Build config | `./gradlew tasks` | listar tasks | FAIL | SDK ausente no ambiente |
| Build debug | `./gradlew assembleDebug` | gerar APK debug | FAIL | SDK ausente |
| Unit tests | `./gradlew test` | executar testes | FAIL | SDK ausente |
| Instrumentation | `./gradlew connectedAndroidTest` | testes em device | FAIL | sem SDK/device |
| Sessão terminal | `echo rafcodephi_beta_ok` | stdout com texto | NEEDS_MANUAL_TEST | executar no app em device |
| Exit code 0 | `exit 0` | retorno 0 | NEEDS_MANUAL_TEST | validar callback/session |
| Exit code 7 | `exit 7` | retorno 7 | NEEDS_MANUAL_TEST | validar propagação |
