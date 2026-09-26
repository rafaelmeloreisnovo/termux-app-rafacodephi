# CI Workflow Ownership Matrix

## Entrada humana canônica

Ponto preferencial para operação manual:

`.github/workflows/00_START_HERE.yml`

Nome visível: **00 START HERE — RAFCODEΦ Enterprise**.

O operador escolhe uma rota válida, em vez de combinar livremente parâmetros de dezenas de especialistas. Os especialistas permanecem isolados e auditáveis.

Rotas humanas:

- `01_DIAGNOSTICO`
- `02_ARM32_CANONICO`
- `03_ARM32_NDK29`
- `04_BOOTSTRAP_CANONICO`
- `05_BOOTSTRAP_NDK29`
- `06_EVIDENCIAS`
- `07_E2E`
- `08_VECTRA_V3`
- `09_ENTERPRISE`
- `10_PROVIDER`

## Workflows canônicos por trilha

| Trilha | Workflow | Autoridade | Claim boundary |
|---|---|---|---|
| `operator` | `.github/workflows/00_START_HERE.yml` | entrada humana, roteamento, receipt final | não cria prova física |
| `official` | `.github/workflows/apk_matrix_build.yml` | matriz de build/release | official exige assinatura/gates próprios |
| `artifact` | `.github/workflows/rafcodephi-usable-apk.yml` | materialização de APK candidato | artifact != device proof |
| `debug` | `.github/workflows/run_tests.yml` | testes/smoke de software | test PASS != physical runtime |
| `arm32` | `.github/workflows/_reusable-arm32-compat.yml` | build/ABI ARM32 | physical Android continua TOKEN_VAZIO |
| `bootstrap` | `.github/workflows/beta-real-bootstrap-contract.yml` | contrato estrutural do bootstrap | contract PASS != handset execution |
| `evidence` | `.github/workflows/apk-evidence-gate.yml` | parser/contrato de evidência APK | evidence contract != supplied physical evidence |
| `integration` | `.github/workflows/rafaelia_e2e_product_proof.yml` | integração E2E | CI integration != device proof |
| `metrology` | `.github/workflows/vectra-grade-benchmarks.yml` | contratos/artefatos Vectra em CI | PA physical execution permanece TOKEN_VAZIO |
| `provider-security` | `.github/workflows/provider-protection-gate.yml` | ruleset live do `master` | configuração live pode bloquear o Enterprise preflight |
| `governance` | `scripts/ci/workflow_control_plane.py` | inventário de todos os YML/YAML | discovery != execution |

## Provider protection — autoridade

A trilha `provider-security` usa a cadeia:

`governance/provider/PROVIDER_RULESET_TARGET_20260831.v1.json` → `scripts/ci/provider_protection_contract.py` → `.github/workflows/provider-protection-gate.yml` → receipt publicado → `00_START_HERE` rota `10_PROVIDER`/`09_ENTERPRISE`.

O JSON é a **fonte desejada**; o estado live do GitHub é uma observação separada. O evaluator não altera o ruleset administrativo e nunca converte ausência de permissão em PASS.

## Metadados obrigatórios

Todo workflow ativo deve declarar:

- `ci_track: <debug|internal|official|artifact|ops|deprecated>`
- `ci_abis: <csv ou n/a>`

Direção para workflows ativos:

- `permissions` mínimos explícitos;
- `concurrency` para execução manual quando aplicável;
- `timeout-minutes`;
- `persist-credentials: false` no checkout quando aplicável;
- `workflow_call` para pilares reutilizáveis;
- resumo humano;
- receipt/evidence envelope quando produz evidência.

Ausência histórica entra como `TOKEN_VAZIO`/warning, ou bloqueia quando `strict_governance=true`.

## Semântica de artifact

`source/build PASS → artifact bytes + digest + receipt`

não implica:

`install PASS → device runtime PASS → release certification`.

A trilha `artifact` pode produzir material instalável/candidato sem alterar `PHYSICAL_ANDROID=TOKEN_VAZIO`.

## Separação de responsabilidades

- START HERE escolhe e exige;
- especialista executa seu domínio;
- scanner inventaria;
- gate final avalia apenas os especialistas requeridos;
- receipt registra o que ocorreu;
- evidência física vem de execução física correspondente.

Nenhum pilar herda PASS de outro.

## Deprecated

Workflows marcados `ci_track: deprecated` não são fonte de verdade. Remoção deve ocorrer somente depois de:

`substituto canônico + equivalência observada + janela/rollback + ausência de dependência ativa`.

Não realizar big-bang delete só para reduzir a quantidade de YAML.

## Regra de segurança

`workflow_discovered != workflow_executed != build_pass != apk_proof != device_proof != release_certification`.

`TOKEN_VAZIO != 0 != PASS`.

`URGENCY != GATE_BYPASS`.
