# RAFAELIA Low-Level / ASM Index

## Escopo
Inventário factual de arquivos `.s/.S`, inline ASM e fontes low-level com classificação Pontos S.

## Arquivos ASM/inline/low-level

### 1) app/src/main/cpp/termux-bootstrap-zip.S
- build oficial: SIM (`libtermux-bootstrap`, Android.mk)
- ABI alvo: multi-ABI (via NDK build do app)
- instruções/funções: bootstrap unzip helpers em ASM
- cache/L1/L2: sem modelagem explícita
- Hz/tick/phase/TTL: ausente
- branchless: não declarado
- CRC/seal: não declarado
- benchmark: sem integração direta
- Pontos S: S1=OFICIAL/IMPLEMENTADO; S3=SYNCED; S12=sem claim não comprovada

### 2) app/src/main/cpp/lowlevel/baremetal_asm.S
- build oficial: SIM (`termux-baremetal`, arm64-v8a e armeabi-v7a)
- ABI alvo: ARM32/ARM64 (NEON quando habilitado)
- instruções/funções: rotinas ASM para path baremetal
- cache/L1/L2: relação indireta via otimização hot path
- Hz/tick/phase/TTL: não é scheduler primário
- branchless: parcial (redução de branches em path SIMD)
- CRC/seal: suporte indireto por pipeline baremetal
- benchmark: usado em suíte low-level/top42
- Pontos S: S1=OFICIAL/IMPLEMENTADO; S3=CODE_AHEAD_DOC parcial

### 3) mvp/rafaelia_mvp_puro.s
- build oficial: NÃO
- módulo: MVP
- ABI: ARM syntax didática
- cache/L1/L2: claim de alinhamento L1 em comentários
- Hz/tick/phase/TTL: conceitual
- branchless: não comprovado
- CRC/seal: não comprovado
- benchmark: sem artifact oficial
- Pontos S: S1=MVP/EXPERIMENTAL; S3=CLAIM_WITHOUT_ARTIFACT

### 4) rmr/Rrr/rafaelia_b1.S
- build oficial: NÃO
- módulo: SOLTO_Rrr
- ABI: ARM
- instruções/funções: pipeline toroidal, CRC32C SW
- cache/L1/L2: sem integração oficial
- Hz/tick/phase/TTL: phase/cycle explícitos
- branchless: misto (tem branches)
- CRC/seal: sim, CRC32C
- benchmark: sem CI artifact oficial
- Pontos S: S1=SOLTO/Rrr/PARCIAL

### 5) rmr/src/main/cpp/rmr.c (inline asm)
- build oficial: SIM (módulo rmr)
- inline asm: `__asm__ __volatile__("rev ...")`
- ABI alvo: condicionais por arquitetura
- cache/Hz/TTL: não aplicável diretamente
- branchless: `rev` é instrucional, não pipeline branchless global
- CRC/seal: não direto
- benchmark: sem medição dedicada
- Pontos S: S1=OFICIAL/IMPLEMENTADO; S3=SYNCED

## Observações obrigatórias
- TTL literal: não encontrado no núcleo oficial low-level; classificado como conceito não formalizado.
- BitStack/ZipRAF/BitOmega: BitStack aparece em Rrr; ZipRAF e BitOmega sem implementação oficial comprovada.
