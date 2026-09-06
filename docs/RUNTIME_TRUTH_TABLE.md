# RUNTIME_TRUTH_TABLE — mapa operacional RAFCODEΦ

> Revisão documental: 2026-09-06.
> Baseline inicial auditado: `b207970fc7a8630a534956cb544350cfd61ba33a`; reconciliado com `master` `3f97ef42ae9756b9f7fb4965b941b5b3048fc8d1` após PR #415.
> Regra: `SOURCE/TEST/WORKFLOW > CONTRACT > RECEIPT > DOC`. Ausência de evidência = `TOKEN_VAZIO`.

| Recurso | Estado real | Evidência | Lacuna / próximo gate verificável |
|---|---|---|---|
| identidade do repositório | SOURCE_OBSERVED | `rafaelmeloreisnovo/termux-app-rafacodephi` | referências históricas a owners antigos devem ser marcadas como históricas |
| contrato semântico de `termux-packages` | SOURCE_OBSERVED + TESTABLE | `data/contracts/termux-packages-rafcodephi-pin.v1.json` + resolver | promoção candidate→canonical só por evidência/contrato atualizado |
| canal `canonical` de `termux-packages` | MERGED_BASELINE | SHA `837afec42ecf5f9ac1bd8b00e65d143bc23a380b` | não equivale a device proof |
| canal `candidate` de `termux-packages` | CURRENT_MAIN_PIN_VALIDATION | SHA `0ffb24a5a6be58316236383a6d249544c39eb3e3`; supersede stale `1fc540b...` | CI/build/runtime precisam provar promoção |
| resolver de pin | TEST_ENFORCED/STRUCTURAL | valida repo, package, prefix, ABIs, claim e `TOKEN_VAZIO` | observar cada workflow consumidor |
| workflow `beta-build-libllvm18-unblock` | WORKFLOW_WIRED | resolve candidate, exige SHA pinado e preflight | resultado final depende do run |
| preflight de capacidade da fonte | WORKFLOW_WIRED | verifica scripts, `libllvm18`, schema/naming do manifest e `--architectures` | PASS de run específico |
| receipt `usable-beta` de sucesso | WORKFLOW_WIRED | gerador estrito roda com `if: success()` | receipt materializado |
| receipt de falha upstream | WORKFLOW_WIRED | `UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE` + listas present/missing | primeira falha continua causa primária |
| causalidade downstream | TEST/WORKFLOW POLICY | artifact ausente após produtor falhar é consequência por padrão | apontar step produtor |
| workflow `beta-build.yml` | WORKFLOW_WIRED COM PIN PRÓPRIO | SHA default `2538114ca05a7f9c0849d9a1e6bf764702f038a0` + override manual | não confundir com candidate |
| gate freestanding `rafproot-fs` | SOURCE_OBSERVED + WORKFLOW_WIRED | `bootstrap/proot_freestanding.c`, syscall bridge, builder e `freestanding-runtime-gate.yml` | execução física Android |
| ELF freestanding ARMv7/AArch64 | BUILD-PROPERTY GATE WIRED | CI compila `-ffreestanding -nostdlib -static` e rejeita `PT_INTERP`, `DT_NEEDED` e undefined externals | artifact/receipt de run específico |
| cold-start freestanding bootstrap | SOURCE_OBSERVED + CONTRACT_WIRED | `build_freestanding_real_arm_bootstrap.py` injeta `libexec/rafproot-fs` e registra SHA/receipt | bootstrap materializado + device |
| `pkg`/PRoot/Ninja/Clang/QEMU como “freestanding” | FALSE BY BOUNDARY | documentação/source distinguem control gate de package payloads | só reimplementação própria poderia mudar classe |
| freestanding device runtime | TOKEN_VAZIO | workflow fixa `device_runtime_state=TOKEN_VAZIO`, `claim_allowed=false` | instalar/executar no Android real |
| safe-core finalization | PROVADO ESTRUTURAL | contrato, validador e testes de finalização | execução no SHA final |
| APK build | PROVADO CI HISTÓRICO | Gradle e matriz ARM32/ARM64 | repetir/observar no SHA de release |
| release assinado | PROVADO ESTRUTURAL | contrato Gradle e workflow | credencial oficial e receipt APK |
| GitHub Actions refs | PROVADO ESTRUTURAL | auditor/correção versionada | CI do SHA alvo |
| loader APK | FUNCTIONAL_SECURITY_GATED ESTRUTURAL | HTTPS limitado, permissão `signature`, provider read-only/handoff | build pareado e inspeção APK |
| loader funcional | PROVADO ESTRUTURAL, TOKEN_VAZIO NO DEVICE | validador/casos adversariais | instalação/uso físicos |
| bootstrap instala | PARCIAL | installer com staging, rollback e `Context.getFilesDir()` | receipt físico filesystem |
| failsafe shell | PROVADO ESTRUTURAL | sessão failsafe independente de `bash` | first-shell físico |
| `bash` source-built ARM/ARM64 | PROVADO ESTRUTURAL | builder custom-prefix + importador ELF32/ELF64 | artifact + first-shell físico |
| wrappers cat/ls/clear/grep | PROVADO ESTRUTURAL | builder/testes | smoke físico |
| pkg help | PROVADO ESTRUTURAL | bridge/camada estrutural | smoke físico |
| payload ARM/ARM64 real | PROVADO ESTRUTURAL; MATERIALIZAÇÃO DEPENDE DO RUN | source-build + validação par/hashes/ABI/prefixo | receipt imutável + device |
| `pkg` real | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | package stack real exigida | `DEVICE_REAL_PKG_VALIDATED` |
| `pkg update` | BLOCKED/TOKEN_VAZIO NO DEVICE | repositório custom ainda bloqueado | repo publicado/assinado + smoke |
| `pkg install` | BLOCKED/TOKEN_VAZIO NO DEVICE | mesmo gate | instalação física com receipt |
| `apt` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix exigido | execução física + repo custom |
| `apt-get` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF custom-prefix exigido | execução física + repo custom |
| `dpkg` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | ELF/status database | execução física |
| `libapt` | TOKEN_VAZIO | dependency closure não promovida aqui | dynamic-link físico |
| busybox | PARCIAL | implementação/delegação estrutural | comportamento físico consistente |
| `proot` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | payload separado do gate freestanding | `proot --version` físico |
| certificados | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE | `ca-certificates` no source-build | TLS/update físico |
| DNS/network básico | TOKEN_VAZIO | configuração candidata | teste físico |
| repositório binário custom | BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED | guard fail-closed | publicar/assinar índice + `.deb` prefix-safe |
| cliente CLI `termux-api` | PROVADO ESTRUTURAL NO PAYLOAD | receiver RAFCODEPHI | APK API pareado + chamada física |
| APK `termux-api_rafcodephi` | PROVADO ESTRUTURAL | permission signature; teste exige no sharedUserId | assinatura + instalação física |
| `android:sharedUserId` no manifest principal | TEST_ENFORCED ABSENT | `tests/test_termux_api_access_contract.py` | qualquer doc contrário é `STALE` |
| wizard `bootstrap.zip` | PROVADO ESTRUTURAL | picker SAF + BLAKE3 + ABI + profile + receipt | import/install físico |
| runtime path Android-assigned | PROVADO ESTRUTURAL | `Context.getFilesDir()` | receipt físico |
| real-pkg relocation | BLOCKED/CLAIM FALSE | fronteira explícita | rebuild/ELF validation contra runtime prefix |
| RAFAELIA ZERO core/runtime | PROVADO ESTRUTURAL | RFZ1/JNI/wiring | APK + execução física |
| RAFAELIA ZERO probe | PROVADO ESTRUTURAL | probe/receipt/captura/matriz | bundles físicos |
| ARM32 physical bundle | TOKEN_VAZIO | instrumento implementado | bundle de device |
| ARM64 physical bundle | TOKEN_VAZIO | instrumento implementado | bundle de device |
| dual ARM device matrix | TOKEN_VAZIO | validador implementado | dois bundles físicos |
| runtime receipt v2 | PROVADO ESTRUTURAL | coletor/validador | `DEVICE_RECEIPT_COMPLETE` |
| sensor catalog/snapshot v2 | PROVADO ESTRUTURAL | serviço/limites/callback | permissões/OEM reais |
| RAFAELIA JNI | PROVADO ESTRUTURAL | C/JNI | benchmark físico |
| CTI | PROVADO ESTRUTURAL | scanner C | arquivos grandes/runtime se claimado |
| ZIPRAF | PROVADO ESTRUTURAL | manifesto | não prova compressão física sozinho |
| VCPU | PROVADO ESTRUTURAL | kernel determinístico | componentes de VM completa |
| APKC DEX | PROVADO ESTRUTURAL PARCIAL | fixture limitada | backend arbitrário + ART físico |
| APKC ELF | PROVADO ESTRUTURAL PARCIAL | ET_REL/ET_EXEC fixos | linker geral + runtime físico |
| browser HTTPS | FAIL_CLOSED PROVADO | downgrade plaintext bloqueado | TLS/X.509 físico |
| runtime-lock federation | STALE_OR_INCOMPLETE | lock histórico | lock do release commit |
| observable remote CI | TOKEN_VAZIO POR RUN NÃO CITADO | instrumento existe; não inventar resultado | receipt/logs do run alvo |
| production release | BLOCKED | gates funcionais e físicos separados | CI + assinatura + pkg + dual ARM + lock |

## Frases canônicas

```text
safe-core fechado != distribuição funcional liberada != plataforma de pesquisa completa
WORKFLOW_WIRED != BUILD_PROVEN != DEVICE_PROVEN
missing downstream evidence != root cause automatically
freestanding control gate != freestanding package payload
```

Auditoria documental relacionada:

- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
