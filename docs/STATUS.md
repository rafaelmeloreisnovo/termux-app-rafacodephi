# STATUS — fonte de verdade de build, runtime e conclusão

> Última revisão: 2026-07-20  
> Contrato de conclusão: `configs/system-finalization-contract.json`  
> Validador: `tools/validate_system_finalization.py`

## Regra principal

```text
implementado != compilado != executado != provado no Android != liberado para produção
```

A palavra **finalizado** deve sempre indicar um perfil:

```text
SAFE_CORE_IMPLEMENTATION_CLOSED
FUNCTIONAL_DISTRIBUTION_RELEASE_CLOSED
FULL_RESEARCH_PLATFORM_CLOSED
```

## Estado atual

| Perfil | Estado | Release |
|---|---|---|
| `safe-core` | candidato a `SAFE_CORE_IMPLEMENTATION_CLOSED`, com gate executável | `false` |
| `functional-distribution` | `BLOCKED` | `false` |
| `full-platform` | `BLOCKED` / pesquisa aberta | `false` |

O perfil `safe-core` verifica metadados Android/NDK/ABI, referências de Actions, quarentena do loader, instrumentação RAFAELIA ZERO e presença das fontes canônicas. Ele não exige nem finge prova física.

## Evolução consolidada em 2026-07-20

- PR #288: catálogo e snapshot governado dos sensores Android;
- PR #289: `checkout@v7` corrigido para `checkout@v6` em 33 workflows e política de referências adicionada;
- PR #291: core RAFAELIA ZERO RFZ1 integrado ao APK por JNI/DirectByteBuffer;
- PR #292: probe físico protegido no build debug;
- PR #293: receipt v2 selado para Android/Termux;
- PR #296: captura do APK instalado, bundle atômico e matriz ARM32/ARM64;
- PR #297: loader parcial inseguro bloqueado; stub inerte preservado por quarentena.

## Verdade Android/NDK

- `compileSdkVersion=35`;
- `targetSdkVersion=28`;
- `minSdkVersion=21`;
- `ndkVersion=26.3.11579264`;
- ABIs obrigatórias: `armeabi-v7a`, `arm64-v8a`;
- APK universal habilitado;
- package: `com.termux.rafacodephi`.

## Núcleo que pode ser encerrado

Estado esperado após o gate `safe-core`:

```text
build_metadata=PROVEN_STRUCTURAL
github_action_references=PROVEN_STRUCTURAL
loader_quarantine=FUNCTIONAL_SECURITY_GATED
rafaelia_zero_instrumentation=PROVEN_STRUCTURAL
canonical_truth_sources=PROVEN_STRUCTURAL
state=SAFE_CORE_IMPLEMENTATION_CLOSED
claim_allowed_scope=true
release_allowed=false
```

Comando:

```bash
python3 tools/validate_system_finalization.py \
  --profile safe-core \
  --strict \
  --write-report
```

O mesmo gate é chamado por `scripts/verify_rafaelia_native_safety.py`.

## Bloqueadores da distribuição funcional

A distribuição não pode ser liberada enquanto qualquer item abaixo permanecer aberto:

- CI remoto final sem etapas/logs observáveis;
- assinatura de produção sem recibo de artefato;
- bundle físico ARM32 ausente;
- bundle físico ARM64 ausente;
- matriz dual ARM ausente;
- execução física do payload source-built `pkg/apt/apt-get/dpkg/proot` ainda ausente;
- repositório binário RAFCODEΦ ainda não publicado/assinado;
- certificados, DNS e repositório sem teste no aparelho;
- `runtime-lock.json` histórico/incompleto.

Estado correto:

```text
FUNCTIONAL_DISTRIBUTION_RELEASE_CLOSED=false
release_allowed=false
```

## Loader

O loader atual passa estruturalmente como:

```text
state=FUNCTIONAL_SECURITY_GATED
android:hasCode=true
loader_extraction=false
host_custody=SHA256+BLAKE3+BOUNDED_ZIP+ATOMIC_INSTALL
release_allowed=false
```

A promoção continua bloqueada até haver build pareado, certificados iguais e
provas independentes no Android para caller não autorizado, grant/revogação de
URI e instalação/rollback.

## RAFAELIA ZERO e prova física

Implementado estruturalmente:

- core RFZ1 freestanding;
- integração JNI/DirectByteBuffer;
- inicialização no app;
- probe debug protegido;
- receipt atômico;
- captura do `base.apk` instalado;
- manifesto, transcript e hashes;
- bundle atômico;
- seleção por papel;
- matriz ARM32/ARM64 e anti-replay.

Ainda ausente:

```text
arm32-legacy=TOKEN_VAZIO
arm64-modern=TOKEN_VAZIO
dual_arm_matrix=TOKEN_VAZIO
DEVICE_RECEIPT_COMPLETE=TOKEN_VAZIO
```

## Pesquisa que não bloqueia o safe-core

Fronteiras sem overclaim: ZIPRAF não comprime fisicamente; ele mantém um
índice lógico sobre bytes existentes. A VCPU atual é uma **RAFAELIA deterministic VCPU state kernel**,
não uma máquina virtual completa.

- TLS próprio e certificação;
- compiladores APKC completos;
- linker geral e backends arbitrários;
- VCPU promovida a VM completa;
- loader funcional opcional.

Esses objetivos pertencem ao perfil `full-platform`, não ao encerramento do núcleo seguro.

## Fontes canônicas

1. `gradle.properties`;
2. `docs/RUNTIME_TRUTH_TABLE.md`;
3. `docs/FINAL_BUILD_CLOSURE.md`;
4. `configs/system-finalization-contract.json`;
5. `tools/validate_system_finalization.py`;
6. `configs/loader-functional-security-contract.json`;
7. `reports/RAFAELIA_ZERO_OPERATIONAL_EVIDENCE_BASELINE_20260720.json`;
8. `runtime-lock.json`.

## Retroalimentação operacional

```text
F_ok   = núcleo integrado + gates fail-closed + instrumentos físicos
F_gap  = package stack prefix-safe + dual ARM + signing + CI observável + lock
F_next = executar #295 nos dois aparelhos e reconstruir a matriz física
```
