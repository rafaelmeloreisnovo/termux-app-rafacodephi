# RAFCODE-Φ — prova da transformação ECC32 mascarada

## Estado

```text
source_integration = IMPLEMENTED
gf2_basis_proof    = PASS
million_state_test = PASS
host_compile       = PASS
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

O loop era determinístico e correto, mas repetia em runtime uma relação de posições que é constante em compile-time.

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

Além da base canônica, o gate executa:

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

Esse milhão de estados não é necessário para completar a prova algébrica, mas protege contra erros de codificação, constantes digitadas incorretamente e regressões no folding de paridade.

## 5. Runtime integrado

Arquivo canônico da transformação:

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

## 6. Paridade sem loop

A paridade de cada máscara é reduzida por folding:

```text
v ^= v >> 16
v ^= v >> 8
v ^= v >> 4
parity = (0x6996 >> (v & 0xF)) & 1
```

Não há dependência de libc, heap, `libm` ou builtin específico de uma arquitetura.

O header também fixa o contrato:

```c
_Static_assert(sizeof(unsigned int) == 4u, ...);
```

Isso impede compilar silenciosamente o transformador em uma plataforma onde `unsigned int` não possua 32 bits.

## 7. Gate

Comando canônico:

```sh
bash scripts/test_raf_native_compile_contract.sh
```

O gate:

1. compila a transformação com `-Wall -Wextra -Werror`;
2. executa a prova de base e o milhão de estados;
3. confirma que `rafaelia_bitraf_core.c` usa o novo header;
4. proíbe o retorno dos dois loops antigos;
5. permanece integrado ao workflow `Rafaelia Native Safety`.

## 8. Limites

A equivalência funcional de `raf_ecc32` está fechada no domínio de 32 bits. Ainda não estão demonstrados neste documento:

- ganho exato de ciclos em ARMv7;
- ganho exato de ciclos em ARM64;
- redução final de bytes no `.so` após todas as decisões de inline;
- comportamento térmico em execução prolongada;
- execução em dispositivo físico.

Esses valores dependem do compilador, ABI, inlining e microarquitetura. Devem ser medidos, não inferidos.

---

```text
authorial_scope = RAFCODE-Φ / Rafael Melo Reis
proof_scope      = ECC32 functional equivalence
release_decision = HUMAN_REVIEW_REQUIRED
```
