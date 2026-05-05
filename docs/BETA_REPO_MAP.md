# BETA_REPO_MAP

## Estrutura e classificação
- `app/` Android app principal (activities, service, terminal, JNI): **CORE_BETA**.
- `termux-shared/`, `terminal-emulator/`, `terminal-view/`: runtime terminal base: **CORE_BETA**.
- `app/src/main/java/`: ciclo de vida sessão terminal/service/UI: **CORE_BETA**.
- `app/src/main/cpp/`: bootstrap e integração nativa: **CORE_BETA**.
- `app/src/main/cpp/lowlevel/`: módulos lowlevel BitRAF/vcpu/clock/memory e ASM auxiliar: **EXPERIMENTAL_NOT_BLOCKING** (isolar por flags).
- `rmr/` e `rmr/Rrr/`: RAFAELIA/RMR, C/ASM/JNI: **EXPERIMENTAL_NOT_BLOCKING**.
- `mvp/`: assembly MVP autoral: **EXPERIMENTAL_NOT_BLOCKING**.
- `docs/`: documentação técnica e auditorias: **DOC_ONLY**.
- `.github/workflows/`: pipelines CI/CD: **SUPPORT_BETA**.
- `scripts/`: automações de build/bootstrap/hash: **SUPPORT_BETA**.

## Pontos críticos beta
1. SDK Android ausente no ambiente local bloqueia build Gradle.
2. Sessões terminal dependem da `TermuxService` para cleanup.
3. Módulos ASM e RAFAELIA precisam permanecer desativáveis por flag.
4. CI deve gerar artifact + SHA256 mesmo sem release signing oficial.
