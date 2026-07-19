# RAFCODE-Φ — contrato de warnings, símbolos, loops e `void`

## Estado

```text
contract_version = 2
source_changes    = IMPLEMENTED
host_gate         = PASS
native_safety     = PASS
android_ndk_build = RUNNING
apk_runtime       = TOKEN_VAZIO
claim_allowed     = false
```

## 1. Invariante

Warnings do compilador são **diagnósticos de intenção e estrutura**. Eles não são, sozinhos, comandos de remoção.

A eliminação segura de código ocorre pela composição:

```text
diagnóstico
→ classificação
→ correção ou anotação explícita
→ função/dado em seção própria
→ análise de alcançabilidade do linker
→ --gc-sections
→ teste
```

Formalmente:

```text
DEAD_CODE_REMOVAL =
    INTERNAL_OR_HIDDEN_LINKAGE
  ∧ FUNCTION_DATA_SECTIONS
  ∧ LINKER_GC
  ∧ NO_LIVE_REFERENCE
  ∧ VERIFICATION
```

Nenhum warning autoriza apagar fonte automaticamente.

## 2. Flags canônicas

Para módulos nativos sujeitos à coleta de símbolos:

```text
-std=c11
-Wall
-Wextra
-Werror
-fno-common
-ffunction-sections
-fdata-sections
-Wl,--gc-sections
```

No módulo RAFAELIA, candidatos `unused` permanecem visíveis, mas temporariamente não quebram o build:

```text
-Wno-error=unused-function
-Wno-error=missing-field-initializers
```

Isso é diferente de esconder o diagnóstico com `-Wno-unused-function`.

## 3. Contrato de intenção C

O arquivo:

```text
rafaelia/src/main/cpp/raf_compile_contract.h
```

centraliza:

| Macro | Intenção |
|---|---|
| `RAF_UNUSED` | símbolo opcional conscientemente preservado |
| `RAF_USED` | símbolo que não pode ser removido pelo compilador |
| `RAF_NORETURN` | função terminal |
| `RAF_DISCARD(x)` | resultado/parâmetro ignorado de modo explícito |
| `RAF_SPIN_FOREVER()` | loop terminal intencional com barreira de compilador |
| `RAF_UNREACHABLE()` | caminho logicamente impossível já provado |
| `RAF_PURE` / `RAF_CONST` | ausência declarada de efeitos laterais compatível com otimização |
| `RAF_EXPORT` | símbolo deliberadamente exposto apesar da visibilidade oculta |

`RAF_UNUSED` não deve ser usado como silenciador global. O símbolo precisa ser opcional por contrato.

## 4. `void` e valores ignorados

Em JNI, callbacks e ABIs estáveis, parâmetros podem existir por contrato mesmo sem uso local.

Forma canônica:

```c
RAF_DISCARD(env);
RAF_DISCARD(clazz);
```

ou, em código legado equivalente:

```c
(void)env;
(void)clazz;
```

Essa conversão não cria loop nem trabalho operacional relevante; ela documenta que o valor foi conscientemente descartado.

Resultados com `warn_unused_result` não devem ser descartados automaticamente. O classificador marca `-Wunused-result` como bloqueador até que o retorno seja tratado ou a perda seja justificada.

## 5. Loops

Há três classes distintas.

### 5.1 Loop produtivo

Executa transformação necessária e permanece no código.

Exemplos:

- serialização;
- CRC/ECC;
- iteração de matriz;
- processamento de buffer.

A otimização deve reduzir complexidade ou permitir vetorização sem alterar a semântica.

### 5.2 Loop morto ou inalcançável

Não contribui para saída observável ou está após término comprovado.

Ação:

```text
reparar fluxo
OU remover após auditoria de referências
OU permitir que --gc-sections retire a função inteira inalcançável
```

### 5.3 Loop terminal intencional

Usado por panic/bare-metal quando não existe retorno seguro.

Forma canônica:

```c
static RAF_NORETURN RAF_COLD void panic_terminal(void) {
    RAF_SPIN_FOREVER();
}
```

Não deve ser confundido com loop desnecessário.

## 6. Classificador de warnings

Uso:

```sh
clang ... 2>&1 \
  | python3 scripts/raf_compile_warning_contract.py --pretty
```

O relatório usa o schema:

```text
raf.compile-warning-contract.v1
```

Categorias principais:

