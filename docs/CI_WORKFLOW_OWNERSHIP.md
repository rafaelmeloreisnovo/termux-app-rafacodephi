# CI Workflow Ownership Matrix

## Entrada humana canônica

O ponto de entrada preferencial para operação manual é:

`.github/workflows/00-rafaelia-control-plane.yml`

Nome visível em Actions: **🧭 RAFAELIA — Executar / Diagnosticar**.

O operador escolhe uma intenção (`diagnostico`, `arm32-v7`, `bootstrap-arm32`, `completo-seguro`) em vez de precisar interpretar dezenas de workflows especialistas. Os especialistas continuam existindo como unidades auditáveis; a interface humana fica centralizada.

A governança do conjunto inteiro é feita por `scripts/ci/workflow_control_plane.py`, que descobre todos os `.github/workflows/*.yml|*.yaml` e preserva metadados ausentes como `TOKEN_VAZIO`/warning, nunca como PASS implícito.

## Canonical workflows por trilha

| Trilha | Workflow canônico | Objetivo | ABIs | Política de assinatura | Artefatos |
|---|---|---|---|---|---|
| `operator` | `.github/workflows/00-rafaelia-control-plane.yml` | Entrada humana única; inventário + roteamento dos pilares | conforme missão | não publica release por si só | inventário/step summary + artefatos dos especialistas chamados |
| `official` | `.github/workflows/apk_matrix_build.yml` | Build oficial de release/debug com matriz completa e gates de contrato de release | `armeabi-v7a`, `arm64-v8a`, `universal` | `official` exige release assinado (`use_official_signing=true`), sem fallback implícito para artefato oficial | APKs signed/unsigned por ABI, relatórios de tamanho, checksums, manifest |
| `internal` | `.github/workflows/arme-benchmark.yml` | Benchmark low-level ARM com validação de manifesto | `armeabi-v7a`, `arm64-v8a` | não aplicável (benchmark) | relatórios de benchmark/manifesto |
| `debug` | `.github/workflows/run_tests.yml` | Testes unitários, smoke de bootstrap e inventário de código | host + validações Android | não aplicável (test lane) | relatórios de testes, inventário, logs de smoke |
| `arm32` | `.github/workflows/_reusable-arm32-compat.yml` | Pilar único para build/inspeção ARM32 v7, usado por wrappers canônico e NDK29 | `armeabi-v7a` (valida também artefatos complementares da matriz debug) | verifica assinatura do debug; não promove release | APKs, badging, ELF, assinatura, bootstrap, SHA-256, receipt |
| `bootstrap-contract` | `.github/workflows/beta-real-bootstrap-contract.yml` | Contrato estrutural do bootstrap real usado pela beta | `arm`, `aarch64` | não aplicável | logs/summary de contrato; prova física continua separada |
| `evidence-contract` | `.github/workflows/apk-evidence-gate.yml` | Parser e contratos do gate de evidência APK | `armeabi-v7a`, `arm64-v8a` | não promove artefato | testes do parser/contratos |

## Workflows legados (deprecated)

Estes workflows permanecem temporariamente para compatibilidade, mas **não são fonte de verdade** e devem ser removidos até **2026-09-30**:

- `.github/workflows/apk_arm32_signed_unsigned.yml`
- `.github/workflows/apk_arm32_signed_unsigned_target29.yml`
- `.github/workflows/apk_matrix_artifacts_variants.yml`

Substituição estrutural: consolidar chamadas na matriz canônica (`apk_matrix_build.yml`) com trilha explícita.

## Contratos obrigatórios

Todo workflow novo/ativo em `.github/workflows/*.yml` deve declarar metadados no cabeçalho YAML (comentários):

- `ci_track: <debug|internal|official|ops|deprecated>`
- `ci_abis: <csv com ABIs ou n/a>`

Exemplo:

```yaml
# ci_track: official
# ci_abis: armeabi-v7a,arm64-v8a,universal
name: APK Matrix Build (signed + unsigned)
```

Além dos metadados, a direção V1 para workflows ativos é tornar explícitos, quando aplicáveis:

- `permissions` mínimos;
- `concurrency` para evitar execuções duplicadas;
- `timeout-minutes`;
- `persist-credentials: false` no checkout;
- `workflow_call` quando o workflow for um pilar reutilizável;
- resumo humano via `$GITHUB_STEP_SUMMARY` para tarefas manuais.

A ausência histórica desses campos não é automaticamente corrigida em massa: entra no inventário de migração e é tratada por família para não quebrar checks/branches existentes.

## Regra de segurança da trilha official

- A trilha `official` **não pode** depender de fallback implícito para assinatura de release oficial.
- Se secrets oficiais não estiverem disponíveis, o workflow deve falhar para a trilha `official`, nunca publicar artefato oficial assinado por chave local de validação.

## Fronteira de evidência

`workflow_discovered != workflow_executed != build_pass != apk_proof != device_proof != release_certification`.

Em particular, um PASS no GitHub não transforma `device_runtime_proof=TOKEN_VAZIO` em evidência física. O receipt do Android permanece um gate separado e obrigatório quando a claim depender do aparelho.
