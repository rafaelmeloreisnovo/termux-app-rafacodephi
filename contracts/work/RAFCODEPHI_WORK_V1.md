# RAFCODEΦ WORK v1 — fechamento operacional integrado

Decision ID: `DEC-RAFCODEPHI-WORK-V1-20260905`
Symbol: `‡`
State: `WORK_CANDIDATE_RECONCILED_SOURCE_FIRST`
Release allowed: `false`
Claim allowed: `false` until the physical and provenance gates below pass.

## 1. Decisão

A primeira entrega `A` não é definida pelo menor conjunto que abre um terminal. Ela é definida pela primeira fronteira operacional completa capaz de sustentar a continuidade real da obra sem depender de voltar para outro ambiente para completar o trabalho.

A bifurcação é:

- `A = RAFCODEPHI_WORK`: estação operacional integrada, reproduzível, recuperável e suficiente para continuar o desenvolvimento da obra.
- `B = RAFCODEPHI_EVOLUTION`: componentes, variantes e pesquisas ainda sem maturidade operacional suficiente para entrar em A.

A fronteira A/B é determinada por `necessidade operacional + maturidade + evidência`, e não pelo nome do repositório.

## 2. Fontes de verdade desta revisão

A leitura é `source-first`:

`TREE → SOURCE → BUILD GRAPH → RUNTIME GRAPH → CI/EVIDENCE → DOCUMENT RECONCILIATION`.

Documentação histórica serve como pista e índice; não prevalece sobre código e artefatos executáveis atuais.

Fontes observadas nesta revisão:

- `rafaelmeloreisnovo/termux-app-rafacodephi@daf6a45f6251630b444cfd9c3b8d343c16709322`;
- `rafaelmeloreisnovo/Vectras-VM-Android@707350658092f030b441c37094037cf288736466`;
- `rafaelmeloreisnovo/termux-packages@cdd3d68d89e1f6051a0798561a586e110d4004b4` como produtor candidato de packages;
- matriz de gates Drive existente, preservada como índice vivo e não como substituto do source.

## 3. O que o código mostra

A superfície operacional já atravessa mais de um repositório:

`Termux/RafCodePhi ↔ Bootstrap/Packages ↔ RMR/LowLevel ↔ Vectras ↔ QEMU/VM ↔ Android runtime`.

Consequentemente, Vectras, QEMU, VM, RMR ou outros componentes não podem ser excluídos de A por categoria. Se forem necessários para continuar o trabalho e passarem os gates, pertencem ao fechamento operacional de A.

Apenas variantes experimentais ou não necessárias ao fluxo operacional podem permanecer em B.

## 4. Fronteira operacional completa de A

`RAFCODEPHI_WORK-v1` só poderá ser considerada utilizável quando um mesmo lineage verificável demonstrar, conforme aplicável ao fluxo real da obra:

1. identidade side-by-side coerente entre Gradle, BuildConfig, manifest, bootstrap e runtime;
2. APK construído e ligado ao commit exato;
3. APK instalado em Android físico;
4. shell e serviço Termux iniciados;
5. bootstrap real instalado e identificado;
6. `dpkg`, `apt/apt-get` e `pkg` executados com backend real;
7. repositório de pacotes acessível, indexado e com origem identificada;
8. ferramentas de trabalho necessárias disponíveis e executadas, incluindo `git`, `clang` e `python`;
9. módulos RAFCODEΦ/RMR/low-level necessários ao fluxo compilados e ligados ao artefato correspondente;
10. caminho Vectras necessário ao trabalho ligado ao mesmo grafo de proveniência;
11. QEMU/VM materializados e executados quando fizerem parte do fluxo operacional da estação;
12. interfaces consumer/guest/IPC necessárias ao trabalho exercitadas;
13. ARM32 e ARM64 cobertos por receipts físicos separados;
14. hashes/manifests de APK, bootstrap, package snapshot e artefatos runtime preservados;
15. rollback/recovery efetivamente exercitado;
16. receipts ligando fonte → transformação → artefato → execução → evidência.

## 5. Side-by-side: contradição de auditoria encontrada

O CI atual falhou porque `scripts/validate_side_by_side_contract.py` ainda exigia literais hard-coded em `TermuxConstants.java` e esperava `com.termux` como code package.

O source atual, porém, usa a arquitetura BuildConfig:

- `app/build.gradle`: `appPackageName = com.termux.rafacodephi`, `appCodePackageName = com.termux.app`;
- `termux-shared/build.gradle`: injeta `TERMUX_PACKAGE_NAME` e `TERMUX_APP_CODE_PACKAGE_NAME` em `BuildConfig`;
- `TermuxConstants.java`: lê `BuildConfig.TERMUX_PACKAGE_NAME` e `BuildConfig.TERMUX_APP_CODE_PACKAGE_NAME`.

Portanto, a falha observada é classificada como `VALIDATOR_DRIFT` até que o gate atualizado seja executado. Ela não autoriza alegar que a identidade instalada está correta; também não autoriza tratar a falha anterior como prova de identidade runtime incorreta.

## 6. Dívida de fronteira canônica Termux ↔ Vectras

