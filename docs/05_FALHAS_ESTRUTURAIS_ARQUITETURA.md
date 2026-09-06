# Falhas Estruturais e Arquiteturais — RAFAELIA/VECTRA_OS — registro auditado

> Repository atual: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Baseline auditado: `b207970fc7a8630a534956cb544350cfd61ba33a`
> Escopo desta revisão: documentação. Hipóteses antigas são preservadas como categorias de investigação, mas não são apresentadas como fatos do source atual sem evidência.

## 1. Método

```text
CURRENT SOURCE / TEST / WORKFLOW
  > CURRENT CONTRACT
  > CURRENT RECEIPT
  > NORMATIVE DOC
  > HISTORICAL / INFERRED ANALYSIS
```

Para qualquer afirmação não verificada diretamente no baseline, usar `HYPOTHESIS` ou `TOKEN_VAZIO`.

## 2. Proveniência e fork chain

A identidade atual observada é:

```text
rafaelmeloreisnovo/termux-app-rafacodephi
```

A cadeia antiga que terminava em `exacordex-crypto/termux-app-rafacodephi (atual)` não é válida como identidade corrente e passa a ser tratada como `HISTORICAL/STALE`.

Qualquer comparação de segurança com upstream deve citar commit/patch concreto no fork atual. A ausência de uma mensagem encontrada por `grep git log` não prova ausência de um patch; portanto instruções antigas do tipo "se grep vazio, aplicar patch imediatamente" não são método de auditoria suficiente.

## 3. Modelo matemático vs implementação

A versão histórica deste documento apresentava várias hipóteses — sample rate de 144 kHz, STFT, período de Pisano, relação com VOID e outros vínculos — como se fossem falhas verificadas do runtime.

Nesta revisão elas são separadas em duas classes:

### 3.1 SOURCE_OBSERVED

Há source atual para o espaço de atratores de 41 estados:

```text
rmr/Rrr/attractor_table.c
rmr/Rrr/attractor_table.h
rmr/Rrr/attractor_table_validator.c
```

`attractor_table.h` declara `count=41`, `period=41`, `dim=7` e range `[0..40]`.

`rmr/Rrr/vectra_pulse.S` também opera com período 41 no path AArch64 auditado.

### 3.2 HYPOTHESIS / RESEARCH QUESTION

Sem apontamento source/receipt específico neste documento, permanecem como perguntas de pesquisa e NÃO como bugs demonstrados:

- relação de um sample rate simbólico/especulativo de 144 kHz com AudioRecord real;
- erro percentual de frequências geométricas derivado de fallback de hardware;
- mapeamento Fibonacci/Pisano como causa do antigo VOID;
- qualquer equivalência direta entre uma fórmula matemática e comportamento físico de áudio;
- claims de resampling não observados no source auditado.

Se esses tópicos forem retomados, devem ter documento próprio com fonte matemática, source consumidor e experimento reproduzível.

## 4. CTI

O source atual `rmr/Rrr/cti_raw_reader.c` documenta cinco modos e implementa `CTI_DELTA_MISS` diretamente no scanner. O algoritmo atual calcula um `expected` determinístico a partir de `prev_crc`, índice físico e `seed`, depois produz `miss_score` a partir da diferença contra o CRC atual.

Portanto a afirmação histórica de que `DELTA_MISS` "provavelmente divide por C_expected da attractor_table" é `STALE/HYPOTHESIS` e não descreve o código corrente.

O source também contém:

```text
rmr/Rrr/cti_scanner_barrier.h
rmr/Rrr/cti_race_condition_validator.c
Makefile: cti-race-condition-gate
```

Isso permite afirmar wiring estrutural de um gate de concorrência; execução física continua separada.

## 5. Testes e gates

A frase histórica "não há evidência de test suite automatizado" é `STALE` como afirmação geral do baseline atual.

O `Makefile` atual contém gates específicos, incluindo ao menos:

```text
attractor-table-complete-gate
lyapunov-convergence-gate
cti-race-condition-gate
```

