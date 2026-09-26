# RAFAELIA CODE DOC SYNC REPORT

## Sucessor observado — 2026-09-26 / PR #463

- baseline de entrada: `master@988d3c24cf3a4a676ff0cab0512a2436a8d1c704`;
- commit do gate observado: `38e02d288b24a48bbbc0fe0909af541442f05bd0`;
- control-plane run `36245846563`: inventário `success`, policy/runtime `success`, Ω summary `success`;
- `pure_core_source_policy=PASS`; probes ARMv7/AArch64: `compile_exit=0`, zero transferências não terminais detectadas;
- RAFAELIA ZERO compatibility: `PASS`; runtime host simulation: `PASS`;
- `physical_android=TOKEN_VAZIO`, `whole_binary_no_branch=TOKEN_VAZIO`, `claim_allowed=false`;
- `Provider Protection Gate` continua FAIL por configuração do ruleset de `master`, separado deste delta de código.

## F_ok
- `README.md`, `docs/README.md` e `docs/EXCELENCIA_OPERACIONAL_MATRIX.md` agora descrevem a rota até o platô estrutural com foco em coerência operacional.
- `INDICE_DOCUMENTACAO.md` já expõe `docs/README.md`, `docs/EXCELENCIA_OPERACIONAL_MATRIX.md`, `docs/RAFAELIA_CODE_DOC_SYNC.md` e `docs/RAFAELIA_CODE_DOC_SYNC_REPORT.md` como pontos formais de navegação.
- A identidade canônica `com.termux.rafacodephi` permanece confirmada no código e na validação `scripts/validate_side_by_side_contract.py`.

## F_gap
- `docs/STATUS.md` ainda precisava refletir explicitamente o hub moderno, a rota curta de coerência e o runbook operacional atual.
- `docs/RAFAELIA_CODE_DOC_SYNC.md` tinha inventário bruto amplo, mas não priorizava os claims documentais mais críticos para leitura, build, runtime e auditoria.
- A trilha `./run_tests.sh` não fecha totalmente porque `:app:generateRafcodephiBootstraps` falha com `LEGACY_PREFIX_BINARY_RISK`; portanto bootstrap/pkg continuam exigindo linguagem prudente na documentação.

## F_noise
- A existência simultânea de `docs/ENGINEERING_SYSTEM_RUNBOOK.md` e `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md` pode sugerir duplicidade; o primeiro é o runbook operacional atual e o segundo deve ser tratado como complementar/legado.

## F_error
- Referência de runbook em `docs/STATUS.md` apontando só para `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md`, sem destacar o runbook operacional atual.
- Ausência de uma matriz curta de claims prioritários em `docs/RAFAELIA_CODE_DOC_SYNC.md`, o que enfraquecia a aplicação prática da coerência documental.

## F_next
- Atualizar `docs/RUNTIME_TRUTH_TABLE.md` quando o bootstrap real sair de `PARCIAL` para evidência reproduzível em CI ou device.
- Reduzir a ambiguidade entre runbooks caso o legado deixe de ser necessário como referência complementar.
- Sempre revalidar a rota hub → status → runbook → runtime truth → sync report após mudanças estruturais na documentação.
