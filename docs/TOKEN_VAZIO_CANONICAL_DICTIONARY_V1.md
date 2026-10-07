# TOKEN_VAZIO — Dicionário Canônico dos Vazios V1

> Estado: contrato operacional append-only.  
> Base desta rodada: `master@8004dfd5c112061579d54c6bf5a5afdd514ddfdf`.  
> Regra: `TOKEN_VAZIO != 0 != false != PASS != FAIL != NOT_APPLICABLE != NOT_REQUESTED`.

## 1. O vazio é informação

`TOKEN_VAZIO` não é “não sei e vou completar depois”. É um estado que preserva
integridade quando uma conclusão exigiria evidência, autoridade, definição,
proveniência, execução ou controle que ainda não existe.

A dinâmica é:

```text
observação
  -> vazio tipado
  -> causa do vazio
  -> gate de fechamento
  -> próximo probe observável
  -> evidência
  -> successor/closure
  -> novos vazios revelados
```

Assim, **vazio -> parcial -> fechado -> novos vazios -> retroalimentação**.
Descobrir mais pode aumentar o mapa de lacunas sem reduzir a verdade do que já
foi estabelecido.

## 2. Arquivos canônicos

- dicionário machine-readable: `configs/token-vazio-dictionary.v1.json`
- schema de instância: `schemas/token-vazio-instance.v1.schema.json`
- validador: `tools/validate_token_vazio_dictionary.py`
- teste: `tests/test_token_vazio_dictionary.py`
- inventário low-level associado: `docs/FREESTANDING_CODE_INVENTORY_V3.md`

O JSON é a autoridade dos **tipos**. Este documento explica uso e roteamento.

## 3. Famílias

| família | pergunta que preserva |
|---|---|
| definition/scope | “o que exatamente está sendo afirmado?” |
| evidence/execution/artifact | “foi observado, produzido e executado?” |
| provenance | “essas bytes/resultados vêm de qual fonte exata?” |
| conformance/compatibility | “é realmente o algoritmo/ABI/propriedade nomeada?” |
| authority/governance | “há poder e regra para executar/promover?” |
| privacy/security/legal | “podemos fazer isso sem quebrar a fronteira humana/jurídica/técnica?” |
| risk/dependency/environment/toolchain/provider | “o que bloqueia a próxima observação válida?” |
| physical/measurement/causality | “a escala da evidência é a mesma escala do claim?” |

## 4. Instância/roadmap

Cada vazio material deve poder ser reconstruído por um nó mínimo:

```text
GAP_ID | TOKEN_TYPE | source/ref | parent | missing_field | why_empty |
blocker | evidence_needed | falsifier | next_probe | authority | urgency |
closure_gate | routes[L/O/T/P/C/R/I/E/A] | supersedes | hash/ref |
claim_allowed=false
```

Não é necessário preencher campos irrelevantes: use `null` quando o schema
permitir. **Não invente um valor apenas para completar a linha.**

## 5. Fechamento e successor

Um vazio não é apagado. Quando o gate fecha:

1. preserve o registro anterior;
2. crie successor com evidência;
3. marque o anterior como `SUPERSEDED` ou `CLOSED`;
4. crie novos vazios tipados que a nova evidência revelou;
5. ligue tudo por `parent/supersedes/hash_ref`.

`IMPLEMENTED_UNTESTED` continua sendo um estado não-vazio distinto: existe
implementação, mas o teste ainda não autorizou PASS.

## 6. Uso no C/freestanding

Exemplos:

- fonte compilou, mas ELF exato não foi auditado ->
  `TOKEN_VAZIO_ARTIFACT_UNBOUND`;
- ELF auditado em CI, sem aparelho ->
  `TOKEN_VAZIO_PHYSICAL_EVIDENCE_MISSING`;
- comentário diz “branchless”, fonte/assembly não sustentam ->
  `TOKEN_VAZIO_CONTRADICTION` + `TOKEN_VAZIO_CONFORMANCE_UNPROVEN`;
- algoritmo chamado “BLAKE3”, mas sem KAT/conformidade ->
  `TOKEN_VAZIO_CONFORMANCE_UNPROVEN`;
- log contém identificadores privados ainda não minimizados ->
  `TOKEN_VAZIO_PRIVACY_BLOCKED`;
- ruleset/admin/reviewer é necessário ->
  `TOKEN_VAZIO_EXTERNAL_AUTHORITY`.

## 7. Comando de controle

```bash
python3 tools/validate_token_vazio_dictionary.py
python3 tools/validate_token_vazio_dictionary.py --scan
```

`--scan` é inventário, não migração automática. Tokens legados/desconhecidos
são reportados e devem ser tipados em deltas pequenos; não se faz rewrite
massivo que destrua contexto histórico.
