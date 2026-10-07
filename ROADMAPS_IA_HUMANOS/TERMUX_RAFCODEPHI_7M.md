# RAFCODEΦ — roteiro 7M | execução controlada

**ROOT:** [Índice](./README.md) · [Contrato estruturado](./7M_INDEX.json) · [Matriz Drive](https://docs.google.com/spreadsheets/d/1m_QeBOrEAsX8UGMeaLADjwnmZvsbDHRM6ZYay9nss7Y/edit)

**Caso causal:** [run 37660128059 / job 112925303754](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions/runs/37660128059/job/112925303754); o link original `#step:7:260681` aponta para uma listagem de ZIP, **não** para o erro. O erro do step 7 aparece no log próximo de `261055`: `cannot seal profile; missing installed entries: bin/sh,libexec/termux-api`. O step 15 também falhou; a classificação de nomes com `:` decorre do HOTSTATE/ledger, como causa separada, não como nova leitura exata do step 15. O job de contrato de custódia foi **success**, diferente do build pesado.

**Estado de referência:** produtor `termux-packages` main @ `97cd0fd25b194b3d857b3f6a2ac330ff0efb003c`; PRs consumidores #481–#484 sobrepostos no HOTSTATE; PRs #485/#486 encerrados sem merge. [Producer build](https://github.com/rafaelmeloreisnovo/termux-packages/actions/runs/37698771017) em `IN_PROGRESS` e [consumer exact-head](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions/runs/37700322813) em `QUEUED` na última leitura desta sessão. Tratar todo estado posterior como **não observado** até novo readback.

## 7M — fluxo, checklist e critérios de saída

### M1 — Meta e limite `CP-01`
- [x] Definir alvo: diagnóstico da cadeia **bootstrap real ARM32/ARM64 → par com hashes → APK → execução física**.
- [x] Estabelecer `claim_allowed=false`, `release_allowed=false`, `physical_android=TOKEN_VAZIO` até provas.
- [ ] Confirmar autoridade atual do responsável por promoção antes de qualquer merge/release.
- **Saída:** objetivo + proibições explícitas; **estado:** `SOURCE_OBSERVED` (não é liberação).

### M2 — Mapa de autoridade `CP-02`
- [x] [`termux-packages`](https://github.com/rafaelmeloreisnovo/termux-packages) = produtor de pacote/bootstrap; [`termux-app`](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi) = consumidor, APK e runtime.
- [x] Vincular [PR #482](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/pull/482) (pin + preflight mínimo) e [PR #484](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/pull/484) (normalização + P0) como **alternativas não cumulativas sem revisão**.
- [ ] Resolver **um** sucessor consumidor pela menor difusão e evidência exata de HEAD, sem abrir outro PR sobreposto.
- **Saída:** `chosen_pr | chosen_sha | owner | reason | supersedes`; **estado:** `BLOCKED_SELECTION`.

### M3 — Medida e causa `CP-03`
- [x] Isolar step 7: falta de entradas instaladas `bin/sh` e `libexec/termux-api`, dentro do seal.
- [x] Isolar step 15: falha independente de transporte/custódia com nome de artefato contendo `:` (evidência via HOTSTATE).
- [x] Registrar defeito **posterior, distinto**: [run 37699530923](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions/runs/37699530923) teve `Permission denied`/exit 126 ao chamar script de bootstrap; há patch produtor preparado, ainda sem PASS atribuído.
- [ ] Comparar esses falsificadores no HEAD selecionado, sem ressuscitar erro já supersedido.
- **Saída:** falha causal confirmada no HEAD ou defeito `SUPERSEDED`; **estado:** `HISTORICAL_CAUSE_OBSERVED`.

### M4 — Mudança mínima `CP-04`
- [ ] No **repo produtor**, testar o patch de invocação do script via Bash (saída/argumentos preservados) em `prep/rafcodephi-bootstrap-exit126-interpreter-20261007@5197a49c3abf2084ce123ef5584dcddc5f2c3783`.
- [ ] No **repo consumidor**, selecionar #482 **ou** #484 após falsificador estático/segurança e revisão de direitos P0.
- [ ] Não alterar runner/Node/libc/bootstrap por aproximação; distinguir falha de fonte e falha de provider.
- **Saída:** diff mínimo revisado + teste focal `PASS` no HEAD específico; **estado:** `PENDING_VERIFICATION`.

### M5 — Matriz de testes `CP-05`
- [ ] Consumir [producer build já em andamento](https://github.com/rafaelmeloreisnovo/termux-packages/actions/runs/37698771017) e [consumer gate já enfileirado](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions/runs/37700322813) antes de solicitar novo run.
- [ ] Primeiro `run-only-fail`: checagens de caminho/symlink, identidade, política de licenças, nome de artefato e contrato de fonte; **só** repetir build integral se novo HEAD realmente exigir.
- [ ] Obter bootstrap fonte real e matriz final de APK para **ambas** `armeabi-v7a` e `arm64-v8a`, com SHA exato e logs.
- **Saída:** `tested_head | run_id | job_id | arch | checks | conclusion`; **estado:** `WAITING_TERMINAL_EVIDENCE`.

### M6 — Manifesto e receipts `CP-06`
- [ ] Preservar `arm.zip`, `aarch64.zip`, manifestos e APKs com SHA-256, identidade, origem, tamanho e job/run verificáveis.
- [ ] Provar que bytes **embutidos** no APK correspondem ao par hash-bound, não apenas ao arquivo de entrada.
- [ ] Checar P0 (copyright, licença, permissões e redistribuição) **antes** de exportar DEB TAR/artefatos externos.
- [ ] Testar o **mesmo APK** em ARM32 e ARM64 físicos; anexar receipt por dispositivo e build ID.
- **Saída:** cadeia `source→build→artifact→embedded→device→receipt` com hashes exatos; **estado:** `BLOCKED_NO_COMPLETE_RECEIPT`.

### M7 — Memória, controle e promoção `CP-07`
- [ ] Só promover depois de `CP-01…CP-06` fechados, revisão independente, direitos P0 e provider protection/enforcement com readback.
- [ ] Se falhar: `TOKEN_VAZIO` ou `FAIL` explícito; anexar evidência e link ao rollback, não apagar estado anterior.
- [ ] μWRITE append-only em [ledger](https://docs.google.com/document/d/1NEU7lg7iUZf1u7lX-SEKuc-knSDGlF5gOx7qzXcJ1yc/edit), atualização pequena em [HOTSTATE](https://docs.google.com/document/d/1d0J5SkF2S2emBq6jLuTkhYVaSOQiWazJWyRCTMJ-iRI/edit) e na [matriz](https://docs.google.com/spreadsheets/d/1m_QeBOrEAsX8UGMeaLADjwnmZvsbDHRM6ZYay9nss7Y/edit) **apenas** se houver novo delta material.
- **Saída:** `APPROVED_WITH_PROOF` ou `HOLD_WITH_RECEIPT`; **estado atual:** `HOLD`.

## Procedimento por checkpoint (humano ou IA)

```text
1. READ: CURRENT_STATE → 7M_INDEX.json → só a evidência do CP ativo
2. LOCATE: repo, PR, sha, run_id, job_id, step
3. VERIFY: authority, direitos P0, SHA do HEAD, estado do provider
4. TEST: menor falsificador; se já rodando, OBSERVE sem reexecutar
5. DECIDE: PASS_EXACT_HEAD | FAIL | NOT_RUN | PENDING | TOKEN_VAZIO
6. RECEIPT: expected/observed, evidência URL, hashes, responsável e rollback
7. μWRITE: somente delta material → Drive ledger → HOTSTATE/planilha
```

**RECIBO MÍNIMO:** `μID | timestamp | source/ref | parent | kind | Δsummary | routes | evidence | gap | next | hash/ref`. O registro é sempre sucessor append-only e nunca apaga evidência anterior.

**Parada segura:** nenhum merge, dispatch pesado, entrega de APK ou claim enquanto faltar autoridade, fonte exata, evidência, direitos P0 ou regra de validação. Falta = `TOKEN_VAZIO`, não zero.

**R3:** `F_ok` = falhas originais isoladas e rota auditável; `F_gap` = sucessor único, producer terminal, provas de APK e execução física, direitos/enforcement; `F_next` = obter terminal readback dos runs já abertos e fechar um falsificador de maior informação sem regressão.