| Diagnóstico | Categoria | Ação |
|---|---|---|
| `unused-function/variable` | `GC_CANDIDATE` | verificar linkage e referências; GC ou remoção auditada |
| `unused-parameter` | `INTENTIONAL_VOID_OR_API_FIX` | `RAF_DISCARD` em ABI ou remoção em função privada |
| `unused-result` | `RESULT_MUST_BE_HANDLED` | bloqueia release |
| `unreachable-code` | `UNREACHABLE_CFG` | corrigir CFG ou provar terminalidade |
| `empty-body` | `LOOP_OR_BRANCH_REVIEW` | distinguir espera terminal de estrutura morta |
| erro de declaração/retorno | `CONTROL_FLOW_OR_ABI_FAILURE` | corrigir antes de otimizar |

O campo abaixo é invariável:

```json
"automatic_source_deletion": false
```

## 7. Otimizações já aplicadas

### 7.1 Núcleo numérico

Em `raf_numbase.c`:

1. a busca de índice Fibonacci deixou de recalcular a sequência desde zero em cada passo;
2. passou a usar uma única iteração linear;
3. `LLONG_MIN` passou a ser convertido sem overflow de negação assinada;
4. o teste de primalidade deixou de usar `i * i`, evitando overflow intermediário;
5. a economia de base para `n_max = 1` passou a representar ao menos um dígito;
6. ponteiro de bases é validado quando `n_bases > 0`.

```text
antes: Σ(k=0..86) O(k)  → O(n²)
agora: uma passagem     → O(n)
```

### 7.2 ECC32 mascarado

A implementação antiga em `rafaelia_bitraf_core.c` realizava:

```text
6 × 32 = 192 iterações posicionais
```

A relação foi convertida em seis máscaras constantes:

```text
55555555
66666666
78787878
7F807F80
7FFF8000
80000000
```

O runtime chama:

```c
raf_ecc32_masked(v)
```

O precompilador escolhe a política:

```text
RAF_ECC32_PROFILE=compact  → seis passos mascarados; padrão do módulo -Os
RAF_ECC32_PROFILE=speed    → seis paridades totalmente desenroladas
```

Os dois caminhos foram verificados contra a referência por:

- todos os 32 vetores da base canônica de `GF(2)^32`;
- 1.000.000 de estados determinísticos por política;
- compilação com `-Werror`;
- verificador estático que proíbe o retorno do loop antigo `6 × 32`.

A prova detalhada está em:

```text
docs/RAFCODEPHI_ECC32_MASKED_PROOF.md
```

## 8. Gate local

Comando único:

```sh
bash scripts/test_raf_native_compile_contract.sh
```

O gate:

1. compila o núcleo numérico com warnings estritos;
2. ativa seções individuais e GC do linker;
3. executa invariantes nativas;
4. compila e executa ECC32 em política compacta;
5. compila e executa ECC32 em política desenrolada;
6. valida as flags e perfis nos módulos Android;
7. testa o classificador de warnings.

O comando é chamado pelo workflow canônico `Rafaelia Native Safety`, sem novo YAML.

## 9. Limites de prova

Os testes de host e a prova algébrica não equivalem a:

- APK assinado para distribuição;
- medição de ciclos no Moto E7 Power;
- medição de ciclos em ARM64 físico;
- comportamento térmico prolongado;
- medição final de bytes no `.so` após todas as decisões de inline;
- prova de que todo warning histórico foi resolvido.

Portanto:

```text
source_contract = IMPLEMENTED
host_logic      = PASS
native_safety   = PASS
android_build   = RUNNING
runtime_device  = TOKEN_VAZIO
```

## 10. Próxima redução segura

O próximo candidato de alto valor é `raf_crc16` em `rafaelia_bitraf_core.c`.

A implementação atual percorre cada byte e executa oito passos de polinômio. Antes de qualquer substituição, devem existir:

1. equivalência old/new para vetores conhecidos e corpus determinístico;
2. separação explícita entre perfil compacto e perfil de velocidade;
3. medição de bytes no objeto e no `.so` ARM32/ARM64;
4. garantia de ausência de tabela grande indesejada no perfil `-Os`;
5. manutenção do polinômio e do valor inicial como contrato versionado.

Até essa prova, o loop CRC permanece produtivo e não pode ser apagado apenas por aparecer como repetição.

---

```text
authorial_scope: RAFCODE-Φ / Rafael Melo Reis
upstream_scope: preserved per repository license map
release_decision: HUMAN_REVIEW_REQUIRED
```