Além disso, o repositório possui diretório `tests/` com contratos, incluindo o contrato de acesso Termux API.

Isso não significa que toda propriedade do sistema tenha cobertura completa. A classificação correta é:

```text
TEST/GATE INFRASTRUCTURE = SOURCE_OBSERVED
GLOBAL EXHAUSTIVE COVERAGE = TOKEN_VAZIO
```

## 6. Android/package identity

A afirmação histórica de que o manifest principal "provavelmente ainda usa sharedUserId=com.termux" é `STALE`.

`tests/test_termux_api_access_contract.py` exige ausência de `android:sharedUserId` no manifest principal e presença da permissão Termux API com `protectionLevel="signature"`.

Prefixo corrente normativo:

```text
/data/data/com.termux.rafacodephi/files/usr
```

## 7. ZrManifest

O risco histórico de stack allocation não deve ser tratado como ausência de mitigação atual. O baseline contém:

```text
rmr/Rrr/zipraf_index.h
rmr/Rrr/zipraf_manifest_pool.h
rmr/Rrr/zipraf_manifest_pool.c
```

com guard/pool estático relacionado a `ZrManifest`.

Classificação:

```text
MITIGATION_STRUCTURE = SOURCE_OBSERVED
UNIVERSAL_RUNTIME_ABSENCE_OF_STACK_FAULT = TOKEN_VAZIO unless independently proven
```

## 8. BLAKE3/build pipeline

O exemplo histórico de mismatch silencioso em `hotfix_ate_compilar.sh` era inferido e não corresponde ao script corrente auditado.

O arquivo real:

```text
scripts/hotfix_ate_compilar.sh
```

usa `set -euo pipefail` e delega preflight/hashes/build para scripts reais do pipeline. Claims de integridade devem apontar aos verificadores/receipts atuais e aos artefatos materializados.

## 9. Bootstrap e falhas em cascata

A rota `beta-build-libllvm18-unblock.yml` agora possui source-capability preflight antes do source-build caro e separa:

```text
SUCCESS → strict semantic receipt
FAILURE → UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE diagnostic receipt
```

A ausência de manifest/ZIP após falha do produtor não deve substituir a primeira falha como causa-raiz.

## 10. Matriz de risco documental atual

| Área | Estado auditado | O que pode ser afirmado | O que continua aberto |
|---|---|---|---|
| attractor table | SOURCE_OBSERVED | 41-state API/source/validator existem | device/performance geral |
| AArch64 vectra pulse | SOURCE_OBSERVED | quatro correções estão presentes no source | benchmark e hardware real |
| CTI | SOURCE_OBSERVED + GATE_WIRED | algoritmo atual e gate existem | cobertura/race em todos ambientes |
| ZrManifest | SOURCE_OBSERVED | guard/pool estrutural existem | prova runtime universal |
| Termux API identity | TEST_ENFORCED | signature permission e no-main-sharedUserId | instalação/chamada física |
| bootstrap source build | WORKFLOW_WIRED | produtor/gates/preflight/receipts existem | run/artifact/device específico |
| package repository custom | BLOCKED | fail-closed documentado | publicação/assinatura/runtime |
| physical Android | TOKEN_VAZIO | instrumentos podem existir | receipts físicos atuais |
| cobertura global | TOKEN_VAZIO | múltiplos gates existem | exaustividade não demonstrada |

## 11. Regra para análises futuras

Nunca escrever:

```text
"provavelmente o código faz X"
```

como se fosse finding de auditoria.

Escrever:

```text
HYPOTHESIS: X
SOURCE_POINTER: TOKEN_VAZIO
NEXT_VERIFIABLE_CHECK: <arquivo/comando/teste>
```

até o source ser observado.

## 12. Fronteira de claim

```text
claim_allowed=false
physical_android=TOKEN_VAZIO
```

Auditoria relacionada:

- `docs/AUDIT_CLAIMS_POLICY.md`
- `docs/00_BUG_MASTER_INDEX.md`
- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
