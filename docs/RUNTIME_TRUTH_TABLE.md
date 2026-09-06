# RUNTIME_TRUTH_TABLE — mapa operacional RAFCODEΦ

> Revisão documental: 2026-09-06.
> Baseline auditado: `b207970fc7a8630a534956cb544350cfd61ba33a`.
> Regra: `SOURCE/TEST/WORKFLOW > CONTRACT > RECEIPT > DOC`. Ausência de evidência = `TOKEN_VAZIO`.

| Recurso | Estado real | Evidência | Lacuna / próximo gate verificável |
|---|---|---|---|
| identidade do repositório | SOURCE_OBSERVED | `rafaelmeloreisnovo/termux-app-rafacodephi` no GitHub baseline | referências históricas a owners antigos devem ser marcadas como históricas |
| contrato semântico de `termux-packages` | SOURCE_OBSERVED + TESTABLE | `data/contracts/termux-packages-rafcodephi-pin.v1.json` + resolver | promoção candidate→canonical só por evidência/contrato atualizado |
| canal `canonical` de `termux-packages` | MERGED_BASELINE | SHA `837afec42ecf5f9ac1bd8b00e65d143bc23a380b` no contrato | não equivale a device proof |
| canal `candidate` de `termux-packages` | CURRENT_MAIN_PIN_VALIDATION | SHA `0ffb24a5a6be58316236383a6d249544c39eb3e3`; supersede stale `1fc540b...` | CI/build/runtime precisam provar a promoção |
| resolver de pin | TEST_ENFORCED/STRUCTURAL | valida repo, package, prefix, ABIs, claim e `TOKEN_VAZIO` | observação de todos os workflows consumidores |
| workflow `beta-build-libllvm18-unblock` | WORKFLOW_WIRED | resolve `candidate`, exige SHA pinado e preflight de capacidade | resultado final do run/artefatos não é inferido deste documento |
| preflight de capacidade da fonte | WORKFLOW_WIRED | verifica scripts, `libllvm18`, manifest schema/naming e `--architectures` antes do build caro | PASS de run específico |
| receipt de sucesso `usable-beta` | WORKFLOW_WIRED | gerador estrito roda com `if: success()` | receipt materializado de run bem-sucedido |
| receipt de falha upstream | WORKFLOW_WIRED | `UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE` + `present_evidence`/`missing_evidence` | primeira falha concreta do run continua sendo causa primária |
| causalidade de erro downstream | TEST/WORKFLOW POLICY | manifest ausente após source-build falhar é consequência, não causa-raiz automaticamente | apontar step produtor que falhou |
| workflow `beta-build.yml` | WORKFLOW_WIRED COM PIN PRÓPRIO | SHA default próprio `2538114ca05a7f9c0849d9a1e6bf764702f038a0` + override manual | não confundir com canal `candidate` do outro workflow |
| safe-core finalization | PROVADO ESTRUTURAL | contrato, validador e testes de finalização | execução no SHA final |
| APK build | PROVADO CI HISTÓRICO | Gradle e matriz ARM32/ARM64 | repetir/observar no SHA de release |
| release assinado | PROVADO ESTRUTURAL | contrato Gradle e workflow | credencial oficial e receipt do APK |
| GitHub Actions refs | PROVADO ESTRUTURAL | auditor e correção versionada | CI observável do SHA alvo |
| loader APK | FUNCTIONAL_SECURITY_GATED ESTRUTURAL | aquisição HTTPS limitada, permissão `signature`, provider read-only e handoff para host | build pareado e inspeção do APK no SHA atual |
| loader funcional | PROVADO ESTRUTURAL, TOKEN_VAZIO NO DEVICE | validador/casos adversariais | instalação e uso físicos |
| bootstrap instala | PARCIAL | installer com staging, rollback e runtime `Context.getFilesDir()` | receipt físico do filesystem no SHA atual |
| failsafe shell | PROVADO ESTRUTURAL | sessão failsafe independente de `bash` | first-shell físico no SHA atual |
| `bash` source-built ARM/ARM64 | PROVADO ESTRUTURAL | builder custom-prefix + importador ELF32/ELF64 fail-closed | artefato de workflow + first-shell físico |
| wrappers cat/ls/clear/grep | PROVADO ESTRUTURAL | builder e testes | smoke físico |
| pkg help | PROVADO ESTRUTURAL | bridge/camada estrutural | smoke físico |
| payload ARM/ARM64 real | PROVADO ESTRUTURAL; MATERIALIZAÇÃO DEPENDE DO RUN | source-build recompila para RAFCODEPHI; importador valida par/hashes/ABI/prefixo | receipt imutável do workflow + instalação em device |
| `pkg` real | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | source-build exige package stack real; bridge não deve ser promovida | `DEVICE_REAL_PKG_VALIDATED` |
| `pkg update` | BLOCKED/TOKEN_VAZIO NO DEVICE | repositório custom ainda bloqueado na documentação normativa | repositório publicado/assinado + smoke físico |
| `pkg install` | BLOCKED/TOKEN_VAZIO NO DEVICE | mesmo gate do repositório binário | instalação física com receipt |
| `apt` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix exigido | execução física + repo custom |
| `apt-get` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix exigido | execução física + repo custom |
| `dpkg` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix/status database | execução física |
| `libapt` | TOKEN_VAZIO | dependency closure não promovida neste documento | dynamic-link físico |
| busybox | PARCIAL | implementação/delegação estrutural | comportamento consistente físico |
| `proot` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix exigido, legado rejeitado | `proot --version` físico |
| certificados | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | `ca-certificates` no source-build | TLS/update físico |
| DNS/network básico | TOKEN_VAZIO | configuração candidata | teste físico |
| repositório binário custom | BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED | guard fail-closed documentado | publicar/assinar índice + `.deb` prefix-safe |
| cliente CLI `termux-api` | PROVADO ESTRUTURAL NO PAYLOAD | rota `com.termux.rafacodephi.api/com.termux.api.TermuxApiReceiver` | APK API pareado + chamada física |
| APK `termux-api_rafcodephi` | PROVADO ESTRUTURAL | permission `signature`; teste exige ausência de `android:sharedUserId` no manifest principal | assinatura pareada + instalação física |
| `android:sharedUserId` no manifest principal | TEST_ENFORCED ABSENT | `tests/test_termux_api_access_contract.py` | qualquer doc que afirme presença atual é `STALE` |
| wizard `bootstrap.zip` | PROVADO ESTRUTURAL | picker SAF + BLAKE3 + ABI + profile + receipt fail-closed | import/install físico |
| runtime path Android-assigned | PROVADO ESTRUTURAL | `Context.getFilesDir()` propagado | receipt físico em storage atribuído pelo Android |
| real-pkg relocation | BLOCKED/CLAIM FALSE | fronteira explícita | rebuild/validação ELF contra runtime prefix |
| RAFAELIA ZERO core/runtime | PROVADO ESTRUTURAL | RFZ1/JNI/wiring | APK e execução física |
| RAFAELIA ZERO probe | PROVADO ESTRUTURAL | probe/receipt/captura/matriz | bundles físicos |
| ARM32 physical bundle | TOKEN_VAZIO | instrumento implementado | bundle de device do SHA atual |
| ARM64 physical bundle | TOKEN_VAZIO | instrumento implementado | bundle de device do SHA atual |
| dual ARM device matrix | TOKEN_VAZIO | validador implementado | dois bundles físicos por papel |
| runtime receipt v2 | PROVADO ESTRUTURAL | coletor e validador | `DEVICE_RECEIPT_COMPLETE` do release |
| sensor catalog/snapshot v2 | PROVADO ESTRUTURAL | serviço/limites/callback | permissões/OEM reais |
| RAFAELIA JNI | PROVADO ESTRUTURAL | C/JNI | benchmark físico |
| CTI | PROVADO ESTRUTURAL | scanner C | arquivos grandes + runtime se claimado |
| ZIPRAF | PROVADO ESTRUTURAL | manifesto | não é prova de compressão física por si só |
| VCPU | PROVADO ESTRUTURAL | kernel determinístico de estado | componentes de VM completa |
| APKC DEX | PROVADO ESTRUTURAL PARCIAL | fixture limitada | backend arbitrário + ART físico |
| APKC ELF | PROVADO ESTRUTURAL PARCIAL | ET_REL/ET_EXEC fixos | linker geral + runtime físico |
| browser HTTPS | FAIL_CLOSED PROVADO | downgrade plaintext bloqueado | TLS funcional/X.509 físico |
| runtime-lock federation | STALE_OR_INCOMPLETE | lock histórico | lock do commit de release |
| observable remote CI | TOKEN_VAZIO POR RUN NÃO CITADO | instrumento existe; este documento não inventa resultado remoto | receipt final/steps/logs do run alvo |
| production release | BLOCKED | gate funcional e physical evidence separados | CI + assinatura + pkg + dual ARM + lock |

## Frases canônicas

```text
safe-core fechado != distribuição funcional liberada != plataforma de pesquisa completa
WORKFLOW_WIRED != BUILD_PROVEN != DEVICE_PROVEN
missing downstream evidence != root cause automatically
```

Auditoria documental relacionada:

- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
