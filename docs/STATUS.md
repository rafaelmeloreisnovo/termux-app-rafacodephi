# STATUS (Fonte de Verdade de Build/Release)

> Última revisão: 2026-07-20 (UTC)

Este documento consolida o estado **real e verificável** do pipeline Android (Gradle + NDK + CI) desta fork. A regra é separar promessa de prova: quando não houver backend ou teste real, o estado fica marcado como `TOKEN_VAZIO`, `PARCIAL`, `EXPERIMENTAL`, `NÃO_EXECUTADO` ou `FUTURO`.

## Histórico de Revisões

| Data | Mudanças Principais |
|------|---------------------|
| 2026-07-20 | **CORREÇÃO EPISTÊMICA DO PR #289**: retirada a afirmação de que `actions/checkout@v6`, `actions/upload-artifact@v7` e `gradle/actions/*@v6` eram inexistentes. Esses majors são publicados oficialmente. A branch foi sincronizada com `master`; foi adicionada política versionada, auditor estático e índice canônico orientado por evidência. O sucesso de CI continua `NÃO_EXECUTADO` até existir run conclusivo associado ao HEAD. |
| 2026-07-20 | Alterações anteriores rebaixaram referências de actions para `@v4`/`@v3`. Elas podem ser compatíveis, mas o rebaixamento isolado não prova resolução de bloqueador. |
| 2026-07-12 | Estabelecimento da fonte de verdade de build/release. |

## Verdade canônica atual

- `compileSdkVersion=35`
- `targetSdkVersion=28`
- `minSdkVersion=21`
- ABIs obrigatórias: `armeabi-v7a`, `arm64-v8a`
- `universalApk=true`
- package/applicationId: `com.termux.rafacodephi`

## Estado epistêmico fixo

- **PROVADO**: evidência executável/CI/local confirma o contrato.
- **PROVADO ESTRUTURAL**: código/contrato existe e é validável estruturalmente, mas ainda pede execução, benchmark ou device real para produção.
- **VERSÃO_PUBLICADA**: o major/tag existe na fonte oficial; não prova execução neste repositório.
- **COMPATÍVEL_DECLARADO**: a política local aceita a referência; não prova sucesso do workflow.
- **NÃO_EXECUTADO**: não há run conclusivo associado ao commit analisado.
- **PARCIAL**: parte funciona, mas falta validação de ambiente real ou dependência externa.
- **TOKEN_VAZIO**: wrapper/ponte/nome existe, mas backend real ainda não foi entregue; é melhor explicitar isso do que simular verdade.
- **EXPERIMENTAL**: implementação em exploração, sem contrato de release.
- **FUTURO**: item planejado, não pronto.

## Estado CI/Pipeline (2026-07-20)

- **Política de GitHub Actions**: PROVADO ESTRUTURAL — `docs/CI_ACTION_VERSION_POLICY.md` registra majors atuais e compatíveis com fontes oficiais.
- **Auditor de referências**: PROVADO ESTRUTURAL — `scripts/audit_github_actions_refs.py` classifica SHA fixo, major atual, major compatível, major não permitido e referência flutuante.
- **Workflow de auditoria**: NÃO_EXECUTADO — `.github/workflows/action-reference-audit.yml` foi adicionado, mas precisa de run conclusivo associado ao HEAD do PR.
- **actions/checkout**: COMPATÍVEL_DECLARADO — a base usa major compatível; `v6` também existe oficialmente. Não registrar “P0 resolvido” sem log causal.
- **actions/upload-artifact**: COMPATÍVEL_DECLARADO — `v4` permanece compatível e `v7` é publicado para GitHub.com; compatibilidade GHES/runner deve ser tratada separadamente.
- **gradle/actions**: COMPATÍVEL_DECLARADO — `v3` é antigo, porém publicado; `v6` é documentado oficialmente.
- **apksigner PATH**: PROVADO ESTRUTURAL — os workflows de compatibilidade contêm a configuração; a execução no HEAD deve ser confirmada por run próprio.
- **RAFAELIA pipeline**: PARCIAL — estrutura presente; estado funcional depende de CI, segredos, toolchain e artefatos.
- **Workflows no escopo após este PR**: 38 arquivos esperados em `.github/workflows/`, incluindo o auditor de referências. A contagem deve ser validada pela árvore no HEAD, não tratada como prova de funcionamento.

