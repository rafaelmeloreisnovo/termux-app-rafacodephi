# Audit Claims Policy

> Repository: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Documentation audit baseline: `b207970fc7a8630a534956cb544350cfd61ba33a`

## Regra central
Este repositório não declara certificação ISO, conformidade formal ISO nem auditoria externa acreditada.

Além disso, documentação não pode promover uma observação estrutural para prova de build, runtime ou device sem evidência correspondente.

## Hierarquia de autoridade documental

Quando documentação e implementação divergirem, use:

```text
CURRENT SOURCE / TEST / WORKFLOW
  > CURRENT MACHINE-READABLE CONTRACT
  > CURRENT RECEIPT / REPORT
  > NORMATIVE DOCUMENTATION
  > HISTORICAL REPORT
  > HYPOTHESIS / INFERRED EXAMPLE
```

Trecho inferido não é prova de que o código existe. Documento histórico não prova estado atual. Ausência de evidência é `TOKEN_VAZIO`, não PASS.

## Estados permitidos para afirmações técnicas

- `SOURCE_OBSERVED`
- `TEST_ENFORCED`
- `WORKFLOW_WIRED`
- `BUILD_PROVEN`
- `RUNTIME_PROVEN`
- `DEVICE_PROVEN`
- `REPRODUCED`
- `HISTORICAL`
- `HYPOTHESIS`
- `STALE`
- `TOKEN_VAZIO`

`RESOLVED`, `PROVADO`, `FUNCIONAL` ou equivalentes devem trazer a classe de evidência e um apontamento verificável.

## Invariantes de claim

```text
VISÃO != CÓDIGO != ARTEFATO != EXECUÇÃO != EVIDÊNCIA != CLAIM
TOKEN_VAZIO != 0
fixture != live
heurística != prova
falha downstream != causa-raiz automaticamente
```

No baseline desta auditoria:

```text
claim_allowed=false
physical_android=TOKEN_VAZIO
```

Esses estados não podem ser ampliados por redação.

## Termos permitidos
- alinhado a boas práticas
- inspirado em ISO/IEC
- checklist interno
- referência metodológica
- mapeamento preliminar
- estruturalmente implementado, quando apontado ao source
- enforced por teste, quando apontado ao teste
- wired em workflow, quando apontado ao workflow

## Termos proibidos sem certificação formal
- certificado ISO
- ISO certified
- ISO compliant
- compliance ISO garantido
- conforme ISO
- auditoria certificada

## Documentação histórica e inferida

Material histórico pode permanecer para cadeia de custódia, desde que não seja confundido com estado corrente. Exemplos de código marcados como `inferido`, `presumido`, `provável` ou equivalentes devem ser classificados como `HYPOTHESIS` até confronto direto com o source atual.

Documentos que mencionem owner/repositório anterior devem deixar explícito quando a referência é histórica. A identidade corrente deste repositório é `rafaelmeloreisnovo/termux-app-rafacodephi`.

## Aplicação no projeto

Benchmarks, artifacts, relatórios e verificações internas são evidências técnicas internas, não certificação. Não representam certificação formal e não substituem auditoria externa acreditada.

Para o ledger da auditoria documental de 2026-09-06, consultar:

- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
