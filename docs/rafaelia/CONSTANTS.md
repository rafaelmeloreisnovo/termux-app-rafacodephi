# CONSTANTS.md — Q16.16, Fundamentação e Relação com o Modelo T^7

Este documento consolida as constantes críticas usadas no ecossistema RAFAELIA (simulação/low-level/documentação), com foco em representação fixa Q16.16, motivação matemática e leitura físico-computacional.

## 1) Convenção numérica

- Formato: **Q16.16** (16 bits inteiros + 16 bits fracionários).
- Conversão real → Q16.16: `q = round(real * 65536)`.
- Conversão Q16.16 → real: `real = q / 65536`.

## 2) Constantes primárias

### 2.1 `alpha = 0.25`

- Uso: taxa de atualização exponencial em coerência e entropia:
  - `C_{t+1} = (1-alpha)C_t + alpha*C_in`
  - `H_{t+1} = (1-alpha)H_t + alpha*H_in`
- Valor Q16.16: `0.25 * 65536 = 16384` (`0x00004000`).
- Justificativa: compromisso entre memória de estado (75%) e adaptação (25%).

### 2.2 `sqrt(3)/2 ≈ 0.8660254038`

- Uso: decaimento/rotação discreta em recorrência e espirais:
  - `r_n = (sqrt(3)/2)^n`
  - `F_{n+1} = F_n*(sqrt(3)/2) - pi*sin(279°)`
- Valor Q16.16: `round(0.8660254038*65536)=56756` (`0x0000DDD4`).
- Justificativa: projeção de simetria hexagonal (60°), comum em acoplamento angular discreto.

### 2.3 `phi = (1+sqrt(5))/2 ≈ 1.6180339887`

- Uso: ponderação estrutural/harmônica e escalonamento multi-camada.
- Valor Q16.16: `round(1.6180339887*65536)=106039` (`0x00019E37`).
- Justificativa: razão áurea como fator de não-comensurabilidade para reduzir sincronizações degeneradas.

### 2.4 `pi ≈ 3.1415926536`

- Uso: termo de deslocamento na dinâmica recorrente com seno angular.
- Valor Q16.16: `round(pi*65536)=205887` (`0x0003243F`).
- Justificativa: normalização de fase contínua em mapeamento toroidal.

### 2.5 `279°`

- Uso: termo de excitação/offset em `sin(279°)`.
- Conversão para radianos: `279 * pi/180 ≈ 4.8694686131`.
- Valor Q16.16 (graus): `279*65536 = 18284544`.
- Valor Q16.16 (radianos): `round(4.8694686131*65536)=319126`.
- Justificativa: ângulo não trivial (fora de eixos canônicos), útil para quebrar simetrias e evitar ciclos curtos artificiais.

### 2.6 `42` (cardinalidade/período alvo)

- Uso: janela de pipeline, atrator discreto e codificação BitRAF de 42 bits.
- Valor inteiro/Q16.16: `42` / `42*65536 = 2752512`.
- Justificativa: período operacional fixo para sincronizar recorrência, commit gate e auditoria de estado.

### 2.7 `Pi_max ≈ 0.9`

- Uso: limite de integridade para seleção de estado válido.
- Valor Q16.16: `round(0.9*65536)=58982` (`0x0000E666`).
- Justificativa: limiar alto para aceitar apenas estados de alta coerência/integridade.

## 3) Constantes auxiliares de entropia e geometria

### 3.1 `256`

- Uso em estimativa `H ≈ U/256 + T/N` e normalização de cardinalidade de byte.
- Justificativa: alfabeto completo de 8 bits.

### 3.2 `6000` e `2000`

- Uso:
  - `entropy_milli = unique*6000/256 + transitions*2000/(len-1)`
- Justificativa: ponderação empírica em milipontos para balancear diversidade (`unique`) e dinâmica (`transitions`).

### 3.3 `0x100000001B3`

- Uso: multiplicador FNV-1a (64-bit) em hash incremental.
- Justificativa: boa difusão com custo computacional baixo em pipeline C/ASM.

## 4) Observações de consistência

1. Sempre explicitar se a trigonometria está em graus ou radianos antes de quantizar para Q16.16.
2. Em comparações de ciclo (`x_{n+42}=x_n`), considerar tolerância de quantização quando houver seno/cosseno em ponto fixo.
3. Constantes de física (ex.: `epsilon_0`) devem ser isoladas por módulo para evitar mistura de unidades com escalas adimensionais de coerência.

## 5) Mapa rápido (real → Q16.16)

| Constante | Real | Q16.16 (decimal) | Hex |
|---|---:|---:|---:|
| alpha | 0.25 | 16384 | 0x00004000 |
| sqrt(3)/2 | 0.8660254038 | 56756 | 0x0000DDD4 |
| phi (áureo) | 1.6180339887 | 106039 | 0x00019E37 |
| pi | 3.1415926536 | 205887 | 0x0003243F |
| Pi_max | 0.9 | 58982 | 0x0000E666 |
| 42 | 42.0 | 2752512 | 0x002A0000 |

## 6) Leitura arquitetural

As constantes acima carregam o "conhecimento operacional" do sistema: elas unem memória dinâmica (alpha), geometria de fase (pi, 279°, sqrt(3)/2), estrutura de robustez (42 ciclos) e critérios de integridade (Pi_max), formando um espaço de estados toroidal `T^7` com recorrência controlada e auditável.
