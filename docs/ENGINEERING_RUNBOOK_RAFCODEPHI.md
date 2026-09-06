# ENGINEERING_RUNBOOK_RAFCODEPHI

> Repository: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Documentation baseline: `b207970fc7a8630a534956cb544350cfd61ba33a`
> Claim boundary: `claim_allowed=false`; `physical_android=TOKEN_VAZIO` until physical evidence exists.

## 1. Ordem de autoridade

Quando este runbook divergir do source atual, prevalece:

```text
source/test/workflow > machine-readable contract > receipt > documentation
```

Não corrigir um erro de CI caçando manualmente o último arquivo ausente. Primeiro localizar a primeira etapa que deveria ter produzido o arquivo.

## 2. Rotas de `termux-packages`

Existem rotas distintas e elas NÃO devem ser tratadas como um único "pin atual".

### 2.1 Rota semântica por contrato

O contrato vive em:

```text
data/contracts/termux-packages-rafcodephi-pin.v1.json
```

Resolução:

```bash
python3 scripts/resolve_termux_packages_pin.py canonical --json
python3 scripts/resolve_termux_packages_pin.py candidate --json
```

No baseline auditado:

```text
canonical = 837afec42ecf5f9ac1bd8b00e65d143bc23a380b
candidate = 0ffb24a5a6be58316236383a6d249544c39eb3e3
```

O workflow `.github/workflows/beta-build-libllvm18-unblock.yml` usa o canal `candidate` e exige SHA pinado.

### 2.2 Rota beta com SHA explícito próprio

`.github/workflows/beta-build.yml` mantém um SHA exato próprio e permite override explícito via `workflow_dispatch`.

Portanto, qualquer receipt ou documento deve registrar **workflow + ref resolvido**, não apenas "termux-packages atual".

## 3. Preflight antes do build caro

Na rota `beta-build-libllvm18-unblock.yml`, antes do source-build devem existir e ser validados:

```text
scripts/properties.sh
scripts/apply-rafcodephi-build-properties.py
scripts/validate-rafcodephi-build-properties.sh
scripts/run-docker.sh
scripts/build-rafcodephi-real-bootstrap.sh
scripts/generate-bootstraps.sh
packages/libxml2/build.sh
```

Também são exigidos:

- fechamento host contendo `libllvm18` em `packages/libxml2/build.sh`;
- token `RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt` no builder;
- schema `rafcodephi.real-bootstrap-sourcebuild/v1`;
- geração `rafcodephi-bootstrap-${arch}.zip`;
- suporte a `--architectures`.

Falha aqui é **falha de capacidade da fonte** e deve encerrar cedo.

## 4. Fonte do manifest e do par ARM

O manifest NÃO é input arbitrário a ser criado manualmente para satisfazer o receipt. Ele é evidência produzida pelo source-build de `termux-packages` junto com:

```text
artifacts/rafcodephi-bootstrap/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt
artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-arm.zip
artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-aarch64.zip
```

Se o source-build falhar antes, a ausência desses três artefatos é consequência.

Invariante de diagnóstico:

```text
UPSTREAM FAILURE != DOWNSTREAM MISSING FILE AS ROOT CAUSE
```

## 5. Receipt de sucesso vs receipt de falha

### Sucesso

`scripts/generate_usable_beta_receipt.py` só deve receber o conjunto completo depois de todas as etapas anteriores terem sucesso. O workflow `beta-build-libllvm18-unblock.yml` chama o receipt estrito com `if: success()`.

### Falha upstream

Quando qualquer etapa anterior falha, o workflow grava um receipt diagnóstico com:

```text
schema=rafcodephi.usable-beta-build/v2
state=UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE
claim_allowed=false
release_allowed=false
physical_android=TOKEN_VAZIO
present_evidence=[...]
missing_evidence=[...]
```

Esse receipt existe para preservar a causa primária, não para transformar arquivos downstream ausentes em causa-raiz.

## 6. Ordem única de execução local do ferreiro RAFCODEΦ

Para a rota local genérica:

1. `./scripts/validate_side_by_side_contract.py`
2. `./scripts/validate_abi_policy_consistency.sh`
3. `./scripts/bootstrap_lowlevel_sync_check.sh`
4. resolver e registrar a origem de `termux-packages` quando a rota depender dela;
5. `./scripts/prepare_bootstrap_env.sh`
6. `./scripts/verify_bootstrap_contract.sh`
7. `./scripts/build_apk_matrix.sh`
8. `./gradlew verifyReleaseContract`
9. `./scripts/device_runtime_smoke.sh`
10. `./scripts/device_pkg_smoke.sh`

A execução física não pode ser simulada por documentação ou CI host-side.

## 7. Floresta de hotfixes

A ordem estratégica completa dos hotfixes vive em:

- `docs/HOTFIX_EXECUTION_FOREST.md`

Cada hotfix deve manter:

```text
vetor → lacuna → hotfix → prova mínima → artefato → promoção epistêmica
```

Nenhum estado sai de `TOKEN_VAZIO` para `DEVICE_PROVEN` sem device real, receipt e comando reproduzível.

## 8. Modo device bloqueante

```bash
DEVICE_SMOKE_REQUIRED=true ./scripts/device_runtime_smoke.sh path/to/app.apk
```

O modo obrigatório falha quando `final_status != DEVICE_VALIDATED`.

## 9. Camada mínima de `pkg`

```bash
./scripts/device_pkg_smoke.sh
```

A camada mínima inspeciona pelo menos:

```bash
cat --help
ls "$HOME"
clear
grep x /dev/null
pkg help
apt help
```

Um PASS estrutural/bridge não prova `pkg update` nem `pkg install`.

## 10. Payload source-built RAFCODEPHI ARM/ARM64

No checkout correto e pinado de `termux-packages`:

```bash
./scripts/build-rafcodephi-real-bootstrap.sh --architectures arm,aarch64
```

Depois importe o par:

```bash
export RAF_BOOTSTRAP_SOURCE=source-built-real
export RAF_REAL_BOOTSTRAP_ZIP_ARM=../termux-packages/artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-arm.zip
export RAF_REAL_BOOTSTRAP_ZIP_AARCH64=../termux-packages/artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-aarch64.zip
export RAF_REAL_BOOTSTRAP_MANIFEST=../termux-packages/artifacts/rafcodephi-bootstrap/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt
./scripts/prepare_bootstrap_env.sh --print-env
```

O importador exige identidade/prefixo corretos e valida o par ARM/AArch64. O estado físico continua separado.

## 11. Repositório binário custom

Enquanto o manifesto declarar:

```text
package_repo_runtime_state=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED
```

não declarar `pkg update`/`pkg install` como funcional em device. O bloqueio é intencional para impedir consumo de binários upstream incompatíveis com o prefixo RAFCODEPHI.

## 12. Processo/zumbi e hot path RAFAELIA

Antes de afirmar ganho geral:

1. medir execuções comparáveis;
2. separar shell/processo de JNI/VCPU;
3. capturar latência p50/p95/p99 e memória;
4. guardar receipts em `reports/`;
5. manter `TOKEN_VAZIO` ou `PARCIAL` quando a vantagem não estiver demonstrada.

## 13. Regra para manutenção deste documento

Toda alteração de workflow que mude produtor, consumidor, nome, schema ou condição de execução de artefato deve atualizar este runbook e `docs/BOOTSTRAP_SOURCE_CONTRACT.md` no mesmo ciclo documental.

Auditoria relacionada:

- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
