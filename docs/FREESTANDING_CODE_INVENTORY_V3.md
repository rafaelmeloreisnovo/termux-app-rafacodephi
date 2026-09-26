# Freestanding Code Atlas — Inventário e Fronteira V3

> Successor documental de `docs/FREESTANDING_PURE_CORE_INVENTORY_V2.md`.  
> Base observada: `master@8004dfd5c112061579d54c6bf5a5afdd514ddfdf`.  
> Git tree: `39f42151fbcc0277380bdba4b333713686c40e2c`, leitura recursiva completa (`truncated=false`).  
> Regra: `SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`.

## 1. Inventário exato da base

| medida | observado |
|---|---:|
| C/C++/ASM fontes | 357 |
| blobs de fonte únicos | 314 |
| grupos de duplicação exata | 29 |
| cópias excedentes por hash | 43 |
| entradas totais da árvore Git | 2868 |

Distribuição principal: `rafaelia=79`, `BugOrAdd=63`, `rmr=53`,
`app=49`, `Arme=36`, `src=18`, `bootstrap_rafaelia=15`,
`tests=13`, `tools=8`, `apkc=7`, `bootstrap=6`.

Os 43 excedentes não são removidos nesta rodada. Eles são DNA histórico/staging
até a autoridade semântica de cada família estar ligada por hash e successor.

## 2. Fronteiras que evitam recompilar a lógica para cada arquitetura

| camada | contrato |
|---|---|
| `PURE_CORE` | C/ASM determinístico; memória/valores do caller; **zero libc, heap, syscall, JNI, I/O, headers hosted e decisão de plataforma** |
| `FREESTANDING_ADAPTER / PLATFORM_GATE` | `_start`, ELF, ABI do SO e syscalls explícitas; sem libc/heap; arquitetura isolada em módulo pequeno |
| `HOSTED_BOUNDARY` | JNI/Android/POSIX/dlopen/Java/Kotlin; traduz o mundo externo para a ABI estável do core |
| `TOOLING_TEST` | preprocessor, compiler, assembler, linker, Python/shell/CI; prova propriedades, não entra no core |
| `STAGING_HISTORY` | rascunhos/cópias/experimentos; não governa produção enquanto não for promovido por contrato |

A frase operacional é:

```text
PURE_CORE(no syscall)
  -> ABI fixa
  -> PLATFORM_GATE(syscall apenas se o ambiente exigir)
  -> HOSTED_BOUNDARY
```

No Linux/Android um executável que lê/escreve/sai precisa, em algum ponto, de
um serviço do ambiente. O ganho arquitetural vem de **não contaminar o core**
com isso: a syscall vira uma folha substituível, não uma dependência semântica
do algoritmo.

## 3. Núcleo observado que já se sustenta

Autoridade atual: `rafaelia/src/main/cpp/zero/`.

Folha forte:
`rafaelia/src/main/cpp/zero/include/rafz_pure_primitives.h`.

Ela não contém headers externos, heap, I/O, syscall, JNI, `if/for/while/switch/goto/?:`.
A sonda existente `tests/pure_core_probe.c` já liga essa folha a probes ARMv7
e AArch64.

O V2 registrou CI específico PASS no commit `38e02d288b24a48bbbc0fe0909af541442f05bd0`;
isso continua **evidência histórica do escopo da sonda**, não prova automática
do `master` futuro nem de aparelho físico.

## 4. “Sem branch” é propriedade por escala

Separar:

1. **source-control-free**: não há sintaxe de decisão/iteração na folha;
2. **assembly-control-free**: o objeto gerado não contém transferências de
   controle proibidas;
3. **whole-binary-control-free**: propriedade do ELF inteiro.

Não promover 1 -> 2 -> 3 sem gate próprio. Retorno terminal de função
(`ret` / `bx lr`) não é “decisão de algoritmo”; é fronteira ABI.

A preferência “sem if/while/for” é aplicável a **folhas de tamanho conhecido**.
Parsing, I/O e comprimentos arbitrários não devem ser falsamente rotulados
branchless; use especialização por tamanho, geração em compile-time,
unroll explícito, tabelas, predicação/seleção e módulos por ISA onde isso
preserva semântica e tamanho.

