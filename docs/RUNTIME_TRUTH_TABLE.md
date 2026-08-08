# RUNTIME_TRUTH_TABLE — mapa operacional RAFCODEΦ

> Revisão: 2026-08-08, preservando a fronteira entre runtime bridge, execução física observada e stack real de pacotes ainda não promovida.

| Recurso | Estado real | Evidência | Lacuna |
|---|---|---|---|
| safe-core finalization | PROVADO ESTRUTURAL | contrato, validador e testes de finalização | execução no SHA final |
| APK build | PROVADO CI HISTÓRICO | Gradle e matriz ARM32/ARM64 | repetir no SHA de release |
| release assinado | PROVADO ESTRUTURAL | contrato Gradle e workflow | credencial oficial e recibo do APK |
| GitHub Actions refs | PROVADO ESTRUTURAL | auditor e correção versionada | CI observável |
| loader APK | STUB_SAFE_BLOCKED | contrato de quarentena e casos locais | funcionalidade integral ou permanência como stub |
| loader funcional | TOKEN_VAZIO | estado parcial é bloqueado | fronteira host-loader e prova Android |
| bootstrap instala | PARCIAL | installer com staging, rollback e runtime `Context.getFilesDir()` | receipt físico do filesystem no SHA atual |
| sh | PARCIAL | bootstrap e wrapper | first-shell físico no SHA atual |
| wrappers cat/ls/clear/grep | PROVADO ESTRUTURAL | builder e testes | smoke físico |
| pkg help | PROVADO ESTRUTURAL | bridge mínima | smoke físico |
| payload ARM real | BLOQUEADO | `LEGACY_PREFIX_BINARY_RISK` | rebuild para o prefixo/runtime realmente usado |
| `pkg` real | TOKEN_VAZIO | candidato não promovido; `device pkg smoke` ainda ausente | `DEVICE_REAL_PKG_VALIDATED` permanece ausente até stack prefix-safe + device pkg smoke + receipt |
| `apt` | TOKEN_VAZIO | candidato bloqueado | rebuild e teste físico |
| `apt-get` | TOKEN_VAZIO | candidato bloqueado | rebuild e teste físico |
| `dpkg` | TOKEN_VAZIO | candidato bloqueado | rebuild e teste físico |
| `libapt` | TOKEN_VAZIO | dependency closure sem promoção | dynamic-link físico |
| busybox | PARCIAL | delegação para toybox/toolbox | substituto consistente validado |
| `proot` | TOKEN_VAZIO | candidato com prefixo legado | rebuild e `proot --version` |
| certificados | TOKEN_VAZIO | candidato presente | update TLS/package físico |
| DNS/network básico | TOKEN_VAZIO | configuração candidata | teste físico |
| repositório configurado | TOKEN_VAZIO | configuração candidata | `apt update` real |
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