Há implementação Termux/RafCodePhi no repositório dedicado e também integração/implementação Termux real dentro do Vectras.

Isso exige uma relação canônica explícita entre as árvores, com pelo menos:

`source_of_truth | consumer_of | shared_component | derived_from | synchronized_from | diverged_from`.

Enquanto essa borda não estiver registrada e testada, o gap `TV-WORK-CANONICAL-IMPLEMENTATION-BOUNDARY` permanece aberto.

## 7. Gates atuais

| Gate | Estado | Leitura source-first | Fechamento exigido |
|---|---|---|---|
| side-by-side static validator | CONTRADICTED/DRIFT | verificador antigo incompatível com BuildConfig atual | executar gate revisado e preservar CI receipt |
| app identity installed | TOKEN_VAZIO | defaults source observados | validar package/paths no APK instalado |
| ARM32 physical | TOKEN_VAZIO | build paths existem | artifact + install + runtime receipt armeabi-v7a |
| ARM64 physical | TOKEN_VAZIO | build paths existem | artifact + install + runtime receipt arm64-v8a |
| bootstrap lineage | TOKEN_VAZIO | múltiplas rotas implementadas | escolher rota canônica + hash + install receipt |
| package stack | TOKEN_VAZIO | produtores e contratos existem | `pkg/apt/dpkg` reais no aparelho |
| package repository | TOKEN_VAZIO | source conhecido | índice/assinatura/origem + device fetch receipt |
| RMR/low-level binding | PARTIAL | código/build graph observados | artifact binding + runtime receipt quando exigido por A |
| Vectras canonical boundary | TOKEN_VAZIO | sobreposição de implementação observada | relação canônica + teste de não-drift |
| Vectras/QEMU/VM E2E | TOKEN_VAZIO | código/tooling material existem | producer→consumer→guest runtime receipt |
| production signing | TOKEN_VAZIO | configuração existe | signed APK hash + cert fingerprint + verification receipt |
| rollback/recovery | PARTIAL | mecanismos existem | execução de rollback/recovery preservada em receipt |

## 8. Contrato de identidade e proveniência

Cada candidata WORK deverá ligar, no mínimo:

`app_repo + app_commit + packages_repo + packages_commit + vectras_repo + vectras_commit + bootstrap_manifest_hash + apk_hash + abi + runtime_artifact_hashes + device_receipt + predecessor_receipt`.

Título, nome do APK, nome do arquivo ou documento não definem identidade.

## 9. Promoção

`WORK_RELEASE_ALLOWED=true` somente quando o fechamento operacional requerido pela obra estiver demonstrado por evidência reproduzível.

Expressão de referência:

`SOURCE_GRAPH ∧ IDENTITY ∧ APK ∧ DEVICE ∧ SHELL ∧ BOOTSTRAP ∧ PACKAGE_STACK ∧ TOOLCHAIN ∧ REQUIRED_LOWLEVEL ∧ REQUIRED_VECTRAS_VM ∧ DUAL_ABI ∧ HASHES ∧ RECEIPTS ∧ ROLLBACK`.

Até então:

`WORK_RELEASE_ALLOWED=false`

`claim_allowed=false`

## 10. O que pode permanecer em B

Não bloqueiam A apenas quando sua ausência não interrompe o fluxo operacional da obra:

- variantes experimentais de ZIPRAF;
- backends VCPU/VM alternativos não usados por A;
- TLS próprio experimental;
- loaders experimentais alternativos;
- novos schedulers não promovidos;
- compiladores experimentais além do toolchain necessário;
- benchmarks de superioridade global.

A categoria do componente nunca é, sozinha, motivo para excluí-lo de A.

## 11. Falsificadores

A candidata WORK-v1 deve falhar o gate se qualquer um ocorrer:

- package/paths instalados divergem da identidade declarada;
- CI valida uma arquitetura anterior ao source atual;
- APK não instala/abre no dispositivo-alvo;
- hash diverge do receipt sem lineage explicável;
- bootstrap não corresponde ao commit/manifest declarado;
- `pkg/apt/dpkg` resolve apenas wrapper/stub sem backend real;
- toolchain necessário não compila/executa no alvo;
- Vectras/QEMU/VM necessário ao trabalho não chega a execução demonstrável;
- duas árvores equivalentes divergem sem relação de proveniência registrada;
- ARM32 ou ARM64 é promovido sem receipt físico correspondente;
- rollback não restaura estado aceitável;
- qualquer claim ultrapassa a classe de evidência observada.

## 12. Próximo caminho verificável

`corrigir validator drift → reexecutar CI → mapear fronteira Termux↔Vectras → congelar bootstrap/package lineage → APK → dispositivo ARM32/ARM64 → package/toolchain → Vectras/QEMU/VM E2E → receipts → rollback → verdict A`.

## 13. Regra epistemológica

`VISÃO ≠ CÓDIGO ≠ ARTEFATO ≠ BUILD ≠ EXECUÇÃO ≠ EVIDÊNCIA ≠ CLAIM`.

`TOKEN_VAZIO ≠ false evidence`; significa evidência necessária ainda não observada.