## 5. Contradições de alta prioridade

### C-FS-001 — Q16 staging

`rmr/Rrr/q16_fixed.h` declara `CONFORM_NO_LOOP_IMPLICIT` e
`CONFORM_BRANCHLESS`, mas contém `if`, `for`, `?:` e headers hosted.

Além da classificação contraditória, três expressões precisam de vetores antes
de promoção: saturação de `q16_sub`, carry de `q16_avg` e escala
`q16_to_byte`.

Estado:
`TOKEN_VAZIO_CONTRADICTION + TOKEN_VAZIO_CONFORMANCE_UNPROVEN`.

Ação segura: **não corrigir a cópia staging como se fosse autoridade**.
Extrair somente operações demonstráveis para a árvore `rafaelia/.../zero`,
com testes e assembly probe por função.

### C-FS-002 — “BLAKE3 wrapper” placeholder

`src/main/jni/blake3_wrapper.h` se nomeia BLAKE3, porém o próprio source
declara compressão simplificada, sem message schedule completo, usando XOR
mínimo. Busca de código na base observada não mostrou consumidor de
`blake3_hash` fora do próprio header/documentação.

Estado:
`TOKEN_VAZIO_CONFORMANCE_UNPROVEN`.

Regra: **não usar esse arquivo como evidência de BLAKE3 nem integridade
criptográfica**. O produtor BLAKE3 autoritativo deve ser ligado por KAT/vetores
oficiais antes de qualquer substituição.

## 6. Redundância como mapa, não lixo

Maior grupo exato observado:

```text
Arme/Add/baremetal_nomalloc.h
BugOrAdd/baremetal_nomalloc.h
app/src/main/cpp/lowlevel/baremetal_nomalloc.h
rmr/Rrr/baremetal_nomalloc.h
```

Os três diretórios `Arme/Add`, `BugOrAdd`, `rmr/Rrr` concentram vários
blobs repetidos. O próximo passo não é apagar: é criar relação
`canonical -> aliases-by-hash -> consumers -> successor`. Só depois uma
deduplicação pode ser demonstravelmente sem perda.

## 7. Rota de extração

```text
A. inventariar função + consumidores + hash
B. marcar autoridade/staging
C. definir tamanho/ABI/semântica fixa
D. extrair folha PURE_CORE
E. compilar -ffreestanding -nostdlib -nostdinc -fno-builtin
F. inspecionar objeto/assembly
G. linkar ELF somente quando necessário
H. auditar PT_INTERP/DT_NEEDED/undef/reloc/symbols/sections
I. executar host/QEMU como evidência própria
J. executar hardware físico como gate separado
K. receipt + successor + novos TOKEN_VAZIO
```

Ordem de menor risco:
`scalar Q16 -> BITRAF fixed-width -> CRC fixed-block/unrolled -> geometria Q16
fixed-size -> memory adapters -> I/O/platform gates`.

## 8. Flags e composição

Base mínima já coerente com o V2:

```text
-std=c11 -O2 -ffreestanding -nostdlib -nostdinc -fno-builtin
-fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables
-ffunction-sections -fdata-sections
```

`--gc-sections` pode remover código inalcançável; não prova equivalência.
Preprocessor/ISA selection deve escolher folhas especializadas em compile-time.
Comentários podem carregar tags de contrato/gate para tooling, mas **comentário
não altera semântica do compilador** sem um gerador/validador explícito.

## R3

**F_ok**: inventário atual exato, autoridade pure-core localizada, fronteira
syscall-free/core vs platform gate explícita, 43 excedentes preservados por
custódia, duas contradições relevantes tipadas.

**F_gap**: whole-binary control-free, KAT BLAKE3 para qualquer substituição,
vetores corretos Q16, deduplicação semântica dos aliases, execução física
ARM32/ARM64.

**F_next**: validar o novo dicionário; então extrair Q16 fixed-size correto
para o produtor `rafaelia/.../zero` com vetores + assembly probe, sem tocar
a cópia staging como autoridade.