## Runtime e bootstrap

O bootstrap atual fornece uma base mínima guardada para instalação e diagnóstico, mas ainda não equivale a uma distribuição Termux completa com backend apt real.

- `bin/sh`: existe como wrapper/base mínima quando presente no payload.
- `bin/pkg`: existe como bridge operacional.
- `bin/apt` e `bin/apt-get`: dependem de backend real (`apt`, `dpkg`, `libapt`, repositório e certificados) para instalação real.
- `bin/busybox`: deve delegar para `toybox`/`toolbox` quando possível, ou ser substituído por busybox real.
- `bin/proot`: precisa de `proot.real` ou equivalente para ser considerado pronto.

## Zero-malloc: limite honesto

Zero-malloc confirmado:

- RAFAELIA Direct JNI arena.
- CTI scanner.
- ZIPRAF manifest quando usado estaticamente.
- VCPU state kernel.

Não zero-malloc:

- `baremetal.c` default em matrizes/arena.
- Java side.
- `TermuxInstaller`.
- bootstrap extraction.

## ZIPRAF

ZIPRAF não comprime fisicamente. ZIPRAF cria endereçamento lógico multirresolução sobre bytes existentes. Portanto, a forma correta de documentar é: **1 GB físico pode ser exposto como 264 GB de espaço lógico endereçável, sem aumentar os bytes físicos armazenados.**

## VCPU

Nome técnico atual: **RAFAELIA deterministic VCPU state kernel** / **VCPU telemétrica determinística**. Ainda não é VM completa. Para virar VM completa faltam bytecode, registradores, memória, instruções, loader, executor, syscall table, testes, dump de estado e replay determinístico.

## Fonte de verdade (arquivos canônicos)

- Build e versões Android/NDK: `gradle.properties`.
- Matriz signed/unsigned: `scripts/build_apk_matrix.sh`.
- Contrato operacional: `docs/RUNTIME_TRUTH_TABLE.md`.
- Runbook operacional atual: `docs/ENGINEERING_SYSTEM_RUNBOOK.md`.
- Runbook legado complementar: `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md`.
- Visão macro do projeto: `README.md`.
- Política de versões CI: `docs/CI_ACTION_VERSION_POLICY.md`.
- Auditor estático: `scripts/audit_github_actions_refs.py`.
- Workflow de auditoria: `.github/workflows/action-reference-audit.yml`.
- Índice canônico interativo: `docs/CANONICAL_INDEX.html`.

## Rota mínima para coerência operacional

Para manter a representação documental fiel ao estado real do repositório, a leitura mínima deve seguir esta ordem:

1. `README.md` — entrada institucional e contrato público do fork.
2. `docs/README.md` — hub canônico de navegação e leitura por objetivo.
3. `docs/ENGINEERING_SYSTEM_RUNBOOK.md` — execução real de build/release/CI.
4. `docs/RUNTIME_TRUTH_TABLE.md` — verdade operacional detalhada e limites de runtime.
5. `docs/CI_ACTION_VERSION_POLICY.md` — diferença entre versão publicada, compatibilidade e execução.
6. `scripts/audit_github_actions_refs.py --strict` — inventário estático determinístico.
7. Run do workflow `GitHub Actions Reference Audit` associado ao SHA analisado.
8. `docs/RAFAELIA_CODE_DOC_SYNC.md` e `docs/RAFAELIA_CODE_DOC_SYNC_REPORT.md` — coerência entre narrativa, código, teste e evidência.
9. `docs/CANONICAL_INDEX.html` — mapa visual do estado corrigido.

Se algum documento divergir do código, do script, do workflow, do log ou do artefato correspondente, a documentação deve ser corrigida ou rebaixada para `PARCIAL`, `NÃO_EXECUTADO`, `TOKEN_VAZIO` ou gap explícito.
