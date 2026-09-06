# RAFCODEPHI — Value Evidence Gap Closure — 2026-09-06

## Escopo

Este registro não é laudo contábil, promessa comercial nem avaliação jurídica. Ele separa valor técnico observável, custo de reconstrução e lacunas que ainda não possuem evidência suficiente.

Regra:

```text
VISÃO != CÓDIGO != ARTEFATO != EXECUÇÃO != EVIDÊNCIA != CLAIM
TOKEN_VAZIO != 0
```

## Snapshot observado

- repositório: `rafaelmeloreisnovo/termux-app-rafacodephi`
- branch-base: `master`
- base SHA auditado: `73c9397094c91470a68231a0646ce1e1a33c436f`
- branch de fechamento: `audit/close-value-evidence-gaps-20260906`
- licença principal observada: GPLv3-only para o upstream Termux, com exceções/componentes documentados
- autoria incremental declarada: instituto-Rafael / RafaCodePhi contributors para modificações, sem reatribuir upstream

## Evidência operacional observada

Em 2026-09-06 o conjunto de checks do SHA-base expôs 28 check-runs. No instante da inspeção havia jobs já concluídos com sucesso e jobs ainda em progresso; portanto o estado global não é promovido para `ALL_CI_PROVEN`.

O job `auditor` concluiu como `success` e publicou o artefato:

```text
name   = rafcodephi-auditor-73c9397094c91470a68231a0646ce1e1a33c436f
run_id = 34028794355
job_id = 101474437919
artifact_id = 9987913462
artifact_sha256 = 9098463fbfe05c3a0c17eaba34cfd8d831fde162aae054e39d0d6962093b9891
```

Porém o log revelou:

```text
scripts/rafcodephi_auditor.sh: line 282: skipped: command not found
```

A causa era markdown com backticks dentro de `echo "..."`, permitindo command substitution do shell. O fechamento altera a emissão para literal single-quoted e adiciona teste de regressão.

Estado:

```text
AUDITOR_FALSE_GREEN_FOUND=true
AUDITOR_FALSE_GREEN_FIX_IMPLEMENTED=true
AUDITOR_FALSE_GREEN_REGRESSION_TEST_IMPLEMENTED=true
AUDITOR_FALSE_GREEN_FIX_CI_PROVEN=TOKEN_VAZIO
```

O último campo só muda após CI do SHA que contém a correção.

## Matriz de valor/evidência

| Camada | Estado | Evidência atual | Falta para promoção |
|---|---|---|---|
| código fonte versionado | OBSERVED | árvore Git + SHA | nenhuma para existência |
| diferenciação RAFCODEPHI | OBSERVED | low-level/freestanding, auditoria, contracts, ARM/Android, RAFAELIA | quantificação exaustiva de delta upstream |
| proveniência/licença | OBSERVED_LIMITED | `LICENSE.md`, notices e autoria incremental | auditoria jurídica independente se necessária |
| CI estrutural | PARTIAL_PROVEN | checks e workflows observáveis | fechar todos os checks do SHA-alvo |
| auditor receipt | PROVEN_WITH_DEFECT_ON_BASE | artifact ID + SHA256 | CI da correção sem stderr espúrio |
| APK/build | STRUCTURAL_EVIDENCE_EXISTS | workflows/gates e documentação de fechamento | receipt atual pareado ao SHA-alvo |
| ARM32 físico | TOKEN_VAZIO | contrato e instrumentos existem | execução física + receipt versionado |
| ARM64 físico | TOKEN_VAZIO | contrato e instrumentos existem | execução física + receipt versionado |
| matriz dual ARM | TOKEN_VAZIO | contrato existe | receipts ARM32 + ARM64 pareados |
| `pkg update/install` real | TOKEN_VAZIO | `device_pkg_smoke.sh` e promotion contract | `reports/device_pkg_smoke.json` com `DEVICE_REAL_PKG_VALIDATED` |
| release signing produção | TOKEN_VAZIO | infraestrutura documental | receipt de assinatura/release |
| reproduzibilidade externa | TOKEN_VAZIO | scripts e CI reduzem a lacuna | reprodução independente em ambiente limpo |
| usuários ativos | TOKEN_VAZIO | não inferido do Git | telemetria/registro verificável |
| receita/contratos | TOKEN_VAZIO | não inferido do Git | evidência comercial verificável |

## Interpretação econômica segura

O custo de reconstrução pode ser materialmente superior ao preço de venda porque incorpora tempo de engenharia, especialização Android/Termux/NDK/ARM, CI, governança, auditoria e integração. Contudo:

```text
RECONSTRUCTION_COST != TRANSACTION_PRICE
UPSTREAM_CODE != EXCLUSIVE_RAFCODEPHI_IP
TECHNICAL_DEPTH != COMMERCIAL_TRACTION
```

A existência do upstream GPL não zera o valor das modificações, integração, know-how, documentação, marca, automação e serviços, mas impede tratar o conjunto upstream como IP exclusivo do fork.

## Gates que realmente elevam o valor defendível

1. CI da correção atual sem falso-verde.
2. APK produzido e hash ligado ao mesmo commit.
3. receipt físico ARM32.
4. receipt físico ARM64.
5. matriz dual ARM pareada.
6. `DEVICE_REAL_PKG_VALIDATED` quando o teste mutável for explicitamente autorizado no aparelho.
7. reprodução independente do build/runtime.
8. métricas comerciais verificáveis, se existirem.

## Estado de fechamento desta rodada

```text
SOURCE_OBSERVED=true
LICENSE_BOUNDARY_OBSERVED=true
CI_OBSERVED=true
AUDITOR_DEFECT_IDENTIFIED=true
AUDITOR_DEFECT_FIXED_IN_BRANCH=true
REGRESSION_GUARD_ADDED=true
DEVICE_PROVEN=TOKEN_VAZIO
REPRODUCED_EXTERNALLY=TOKEN_VAZIO
COMMERCIAL_TRACTION=TOKEN_VAZIO
MARKET_VALUE_CERTIFIED=false
```

## R3

```text
F_ok   = evidência source/CI/artifact + correção concreta do falso-verde
F_gap  = device ARM32/ARM64 + reprodução externa + evidência comercial
F_next = fazer o SHA corrigido passar CI e anexar receipts físicos sem promover ausência de evidência
```
