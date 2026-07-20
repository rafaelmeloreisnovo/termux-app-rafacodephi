# Política canônica de versões — GitHub Actions

> Revisão factual: 2026-07-20  
> Escopo: `.github/workflows/*.{yml,yaml}`  
> Regra: **existência da versão, compatibilidade do runner e execução bem-sucedida são provas diferentes**.

## Correção do registro anterior

A afirmação de que `actions/checkout@v6`, `actions/upload-artifact@v7` e `gradle/actions/*@v6` eram versões inexistentes estava incorreta.

A falha real encontrada no `master` era mais específica: **33 arquivos de workflow referenciavam `actions/checkout@v7`**, major que não estava publicado na data desta revisão. O PR #289 corrige somente essas referências para `actions/checkout@v6`.

Foram preservadas as referências oficialmente publicadas que já estavam corretas, incluindo:

- `actions/setup-java@v5`;
- `actions/upload-artifact@v7`;
- `actions/download-artifact@v8`;
- `gradle/actions/wrapper-validation@v6`;
- `gradle/actions/dependency-submission@v6`;
- `gradle/actions/setup-gradle@v6`.

Na data desta revisão, os repositórios oficiais publicam e documentam:

| Ação | Major atual verificado | Majors compatíveis mantidos nesta base | Observação |
|---|---:|---|---|
| `actions/checkout` | `v6` | `v4`, `v6` | `v7` não integra a política porque não estava publicado; `v6` exige runner recente. |
| `actions/setup-java` | `v5` | `v4`, `v5` | Validar JDK e runner antes de atualizar. |
| `actions/upload-artifact` | `v7` | `v4`, `v5`, `v6`, `v7` | `v7` é publicado para GitHub.com; ambientes GHES possuem restrições próprias. |
| `actions/download-artifact` | `v8` | `v4`–`v8` | A versão deve ser verificada separadamente de `upload-artifact`. |
| `gradle/actions/wrapper-validation` | `v6` | `v3`–`v6` | `v3` é antigo, porém publicado; `v6` é documentado oficialmente. |
| `gradle/actions/dependency-submission` | `v6` | `v3`–`v6` | A existência do tag não prova que a submissão funcionou neste repositório. |
| `gradle/actions/setup-gradle` | `v6` | `v3`–`v6` | A partir de versões modernas, `setup-gradle` também executa validação do wrapper. |

Fontes oficiais:

- https://github.com/actions/checkout/releases
- https://github.com/actions/upload-artifact/releases
- https://github.com/actions/download-artifact/releases
- https://github.com/actions/setup-java/releases
- https://github.com/gradle/actions
- https://github.com/gradle/actions/releases

## Estados epistêmicos obrigatórios

- **VERSÃO_PUBLICADA** — o major/tag existe no repositório oficial.
- **COMPATÍVEL_DECLARADO** — a política local aceita o major, mas isso não prova execução.
- **EXECUTADO** — houve workflow run associado ao commit e o job terminou com sucesso.
- **NÃO_EXECUTADO** — não existe run associado ao commit analisado.
- **FALHA_SEM_ETAPA** — o GitHub criou o job, mas não expôs steps/logs suficientes para atribuir causa-raiz.
- **BLOQUEADO** — falha reproduzível atribuída à referência da action, ao runner ou a outro componente identificado.
- **TOKEN_VAZIO** — evidência ainda ausente; não converter em “resolvido”.

## Política operacional deste repositório

1. Não rebaixar nem elevar majors apenas por memória, aparência do número ou suposição de inexistência.
2. Antes de alterar uma action, verificar o repositório oficial e os requisitos mínimos do runner.
3. Alterações em massa devem manter inventário antes/depois e execução CI associada ao HEAD.
4. Um workflow sem run conclusivo não pode ser marcado como `PROVADO` ou `RESOLVIDO`.
5. Uma falha sem steps/logs deve permanecer `FALHA_SEM_ETAPA`; não atribuir causa-raiz por coincidência temporal.
6. Majors antigos podem ser **compatíveis**, mas não são automaticamente “os últimos” nem a solução correta.
7. Preferir SHA imutável em trilhas de release quando a governança do projeto definir processo de atualização automatizada.

## Aplicação ao PR #289

O PR foi refeito sobre o `master` atual e agora contém uma correção delimitada:

- `actions/checkout@v7 → actions/checkout@v6` em **33 workflows** identificados pelo inventário da base;
- nenhuma troca de `upload-artifact@v7`, `download-artifact@v8`, `setup-java@v5` ou `gradle/actions@v6`;
- auditor determinístico para bloquear majors conhecidos fora da política e referências flutuantes;
- índice e status reconstruídos sem declarar sucesso de CI inexistente;
- remoção do workflow temporário usado durante a manutenção do PR.

`claim_allowed = false` enquanto não houver workflow run conclusivo associado ao HEAD do PR.
