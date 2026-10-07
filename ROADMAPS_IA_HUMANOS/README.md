# ROADMAPS_IA_HUMANOS — índice 7M

> Roteador de execução legível por pessoas e IAs. **Não é prova de implementação nem autorização de release.**
>
> Fonte de estado: GitHub Actions, contrato de pin, arquivos do produtor; Drive é espelho documental, não autoridade de build.

## Comece aqui (profundidade 1)

1. **[Incidente ativo: bootstrap ARM32/ARM64 e custódia do APK](RAFCODEPHI_37660128059.md)** — reconstrução, duas causas distintas, checkpoints, falsificadores e gates.
2. **[Checklist estruturado](checkpoints.csv)** — tabela importável por planilhas e agentes, com ID estável, owner, estado, fonte, evidência e próximo passo.
3. [CI do incidente](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions/runs/37660128059/job/112925303754), [workflow produtor APK](../.github/workflows/freestanding-enterprise-closure.yml), [pin exato](../data/contracts/termux-packages-rafcodephi-pin.v1.json).
4. [Repositório produtor dos pacotes](https://github.com/rafaelmeloreisnovo/termux-packages), [construtor](https://github.com/rafaelmeloreisnovo/termux-packages/blob/main/scripts/build-rafcodephi-real-bootstrap.sh).

## Os 7M (navegação e não novos serviços)

| Rota | Pergunta que responde | Saída mínima |
|---|---|---|
| M1 Mapa | Onde estamos? | estado, fronteira, referência exata |
| M2 Microetapas | Qual menor ação seguinte? | um passo reproduzível e rollback |
| M3 Materiais | Que fontes/direitos/deps entram? | origem, SHA, licença, ambiente |
| M4 Medidas | O que foi efetivamente observado? | logs, testes, quantidade, hash |
| M5 Marcos | Qual gate muda o estado? | contrato, status PASS/FAIL/NOT_RUN |
| M6 Mitigação | Quais riscos não podem vazar? | falsificador, contenção, separação causal |
| M7 Memória | Como reconstruir depois? | link, receipt, μWRITE append-only |

## Leitura humana/IA em 30 segundos

1. Abra o incidente e confirme **run, job, app SHA e packages SHA**; não mude versões silenciosamente.
2. Leia M1→M7; vá ao primeiro checkpoint `BLOCKED`/ `FAIL` relevante.
3. Execute apenas o menor teste discriminante. Não repita build de horas quando um preflight local decide a causa.
4. Anexe evidência e atualize estado com nova observação; nunca trate `NOT_RUN` como `PASS`.
5. Promova `release_allowed=true` **somente** via gate explicitamente definido e comprovado.

## Vocabulário invariável

`PASS` = gate observado; `FAIL` = falsificador observado; `NOT_RUN` = não executado; `BLOCKED` = condição de entrada ausente; `TOKEN_VAZIO` = evidência/valor indisponível, diferente de zero. `SOURCE ≠ ARTIFACT ≠ EXECUTION ≠ EVIDENCE ≠ CLAIM`. `IMPLEMENTED_UNTESTED ≠ PASS`.

**Autoridade:** `termux-packages` governa receita/ZIP/debs; `termux-app-rafacodephi` governa workflow, pin, APK, importer e runtime; Drive governa memória/índice/receipt. Correções em produtor ficam no produtor; sem imports cegos ou reescrita de histórico.

## Atualização sem regressão

Para mudança material, acrescente um registro com `μID|timestamp|source/ref|parent|kind|Δsummary|routes|evidence|gap|next|hash/ref`. Não edite receipts anteriores para fingir que o erro nunca existiu. Um checkpoint só pode virar PASS com fonte de evidência nova e exata. Separar provider, source e device. Direitos autorais/licenças: verificar antes de incorporar código de terceiros.
