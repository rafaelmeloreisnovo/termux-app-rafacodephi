# ENGINEERING_RUNBOOK_RAFCODEPHI

> Repository: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Initial documentation audit baseline: `b207970fc7a8630a534956cb544350cfd61ba33a`
> Reconciled with current `master`: `3f97ef42ae9756b9f7fb4965b941b5b3048fc8d1` (PR #415 freestanding gate; no overlap with prior 12-file documentation patch).
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

Também são exigidos `libllvm18`, token/schema do manifest, naming do bootstrap e `--architectures`. Falha aqui é **falha de capacidade da fonte** e deve encerrar cedo.

## 4. Fonte do manifest e do par ARM

O manifest NÃO é input arbitrário a ser criado manualmente para satisfazer o receipt. Ele é produzido pelo source-build de `termux-packages` junto com:

```text
artifacts/rafcodephi-bootstrap/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt
artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-arm.zip
artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-aarch64.zip
```

Se o source-build falhar antes, a ausência desses artefatos é consequência.

```text
UPSTREAM FAILURE != DOWNSTREAM MISSING FILE AS ROOT CAUSE
```

## 5. Receipt de sucesso vs receipt de falha

### Sucesso

`scripts/generate_usable_beta_receipt.py` recebe o conjunto completo apenas após sucesso das etapas anteriores; o workflow estrito usa `if: success()`.

### Falha upstream

O workflow grava receipt diagnóstico:

```text
schema=rafcodephi.usable-beta-build/v2
state=UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE
claim_allowed=false
release_allowed=false
physical_android=TOKEN_VAZIO
present_evidence=[...]
missing_evidence=[...]
```

Esse receipt preserva a causa primária.

## 6. Ordem local genérica

1. `./scripts/validate_side_by_side_contract.py`
2. `./scripts/validate_abi_policy_consistency.sh`
3. `./scripts/bootstrap_lowlevel_sync_check.sh`
4. resolver e registrar `termux-packages` quando aplicável;
5. `./scripts/prepare_bootstrap_env.sh`
6. `./scripts/verify_bootstrap_contract.sh`
7. `./scripts/build_apk_matrix.sh`
8. `./gradlew verifyReleaseContract`
9. `./scripts/device_runtime_smoke.sh`
10. `./scripts/device_pkg_smoke.sh`

A execução física não pode ser simulada por documentação ou CI host-side.

## 7. Gate freestanding `pkg` / PRoot / Ninja

A partir do `master` `3f97ef42...`, existe uma superfície separada para testar payloads por um **control gate freestanding**, documentada em:

```text
docs/FREESTANDING_PROOT_PKG_GATE_V1.md
bootstrap/proot_freestanding.c
bootstrap/proot_syscall_bridge.h
scripts/build_freestanding_real_arm_bootstrap.py
.github/workflows/freestanding-runtime-gate.yml
```

### O que é freestanding

O gate `rafproot-fs` é compilado sem libc/heap/GC/CRT/stdio, usa syscalls e deve resultar em ELF estático sem `PT_INTERP`, `DT_NEEDED` ou símbolos externos indefinidos.

### O que NÃO vira freestanding por associação

```text
pkg
apt/dpkg
PRoot
Ninja
Clang
CMake
QEMU
```

continuam payloads externos. O gate só cria uma fronteira de execução/auditoria para observá-los e acioná-los.

### Build/cold-start

```bash
python3 scripts/build_freestanding_real_arm_bootstrap.py --arch all
```

O builder injeta `libexec/rafproot-fs` nos bootstraps ARM/AArch64 e registra SHA/receipt. O workflow host-side pode promover a propriedade do binário para `BUILD_PROVEN`, mas mantém:

```text
device_runtime_state=TOKEN_VAZIO
claim_allowed=false
```

### Probes

```bash
"$PREFIX/libexec/rafproot-fs" --probe
"$PREFIX/libexec/rafproot-fs" --pkg-bootstrap
"$PREFIX/libexec/rafproot-fs" --pkg-vectras
"$PREFIX/libexec/rafproot-fs" --run ninja --version
"$PREFIX/libexec/rafproot-fs" --run proot --version
```

Ausência de executável é evidência `TOKEN_VAZIO`; não é convertida em PASS.

## 8. Floresta de hotfixes

A ordem estratégica vive em `docs/HOTFIX_EXECUTION_FOREST.md`.

```text
vetor → lacuna → hotfix → prova mínima → artefato → promoção epistêmica
```

Nenhum estado sai de `TOKEN_VAZIO` para `DEVICE_PROVEN` sem device real, receipt e comando reproduzível.

## 9. Modo device bloqueante

```bash
DEVICE_SMOKE_REQUIRED=true ./scripts/device_runtime_smoke.sh path/to/app.apk
```

O modo obrigatório falha quando `final_status != DEVICE_VALIDATED`.

## 10. Camada mínima de `pkg`

```bash
./scripts/device_pkg_smoke.sh
```

Um PASS estrutural/bridge/freestanding-host não prova `pkg update` nem `pkg install` físicos.

## 11. Payload source-built RAFCODEPHI ARM/ARM64

No checkout correto e pinado de `termux-packages`:

```bash
./scripts/build-rafcodephi-real-bootstrap.sh --architectures arm,aarch64
```

Depois importe o par com `RAF_BOOTSTRAP_SOURCE=source-built-real` e os caminhos de ZIP/manifest correspondentes. O importador valida identidade/prefixo/par ARM; o estado físico continua separado.

## 12. Repositório binário custom

Enquanto o manifesto declarar:

```text
package_repo_runtime_state=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED
```

não declarar `pkg update`/`pkg install` como funcional em device.

## 13. Processo/zumbi e hot path RAFAELIA

Antes de afirmar ganho geral:

1. medir execuções comparáveis;
2. separar shell/processo de JNI/VCPU;
3. capturar latência p50/p95/p99 e memória;
4. guardar receipts;
5. manter `TOKEN_VAZIO`/`PARCIAL` onde a vantagem não estiver demonstrada.

## 14. Regra de manutenção documental

Toda alteração de workflow que mude produtor, consumidor, nome, schema, condição de execução ou ladder de evidência deve atualizar a superfície normativa correspondente no mesmo ciclo documental.

Auditoria relacionada:

- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
