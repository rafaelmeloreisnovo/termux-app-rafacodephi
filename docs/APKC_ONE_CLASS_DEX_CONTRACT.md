# APKC — Contrato DEX 035 com uma classe

## Corpo comprovável

`apkc/fmt_dex_one_class.h` gera um arquivo DEX 035 fixo de 392 bytes:

```text
class  Lraf/apkc/Stub;
extends Ljava/lang/Object;
method public static run()V
code   return-void
```

| Estrutura | Quantidade |
|---|---:|
| strings | 4 |
| tipos | 3 |
| protótipos | 1 |
| métodos | 1 |
| classes | 1 |
| `code_item` | 1 |
| unidades de código | 1 |
| itens do mapa | 10 |

O emissor rejeita ponteiro nulo e capacidade menor que 392 bytes, não usa heap,
codifica `class_data_item` em ULEB128, alinha o código em quatro bytes e recalcula
SHA-1 e Adler-32.

## Prova independente

```bash
python3 scripts/validate_apkc_one_class_dex.py classes.dex --pretty
```

O parser verifica magic, header, hashes, tabelas, strings ordenadas, descritores,
protótipo `()V`, método, classe, superclasse, ULEB128, flags `public static`,
alinhamento, frame, opcode `return-void` (`0x000e`) e mapa ordenado.

O gate completo mantém os dois contratos:

```bash
bash scripts/test_raf_native_compile_contract.sh
# classes-empty.dex
# classes-one-class.dex
```

## Estados separados

```text
EMPTY_STRUCTURAL_DEX035      = VERIFIED_HOST
ONE_CLASS_RETURN_VOID_DEX035 = VERIFIED_HOST após gate
ARBITRARY_DEX_BACKEND        = TOKEN_VAZIO
MULTIDEX                     = TOKEN_VAZIO
JAVA_COMPILER                = TOKEN_VAZIO
KOTLIN_COMPILER              = TOKEN_VAZIO
ART_DALVIK_RUNTIME           = TOKEN_VAZIO
```

```text
FIXED_CODE_ITEM != GENERAL_DEX_BACKEND != LANGUAGE_COMPILER
HOST_VALIDATION != ART_DALVIK_RUNTIME
```

O contrato específico e suas lacunas estão em
`configs/apkc-dex-capability.json`, validados por
`scripts/validate_apkc_dex_capability.py`.

## Próxima fronteira

```text
return-void fixo
-> parâmetros e registradores
-> constantes e retorno inteiro
-> múltiplos métodos e constructor
-> branches e alvos validados
-> fields, try/catch, annotations e debug
-> pools dinâmicos limitados
-> backend de IR
-> dexdump/apkanalyzer
-> execução ART controlada
```

Cada promoção exige fixture positiva, casos negativos, parser independente e
prova correspondente ao runtime alegado.
