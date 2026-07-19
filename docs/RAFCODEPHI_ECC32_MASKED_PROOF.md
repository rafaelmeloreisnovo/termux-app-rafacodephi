# RAFCODE-Φ — prova da transformação ECC32 mascarada

## Estado

```text
source_integration = IMPLEMENTED
gf2_basis_proof    = PASS
million_state_test = PASS
compact_policy     = PASS
unrolled_policy    = PASS
native_safety      = PASS
android_arm32      = RUNNING
android_ndk29      = RUNNING
device_runtime     = TOKEN_VAZIO
claim_allowed      = false
```

## 1. Implementação de referência

A versão anterior calculava seis bits ECC. Para cada saída `b`, percorria as 32 posições da palavra:

```text
for b = 0..5
    for i = 0..31
        position = i + 1
        if position contém o bit b
            ecc[b] ^= value[i]
```

Estrutura máxima por palavra:

```text
6 × 32 = 192 testes de posição
```

O algoritmo era determinístico e correto, mas repetia em runtime uma relação de posições constante em compile-time.

## 2. Forma algébrica

Considere `v_i` o bit `i` da palavra de 32 bits, com `i ∈ [0,31]`.

Para cada saída `b ∈ [0,5]`:

```text
e_b(v) = XOR de v_i para todo i onde ((i + 1) & (1 << b)) != 0
```

Definindo uma máscara constante:

```text
M_b[i] = 1  se ((i + 1) & (1 << b)) != 0
       = 0  caso contrário
```

segue:

```text
e_b(v) = parity(v & M_b)
```

As máscaras derivadas são:

| Saída | Máscara |
|---:|---:|
| 0 | `0x55555555` |
| 1 | `0x66666666` |
| 2 | `0x78787878` |
| 3 | `0x7F807F80` |
| 4 | `0x7FFF8000` |
| 5 | `0x80000000` |

Portanto:

```text
ECC32(v) =
    parity(v & M0)       |
    parity(v & M1) << 1  |
    parity(v & M2) << 2  |
    parity(v & M3) << 3  |
    parity(v & M4) << 4  |
    parity(v & M5) << 5
```

## 3. Prova sobre GF(2)

A implementação de referência é linear:

```text
reference(x XOR y) = reference(x) XOR reference(y)
```

A implementação mascarada também é linear, pois AND por máscara, paridade e XOR são operações lineares sobre `GF(2)`.

Todo vetor `v ∈ GF(2)^32` pode ser escrito como combinação XOR dos 32 vetores da base canônica:

```text
v = XOR de e_i para cada bit ativo i
```

Logo, se:

```text
reference(e_i) = masked(e_i)
```

para todos os 32 vetores-base, então:

```text
reference(v) = masked(v)
```

para todas as `2^32` palavras possíveis.

O teste `tests/native/test_raf_ecc32_masked.c` verifica os 32 vetores-base. Essa etapa constitui a prova finita completa da transformação linear.

## 4. Guarda de implementação

Além da base canônica, cada política executa:

```text
1.000.000 estados determinísticos
```

produzidos pelo LCG:

```text
state[n+1] = state[n] × 1664525 + 1013904223 mod 2^32
```

Para cada estado:

```text
reference(state) == masked(state)
```

O milhão de estados não é necessário para completar a prova algébrica, mas protege contra constantes incorretas, regressões no folding e divergências entre os caminhos de compilação.

## 5. Política dirigida pelo precompilador

O primeiro corte totalmente desenrolado removeu todos os loops, mas a inspeção estrutural mostrou que isso pode aumentar bytes quando o compilador opera com `-Os`.

O contrato final possui dois caminhos semanticamente idênticos.

### 5.1 Compacto

Ativado por:

```text
-Os
-Oz
RAF_ECC32_FORCE_COMPACT=1
```

Executa seis máscaras fixas:

```text
for b = 0..5
    ecc[b] = parity(v & M_b)
```

Assim, o loop interno de 32 posições desaparece:

```text
antes = 6 × 32 iterações
agora = 6 iterações
```

### 5.2 Desenrolado

Ativado por:

```text
RAF_ECC32_FORCE_UNROLL=1
```

Expande as seis paridades diretamente, favorecendo o perfil de velocidade e permitindo agendamento independente pelo compilador.

### 5.3 Invariante

```text
compact(v) = unrolled(v) = reference(v)
```

Definir simultaneamente `RAF_ECC32_FORCE_COMPACT` e `RAF_ECC32_FORCE_UNROLL` produz erro de compilação.

## 6. Runtime integrado

Arquivo canônico:

```text
rafaelia/src/main/cpp/raf_ecc32_masked.h
```

Integração no núcleo:

```c
static u8 raf_ecc32(u32 v) {
    return (u8)raf_ecc32_masked((unsigned int)v);
}
```

A implementação de referência não permanece no runtime. Ela existe somente no teste como oráculo independente.

## 7. Folding de paridade

A paridade de cada máscara é reduzida sem loop:

```text
v ^= v >> 16
v ^= v >> 8
v ^= v >> 4
parity = (0x6996 >> (v & 0xF)) & 1
```

Não há dependência de libc, heap, `libm` ou builtin específico de arquitetura.

O header fixa o contrato de palavra:

```c
_Static_assert(sizeof(unsigned int) == 4u, ...);
```

Isso impede compilar silenciosamente o transformador onde `unsigned int` não possua 32 bits.

## 8. Gate

Comando canônico:

```sh
bash scripts/test_raf_native_compile_contract.sh
```

O gate:

1. compila a política compacta com `-Os` e `RAF_ECC32_FORCE_COMPACT=1`;
2. compila a política desenrolada com `-O2` e `RAF_ECC32_FORCE_UNROLL=1`;
3. executa a prova de base e um milhão de estados em cada binário;
4. confirma que `rafaelia_bitraf_core.c` usa o novo header;
5. proíbe o retorno do antigo loop `6 × 32`;
6. permanece integrado ao workflow `Rafaelia Native Safety`.

## 9. Medição e limites

A equivalência funcional de `raf_ecc32` está fechada no domínio de 32 bits. Ainda exigem medição no artefato Android:

- ciclos exatos em ARMv7;
- ciclos exatos em ARM64;
- bytes finais no `.so` após inlining e linker GC;
- comportamento térmico prolongado;
- execução em dispositivo físico.

Medições exploratórias de objeto cruzado indicaram que a política compacta é mais adequada ao `-Os`, enquanto o desenrolamento é mais apropriado a perfis de velocidade. Esses números não substituem o `.so` real produzido pelo NDK.

---

```text
authorial_scope = RAFCODE-Φ / Rafael Melo Reis
proof_scope      = ECC32 functional equivalence + compile policy
release_decision = HUMAN_REVIEW_REQUIRED
```
