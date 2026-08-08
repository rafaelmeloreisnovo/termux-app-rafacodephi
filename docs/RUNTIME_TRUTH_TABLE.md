# RUNTIME_TRUTH_TABLE — mapa operacional RAFCODEΦ

> Revisão: 2026-08-08, preservando a fronteira entre runtime bridge, execução física observada e stack real de pacotes ainda não promovida.

| Recurso | Estado real | Evidência | Lacuna |
|---|---|---|---|
| safe-core finalization | PROVADO ESTRUTURAL | contrato, validador e testes de finalização | execução no SHA final |
| APK build | PROVADO CI HISTÓRICO | Gradle e matriz ARM32/ARM64 | repetir no SHA de release |
| release assinado | PROVADO ESTRUTURAL | contrato Gradle e workflow | credencial oficial e recibo do APK |
| GitHub Actions refs | PROVADO ESTRUTURAL | auditor e correção versionada | CI observável |
| loader APK | FUNCTIONAL_SECURITY_GATED ESTRUTURAL | aquisição HTTPS limitada, permissão `signature`, provider read-only e handoff para o host | build pareado e inspeção do APK no SHA atual |
| loader funcional | PROVADO ESTRUTURAL, TOKEN_VAZIO NO DEVICE | validador e 19 casos adversariais passam; o host conserva SHA-256+BLAKE3, inspeção ZIP e instalação atômica | caller não autorizado, URI grant/revogação e instalação física |
| bootstrap instala | PARCIAL | installer com staging, rollback e runtime `Context.getFilesDir()` | receipt físico do filesystem no SHA atual |
| failsafe shell | PROVADO ESTRUTURAL | sessão failsafe continua independente de `bash` | first-shell físico no SHA atual |
| `bash` source-built ARM/ARM64 | PROVADO ESTRUTURAL | builder custom-prefix + importador ELF32/ELF64 fail-closed | artefatos do workflow e first-shell físico |
| wrappers cat/ls/clear/grep | PROVADO ESTRUTURAL | builder e testes | smoke físico |
| pkg help | PROVADO ESTRUTURAL | bridge mínima | smoke físico |
| payload ARM/ARM64 real | PROVADO ESTRUTURAL, NÃO MATERIALIZADO NESTE SHA | `termux-packages` recompila para `com.termux.rafacodephi`; importador valida par ARM/AArch64, hashes, ABI e prefixo | executar workflow, anexar artefatos e instalar em device |
| `pkg` real | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | source-build exige `pkg`/`apt`/`dpkg` reais e rejeita bridge; `device pkg smoke` ainda ausente | `DEVICE_REAL_PKG_VALIDATED` permanece ausente até artefato + device smoke + receipt |
| `pkg update` | FUTURO | bloqueado até promoção real do package stack | `DEVICE_REAL_PKG_VALIDATED` + device pkg smoke + `apt update` físico |
| `pkg install` | FUTURO | bloqueado até promoção real do package stack | `DEVICE_REAL_PKG_VALIDATED` + device pkg smoke + instalação física com receipt |
| `apt` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix exigido pelo builder/importador | execução física e repositório custom publicado |
| `apt-get` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix exigido pelo builder/importador | execução física e repositório custom publicado |
| `dpkg` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix + status database exigidos | execução física |
| `libapt` | TOKEN_VAZIO | dependency closure sem promoção | dynamic-link físico |
| busybox | PARCIAL | delegação para toybox/toolbox | substituto consistente validado |
| `proot` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix exigido, prefixo legado rejeitado | `proot --version` físico |
| certificados | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | `ca-certificates` incluído no source-build | update TLS/package físico |
| DNS/network básico | TOKEN_VAZIO | configuração candidata | teste físico |
| repositório binário custom | BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED | `termux.sources` com `Enabled: no` + hook `apt update` fail-closed; upstream incompatível não é consumido | publicar/assinar índice com `.deb` compilados para o prefixo RAFCODEPHI |
| cliente CLI `termux-api` | PROVADO ESTRUTURAL NO PAYLOAD | pacote embutido; ELF aponta para `com.termux.rafacodephi.api/com.termux.api.TermuxApiReceiver`; rota de serviço stub rejeitada | APK API pareado e chamada física |
| APK `termux-api_rafcodephi` | PROVADO ESTRUTURAL | applicationId `com.termux.rafacodephi.api`, permissão `signature` sem `sharedUserId` unilateral e `termux-shared` do fork | assinatura pareada + instalação física |
| wizard `bootstrap.zip` | PROVADO ESTRUTURAL | picker SAF + BLAKE3 + ABI + profile + receipt fail-closed | import/install físico no SHA atual |
| runtime path Android-assigned | PROVADO ESTRUTURAL | `Context.getFilesDir()` propagado em installer/env/session | receipt físico no `/mnt/expand/.../files` |
| real-pkg relocation | BLOCKED | claim explicitamente false | rebuild/validação dos ELFs contra runtime prefix |
| RAFAELIA ZERO core/runtime | PROVADO ESTRUTURAL | RFZ1, JNI e wiring no startup | APK e execução física |
| RAFAELIA ZERO probe | PROVADO ESTRUTURAL | probe, receipt, captura do APK, bundle e matriz | bundles físicos |
| ARM32 physical bundle | TOKEN_VAZIO | instrumento implementado | device bundle do SHA atual |
| ARM64 physical bundle | TOKEN_VAZIO | instrumento implementado | device bundle do SHA atual |
| dual ARM device matrix | TOKEN_VAZIO | validador implementado | dois bundles por papel |
| runtime receipt v2 | PROVADO ESTRUTURAL | coletor e validador | `DEVICE_RECEIPT_COMPLETE` do release |
| sensor catalog/snapshot v2 | PROVADO ESTRUTURAL | serviço, limites e callback autenticado | permissões e OEM reais |
| RAFAELIA JNI | PROVADO ESTRUTURAL | C/JNI | benchmark físico |
| CTI | PROVADO ESTRUTURAL | scanner C | arquivos grandes |
| ZIPRAF | PROVADO ESTRUTURAL | manifesto | não é compressão física |
| VCPU | PROVADO ESTRUTURAL | kernel determinístico de estado | componentes de VM completa |
| APKC DEX | PROVADO ESTRUTURAL PARCIAL | fixture de uma classe e método | backend arbitrário e runtime ART |
| APKC ELF | PROVADO ESTRUTURAL PARCIAL | ET_REL e ET_EXEC fixos | linker geral e runtime físico |
| browser HTTPS | FAIL_CLOSED PROVADO | downgrade plaintext bloqueado | TLS funcional e X.509 |
| runtime-lock federation | STALE_OR_INCOMPLETE | lock histórico | lock no commit de release |
| observable remote CI | TOKEN_VAZIO | sem receipt final com steps e logs | `reports/ci-finalization-evidence.json` |
| production release | BLOCKED | gate funcional | CI + assinatura + pkg + dual ARM + lock |

## Frase canônica

```text
safe-core fechado != distribuição funcional liberada != plataforma de pesquisa completa
```
