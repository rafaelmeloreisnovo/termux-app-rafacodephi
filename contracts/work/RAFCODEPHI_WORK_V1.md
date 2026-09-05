# RAFCODEΦ WORK v1 — contrato da primeira entrega utilizável

Decision ID: `DEC-RAFCODEPHI-WORK-V1-20260905`
Symbol: `‡`
State: `WORK_CANDIDATE_SPECIFIED`
Release allowed: `false`
Claim allowed: `false` until runtime gates below pass.

## 1. Decisão

A prioridade passa a ser obter uma primeira entrega A realmente utilizável para trabalho diário antes de exigir fechamento da plataforma evolutiva completa.

A bifurcação é:

- `A = RAFCODEPHI_WORK`: ambiente operacional estável, reproduzível, rollback-capable e compatível com o fluxo Termux.
- `B = RAFCODEPHI_EVOLUTION`: continuidade experimental da arquitetura RAFCODEΦ.

`A` pode consumir apenas componentes de `B` que tenham passado pelos gates de promoção.

## 2. Base observada

Fonte app: `rafaelmeloreisnovo/termux-app-rafacodephi@daf6a45f6251630b444cfd9c3b8d343c16709322`.

Verdade estrutural atualmente documentada no repositório:

- applicationId/package: `com.termux.rafacodephi`;
- compileSdkVersion: `35`;
- targetSdkVersion: `28`;
- minSdkVersion: `21`;
- ABIs obrigatórias: `armeabi-v7a`, `arm64-v8a`;
- APK universal habilitado;
- `safe-core` estruturalmente fechado/candidato conforme gates existentes;
- `functional-distribution` ainda bloqueada;
- `full-platform` permanece experimental.

## 3. Escopo mínimo da primeira A

A primeira `RAFCODEPHI_WORK-v1` só poderá ser considerada utilizável quando, no mesmo lineage de build, estiverem observados e ligados por receipts:

1. APK construído;
2. APK instalado no Android físico;
3. shell aberto;
4. bootstrap real instalado;
5. `dpkg` executado;
6. `apt`/`apt-get` executado;
7. `pkg update` executado;
8. `pkg install` de pacote de teste executado;
9. `git` executado;
10. `clang` compilando e executando hello/minimal probe;
11. `python` executando probe mínimo;
12. repositório de pacotes acessível e com origem identificada;
13. hashes/manifest do APK/bootstrap/packages;
14. device/runtime receipt;
15. rollback definido e testável.

## 4. Gates atuais

| Gate | Estado | Evidência atual | Fechamento exigido |
|---|---|---|---|
| app identity | OBSERVED | `com.termux.rafacodephi` documentado | revalidar no APK instalado |
| build metadata | OBSERVED/STRUCTURAL | docs/status + config existente | build receipt do mesmo commit |
| ARM32 build | TOKEN_VAZIO | estrutura existe; receipt físico ausente | artifact+device receipt armeabi-v7a |
| ARM64 build | TOKEN_VAZIO | estrutura existe; receipt físico ausente | artifact+device receipt arm64-v8a |
| bootstrap source-built | TOKEN_VAZIO | pipeline/contrato existe | hash+manifest+install receipt |
| dpkg runtime | TOKEN_VAZIO | não provado fisicamente | comando+exit+artifact lineage |
| apt runtime | TOKEN_VAZIO | não provado fisicamente | update/install transcript+receipt |
| pkg runtime | TOKEN_VAZIO | wrapper/backend completo não provado | `pkg update` + `pkg install` reais |
| package repository | TOKEN_VAZIO | repo fonte conhecido, publicação/runtime não fechados | signed/indexed repo + device fetch |
| git runtime | TOKEN_VAZIO | não fechado neste contrato | probe no device receipt |
| clang runtime | TOKEN_VAZIO | não fechado neste contrato | compile+run probe no aparelho |
| python runtime | TOKEN_VAZIO | não fechado neste contrato | probe no aparelho |
| production signing | TOKEN_VAZIO | signing oficial separado/opt-in | signing receipt e cert fingerprint |
| rollback | PARTIAL | arquitetura de receipts/rollback existe | rollback executado/validado |

## 5. Contrato de identidade

Cada entrega WORK deverá ligar:

`app_repo + app_commit + packages_repo + packages_commit + bootstrap_manifest_hash + apk_hash + abi + device_fingerprint + runtime_receipt`.

Título, nome do APK ou nome de arquivo não definem identidade.

## 6. Contrato de packages

Fonte produtora candidata da A:

`rafaelmeloreisnovo/termux-packages`

O app deverá consumir snapshot/commit pinado. Estado flutuante de `main` não pode definir uma release WORK.

## 7. Promoção

`WORK_RELEASE_ALLOWED=true` somente se:

`APK_BUILD ∧ DEVICE_INSTALL ∧ SHELL ∧ BOOTSTRAP ∧ DPKG ∧ APT ∧ PKG ∧ REPO ∧ GIT ∧ CLANG ∧ PYTHON ∧ HASHES ∧ DEVICE_RECEIPT ∧ ROLLBACK`.

Até então:

`WORK_RELEASE_ALLOWED=false`
`claim_allowed=false`

## 8. Limites

Não bloqueiam WORK-v1, salvo dependência direta comprovada:

- ZIPRAF experimental;
- VCPU/VM completa;
- TLS próprio;
- loaders experimentais;
- novos schedulers;
- novos compiladores gerais;
- benchmarks de superioridade global.

## 9. Falsificadores

A candidata WORK-v1 deve falhar o gate se qualquer um ocorrer:

- APK não instala/abre no dispositivo-alvo;
- hash do artefato diverge do receipt sem lineage;
- bootstrap não corresponde ao commit/manifest declarado;
- `pkg/apt/dpkg` resolve apenas wrapper/stub sem backend real;
- instalação de pacote falha de forma não explicada;
- clang produz binário que não executa no alvo;
- rollback não pode restaurar o estado anterior;
- runtime observado não corresponde à ABI/configuração declarada.

## 10. Próximo caminho verificável

`packages snapshot pinado → bootstrap build → APK build → install físico → pkg/apt/dpkg probe → git/clang/python probe → hashes/receipts → rollback probe → WORK-v1 candidate verdict`.

## 11. Regra epistemológica

`VISÃO ≠ CÓDIGO ≠ ARTEFATO ≠ BUILD ≠ EXECUÇÃO ≠ EVIDÊNCIA ≠ CLAIM`.

`TOKEN_VAZIO ≠ false evidence`; significa evidência necessária ainda não observada.
