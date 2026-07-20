# Auditoria rápida da base (build/release/CI/integracao nativa)

## Causas-raiz encontradas

1. ~~**Entrypoints de validação quebrados**: o documento `VALIDATION_COMMANDS_RMRV2.md` referencia três scripts que não existem no repositório.~~ **RESOLVIDO 2026-07-19** — `scripts/rafaelia_ci_smoke.sh`, `rafaelia_env/tools/doctor.sh` e `scripts/rafaelia_upstream_parallel_check.sh` estão presentes no repositório.
2. **Teste de fixture fora do pipeline padrão**: `scripts/test_export_termux_package_manifests_fixtures.py` não é compatível com descoberta de `pytest`, então `pytest` retorna *no tests ran*.
3. **Ruído de manutenção em comentário histórico**: há erro de digitação/edição em comentário de changelog (`...DEBUG_BUILD`to...).
4. **Cobertura de teste limitada**: o validador de parser cobre apenas casos felizes/fallback básico, sem casos de regressão para entradas inválidas e bordas de expansão.
5. ~~**Bloqueador P0 CI**: 28 workflows usavam `actions/checkout@v6`/`@v7` (inexistentes), bloqueando todo CI.~~ **RESOLVIDO 2026-07-20** — todos os 28 workflows agora usam `actions/checkout@v4` + `actions/upload-artifact@v4`.

---

## Tarefa 1 — corrigir erro de digitação

- **Categoria**: typo
- **Problema**: comentário de changelog com token colado (`APK_RELEASE_GITHUB_DEBUG_BUILD`to).
- **Arquivo alvo**: `termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java`.
- **Ação sugerida**:
  - corrigir para `...DEBUG_BUILD` to `...`;
  - revisar bloco de changelog próximo por erros similares de edição.
- **Critério de aceite**:
  - diff apenas documental/comentário;
  - sem alteração funcional;
  - build Java continua íntegro.

## Tarefa 2 — corrigir bug ✅ RESOLVIDA (2026-07-19)

- **Categoria**: bug (developer workflow / validação)
- **Status**: ✅ **RESOLVIDA** — Os três scripts ausentes agora estão presentes no repositório: `scripts/rafaelia_ci_smoke.sh`, `rafaelia_env/tools/doctor.sh`, `scripts/rafaelia_upstream_parallel_check.sh`. A documentação `VALIDATION_COMMANDS_RMRV2.md` está alinhada com a realidade do repo.
- **Problema original**: comandos documentados para validação falham por arquivos ausentes (`scripts/rafaelia_ci_smoke.sh`, `rafaelia_env/tools/doctor.sh`, `scripts/rafaelia_upstream_parallel_check.sh`).
- **Arquivo alvo**: `VALIDATION_COMMANDS_RMRV2.md` (e/ou criação dos scripts faltantes).
- **Critério de aceite** (cumprido):
  - todos os comandos listados no documento executam sem `No such file or directory`;
  - `README`/docs de validação ficam consistentes com a realidade do repo.

## Tarefa 3 — ajustar comentário/discrepância de documentação

- **Categoria**: comentário/documentação
- **Problema**: `VALIDATION_COMMANDS_RMRV2.md` descreve um fluxo de validação que não existe na árvore atual, criando divergência entre "fonte de verdade" e execução real.
- **Arquivos alvo**: `VALIDATION_COMMANDS_RMRV2.md`, opcionalmente `README.md` (se houver seção de validação relacionada).
- **Ação sugerida**:
  - definir uma trilha oficial de validação (host smoke + unit tests + checks nativos suportados);
  - registrar pré-requisitos (JDK/SDK/NDK) e comandos reprodutíveis.
- **Critério de aceite**:
  - documentação reproduzível em ambiente limpo;
  - comandos e artefatos esperados explicitados.

## Tarefa 4 — melhorar um teste

- **Categoria**: melhoria de teste
- **Problema**: arquivo `scripts/test_export_termux_package_manifests_fixtures.py` roda via `main()` manual, mas `pytest` não descobre testes.
- **Arquivo alvo**: `scripts/test_export_termux_package_manifests_fixtures.py`.
- **Ação sugerida**:
  - migrar fixtures para funções `test_*` parametrizadas (`pytest.mark.parametrize`);
  - manter possibilidade de execução direta opcionalmente;
  - integrar no workflow de CI (job de testes Python).
- **Critério de aceite**:
  - `python3 -m pytest -q scripts/test_export_termux_package_manifests_fixtures.py` executa casos e não retorna *no tests ran*;
  - falhas exibem fixture/chave esperada de forma legível.

## Tarefa 5 — fix P0 CI: actions inválidas em todos os workflows ✅ RESOLVIDA (2026-07-20)

- **Categoria**: bug crítico (CI totalmente bloqueado)
- **Status**: ✅ **RESOLVIDA** — 28 workflows corrigidos em 4 commits para `master`.
- **Problema**: 28 de 37 workflows usavam `actions/checkout@v6` ou `actions/checkout@v7` (versões inexistentes no marketplace), bloqueando 100% do CI na etapa de checkout. Idem para `actions/upload-artifact@v7`.
- **Fix aplicado**: substituição global `@v6`/`@v7` → `@v4` em todos os arquivos afetados.
- **Commits**: `99a6d0f`, `fccb442`, `037efb4`, `7c9ceb7`.

---

## Comandos usados nesta revisão

```bash
rg --files -g 'AGENTS.md'
find .. -name AGENTS.md -print
python3 -m pytest -q scripts/test_export_termux_package_manifests_fixtures.py
python3 scripts/test_export_termux_package_manifests_fixtures.py
test -f scripts/rafaelia_ci_smoke.sh; echo smoke:$?
test -f rafaelia_env/tools/doctor.sh; echo doctor:$?
test -f scripts/rafaelia_upstream_parallel_check.sh; echo upstream:$?
```
