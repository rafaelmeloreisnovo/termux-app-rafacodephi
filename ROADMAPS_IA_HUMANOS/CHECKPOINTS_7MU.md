# RAFCODEΦ — 7 μpassos verificáveis

[↩ Índice humano/IA](./README.md) · [JSON tipado](./index.v1.json) · [Run incidente](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions/runs/37660128059/job/112925303754).

**Método:** cada etapa fecha apenas quando `SOURCE + AUTHORITY + EXECUTION_TARGET + EVIDENCE_RULE + RECEIPT` estiverem determinados; `TOKEN_VAZIO` não é zero nem PASS. Cada linha de checklist significa uma *condição verificável*, não que a condição já tenha passado. STATUS neste arquivo = fotografia orientativa de 2026-10-07; revalidar no provedor antes de executar.

## μ1 — Estado e autoridade

**Intenção:** fechar uma cadeia única produtor → bootstrap → APK → instalação → receipt, sem misturar lanes.
**Checkpoint C1 / situação:** fonte histórica identificada; destino de promoção **não autorizado**.

- [x] Vincular run/job original e separar consumidor de produtor.
- [x] Ler [START HERE V2.2](https://docs.google.com/document/d/1T4OIN2RC1I-KFsdaQTCqieeZUGYBXissWrdrZMG0DN8/edit) e [HOTSTATE V4.5](https://docs.google.com/document/d/1d0J5SkF2S2emBq6jLuTkhYVaSOQiWazJWyRCTMJ-iRI/edit) antes do histórico.
- [ ] Verificar HEADs atuais, autoridade de branch/PR e estado do runner diretamente no GitHub.
- [ ] Selecionar exatamente **um** PR consumidor e um commit produtor. Evitar quatro PRs sobrepostos aprovados como se fossem independentes.

**Saída:** `consumer_repo/ref/sha`, `producer_repo/ref/sha`, autoridade de ambos, `READY_FOR_PATCH|HOLD`.
**Stop:** falta de autoridade, SHA atual ou direitos → `ROUTE_STATE=BLOCKED`.

## μ2 — Fronteiras causais

**Checkpoint C2 / situação:** **PASS_SCOPED_LOG**, não PASS de build.

- [x] Ler step 7: `cannot seal profile; missing installed entries: bin/sh,libexec/termux-api`, `build_exit=1`.
- [x] Isolar step 15: upload falha separadamente quando nome `.deb` inclui `:` (epoch Debian).
- [x] Marcar steps 8–13 como `SKIPPED_BY_UPSTREAM`; job de contratos `success` não implica APK.
- [ ] Inspecionar o pacote produtor e a coleta `SYMLINKS.txt` no commit selecionado para provar por que entradas não aparecem no perfil.
- [ ] Conferir erro `exit 126` em execução posterior separadamente; não usá-lo como explicação retroativa do step 7 antigo.

**Falsificador:** se os bytes/links esperados existem e o selador ainda falha, investigar normalização da lista e o consumidor do contrato, não inventar ausência de pacote.

## μ3 — Falsificadores baratos

**Checkpoint C3 / situação:** `PENDING_EXACT_HEAD`. Rodar somente testes locais/CI curtos de source/contrato no HEAD escolhido.

- [ ] Teste produtor: instalação de `bin/sh` e `libexec/termux-api`, destinos com `./`, casos ausente/conflito/idempotência.
- [ ] Teste launcher: `bash script` versus invocação direta sem bit executável; preservar argumentos e exit code do produtor sem mascarar falha.
- [ ] Teste upload: dado um nome `ca-certificates-java_1:2026.05.14_all.deb`, o upload não deve incluir caminho inválido e o manifesto deve manter SHA-256 e identidade original.
- [ ] Teste segurança: normalizar prefixo `./` sem aceitar `..`, destino absoluto, links duplicados, inconsistência de hash.
- [ ] Teste direitos: não transportar/publicar `.deb` cru sem autoria, licença e permissões verificadas.

**Regra:** `fast fail` → corrigir uma causa por vez; `fast PASS` → permite observar CI pesado, **não** promove APK. Guardar SHA e comandos de teste exatos.

## μ4 — Um produtor, um consumidor

**Checkpoint C4 / situação:** `HOLD` até reconciliação dos PRs.

- [ ] Revisar [#481](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/pull/481), [#482](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/pull/482), [#483](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/pull/483), [#484](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/pull/484), [#487](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/pull/487) sem presumir que qualquer seja vencedor.
- [ ] Comparar alterações/HEADs para escolher sucessor mínimo que englobe **apenas** fixes provados; preservar PRs e commits como histórico, sem cherry-pick cego.
- [ ] Registrar autoridade da fonte produtora `termux-packages` e atualizar pin imutável somente depois de validar seu commit.
- [ ] Evitar novo patch duplicado ou rerun histórico de `1e63d81...`.

**Saída:** decisão `selected_pr`, `selected_head`, `producer_sha` e tabela supersedes. Sem decisão validada, permanecer `HOLD`.

## μ5 — Gates exatos de CI

**Checkpoint C5 / situação:** `NOT_PROVEN_EXACT_HEAD`.

- [ ] Confirmar fast contracts no HEAD consumidor selecionado.
- [ ] Consumir **uma** execução completa do produtor no SHA selecionado para `armeabi-v7a` e `aarch64`, sem iniciar outra se uma equivalente estiver em curso.
- [ ] Atestar para cada ABI: `bootstrap.zip` presente, parent manifest, hash SHA-256 verificável, links instalados e manifesto de pacotes consistente.
- [ ] Comprovar transporte de artefatos e readback dos arquivos realmente publicados; log do workflow ≠ existência de artifact.
- [ ] Comprovar APK matrix, hashes de bootstrap embutidos e receipt ligado ao commit/artefato exato.
- [ ] Tratar falha de provider/artifact como eixo separado de erro do código fonte. Não marcar PASS se job/step foi `SKIPPED`.

**Saída:** URL do run, job, conclusão, SHA, `artifacts[].id`, manifests e digests. Qualquer peça ausente → `PARTIAL_OR_TOKEN_VAZIO`.

## μ6 — Custódia, direitos e dispositivo

**Checkpoint C6 / situação:** `BLOCKED`.

- [ ] Fazer gate P0 de autoria/licença/permissão/compatibilidade para qualquer redistribuição de `.deb` de terceiros.
- [ ] Inspecionar `SHA256SUMS`/hashes com os bytes correspondentes e cadeia source→artifact, sem substituir payload ausente por manifesto.
- [ ] Provar proteção server-side do branch/ruleset e impossibilidade de bypass por ator não autorizado; `403` ou leitura não observada não prova ausência nem enforcement.
- [ ] Instalar exatamente o APK hash-ligado nos dispositivos físicos ARM32 e ARM64, coletar ABI, SDK, package, SHA, `install/start/pkg` e falhas, com timestamp.
- [ ] Manter `physical_android=TOKEN_VAZIO` até receipts físicos verificáveis. Hosted cross-compilation ≠ execução física.

**Saída:** direitos e custodiante, hash de APK, receipt device e provider. Qualquer autorização pendente → `release_allowed=false`.

## μ7 — Decisão, rollback e μWRITE

**Checkpoint C7 / situação:** `HOLD` até C1–C6 exigidos para promoção.

- [ ] Aplicar matriz `PASS | FAIL | NOT_RUN | BLOCKED | TOKEN_VAZIO` a cada gate, com evidência e data.
- [ ] Se um gate falhar, bloquear release/promoção; documentar a causa nova sem mascarar o primeiro falsificador.
- [ ] Registrar **um único delta material** no [μWRITE LEDGER](https://docs.google.com/document/d/1NEU7lg7iUZf1u7lX-SEKuc-knSDGlF5gOx7qzXcJ1yc/edit) no formato `μID|timestamp|source/ref|parent|kind|Δsummary|routes|evidence|gap|next|hash/ref`.
- [ ] Atualizar CURRENT_STATE apenas se a realidade verificável mudar; apontar para este índice, não copiar o histórico inteiro.
- [ ] Reverter por novo commit/supersedes ao invés de reescrever recibos; validar readback da fonte e do documento.

**Decisão terminal:** `RELEASE` somente após todos gates necessários comprovados e autoridade explícita; caso contrário `HOLD`. Nunca inferir autonomia operacional de um roadmap.

## Template de checkpoint humano e IA

```text
µID=<id estável> | step=C1..C7 | status=PASS|FAIL|NOT_RUN|BLOCKED|TOKEN_VAZIO
repo=<owner/name> | ref=<branch/tag> | sha=<commit>
source=<file:line/url> | authority=<owner> | execution_target=<runner/device>
observed_at=<timestamp> | command=<literal command or TOKEN_VAZIO>
evidence=<run/job/artifact/hash/url or TOKEN_VAZIO>
cause=<observed/candidate/unresolved> | parent=<prior receipt>
next=<smallest falsifier> | rollback=<ref/commit> | claim_allowed=false
```

**Anti-regressão:** um checkpoint concluído é específico de `repo@sha + ABI + toolchain + environment + data + gate`. Alterar um componente reabre **somente** o gate afetado; não transfere PASS para outro escopo.

**R3:** `<F_ok: índice causal e condições explícitas, F_gap: seleção e prova exatas ainda não fechadas, F_next: verificar head único e falsificador barato antes de build terminal>`.
