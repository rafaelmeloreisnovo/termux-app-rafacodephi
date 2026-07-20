# RAFCODEPHI_FINAL_ADMIN_LEDGER

## Estado

`FATO_DOCUMENTADO`: ledger administrativo para fechar a documentação do Termux RAFCODEphi em relação ao Vectra, ao lowlevel, ao BitRAF42, à janela de ruído e à recuperação por liberação parcial de bits.

Este documento não substitui teste. Ele organiza o que já está codificado, o que está documentado e o que ainda precisa ser provado em build/runtime.

---

## Frase canônica

```text
Código é verdade codificada.
Documento é mapa.
Teste é fechamento.
TOKEN_VAZIO protege o que ainda não foi provado.
```

---

## Depósitos administrados

| Depósito | Papel |
|---|---|
| `rafaelmeloreisnovo/termux-app-rafacodephi` | APK/Termux/RAFCODEphi, lowlevel C/H/ASM/JNI, build Android |
| `rafaelmeloreisnovo/Vectras-VM-Android` | Vectra/RMR, contratos, BitRAF/BitOmega/BITWALK, docs active e validação conceitual |

---

## O que já está codificado no Termux RAFCODEphi

| Bloco | Evidência esperada no repo | Status |
|---|---|---|
| App RAFCODEphi | `app/build.gradle` com package/nome RAFCODEphi | `FATO_CODE` |
| Build nativo | `app/src/main/cpp/Android.mk` | `FATO_CODE` |
| Baremetal | `app/src/main/cpp/lowlevel/baremetal.c/.h` | `FATO_CODE` |
| No malloc | `baremetal_nomalloc.c` | `FATO_CODE` |
| ASM/NEON | `baremetal_asm.S` | `FATO_CODE` |
| JNI direto | `rafaelia_jni_direct.c` | `FATO_CODE` |
| Buffers únicos | `IN_BUF`, `OUT_BUF`, `STATE_BUF` | `FATO_CODE` |
| VCPU | `raf_vcpu.c/.h` | `FATO_CODE` |
| Clock/Hz | `raf_clock.c/.h` | `FATO_CODE` |
| Memory layers | `raf_memory_layers.c/.h` | `FATO_CODE` |
| BitRAF | `raf_bitraf.c/.h` | `FATO_CODE` |
| Commit gate | `rafaelia_commit_gate_ll.c/.h` | `FATO_CODE` |
| RAFAELIA ZERO | core RFZ1, JNI, probe, receipt, bundle e matriz | `FATO_CODE` |
| Loader quarantine | contrato e validador fail-closed | `FATO_TESTADO_LOCAL` |
| Finalization gate | contrato, validador e testes por perfil | `FATO_CODE` |

---

## Pontos de documentação final

| Documento | Finalidade | Estado |
|---|---|---|
| `BITRAF42_BASE60_GUARD_BAND_AND_RECOVERY.md` | Canonizar 42 bits, base60, 60..63 e recuperação | P0 aberto |
| `BASE20_BASE60_ADDRESSING.md` | Explicar `60 = 3 x 20` e blocos A/B/C | P0 aberto |
| `EMPTY_SPACE_NEGATIVE_MEMORY.md` | Explicar ponto vazio/não gravado como valor estrutural | P0 aberto |
| `CLOCK_TTL_HZ_PROTOCOL.md` | Unificar clock, Hz, TTL, jitter e vida útil | P1 aberto |
| `VOID_NIL_WARNING_PIPELINE.md` | Unificar void, nil, warning, guard e TOKEN_VAZIO | P1 aberto |
| `NEON_16_LANE_EXECUTION.md` | Explicar 16 lanes físicas e variantes 8/16/32/64 bits | P1 aberto |
| `SME40_RECOVERY_TEST_PLAN.md` | Testar 40 e poucos % de liberação sem afirmar perda | P0 aberto |
| `FINAL_BUILD_CLOSURE.md` | Separar safe-core, distribuição funcional e plataforma completa | ENTREGUE |

O fechamento administrativo do build agora possui autoridade executável:

```text
configs/system-finalization-contract.json
tools/validate_system_finalization.py
tests/test_system_finalization.py
docs/FINAL_BUILD_CLOSURE.md
```

---

## Regra sobre os 40 e poucos por cento

Não escrever:

```text
perdeu 40% => perdeu o dado
```

Escrever:

```text
houve liberação/ausência parcial de 40 e poucos por cento;
os bits podem ser recuperáveis se a rota, paridade, hash, CRC, camada ou assinatura determinística preservarem coerência.
```

Status correto:

```text
FATO_OPERACIONAL: a arquitetura trata ausência como estado/rota, não como perda automática.
FATO_CODE_PARCIAL: existem buffers separados, BitRAF, CRC/hash, guard-band e rotas.
F_NEXT_TEST: provar o limiar de recuperação por teste automatizado.
```

---

## Teste administrativo de recuperação

Nome sugerido:

```text
scripts/test_sme40_recovery.sh
```

Saídas obrigatórias:

```text
recovery_00.json
recovery_10.json
recovery_20.json
recovery_30.json
recovery_40.json
recovery_45.json
```

Campos mínimos por JSON:

```json
{
  "release_percent": 40,
  "payload_hash_original": "...",
  "payload_hash_recovered": "...",
  "crc_original": "...",
  "crc_recovered": "...",
  "route_preserved": true,
  "payload_recovered": false,
  "status": "ROUTE_ONLY | RECOVERED | TOKEN_VAZIO | FAILED"
}
```

---

## Regra para documentação final

Cada claim deve ter esta forma:

```text
claim:
arquivo:
função:
evidência:
teste:
status:
próxima ação:
```

Estados aceitos:

| Estado | Significado |
|---|---|
| `FATO_CODE` | existe no código |
| `FATO_TESTADO` | existe teste passando |
| `FATO_TESTADO_LOCAL` | teste delimitado executado, sem promoção remota/física |
| `DOC_ATRASADA` | código existe, documento ainda insuficiente |
| `INCUBADORA` | existe fora do caminho canônico |
| `TOKEN_VAZIO` | lacuna marcada, sem inferência |
| `F_NEXT` | próxima ação |

---

## Fechamento por perfil

```text
safe-core:
  documentação e gate final presentes
  release_allowed=false

functional-distribution:
  bloqueada por package stack prefix-safe, dual ARM, signing, CI e runtime lock

full-platform:
  pesquisa aberta em TLS, compiladores completos e VM completa
```

O projeto não precisa terminar todos os experimentos para encerrar o núcleo seguro. Também não pode promover o núcleo seguro a release sem as provas físicas e operacionais.

Frase final:

```text
Libertar bit não é perder bit; perder só é fato depois que a rota de recuperação falha.
Fechar o núcleo não é fingir que a distribuição inteira já foi provada.
```
