# RAFCODE-Φ — contrato de warnings, símbolos, loops e `void`

## Estado

```text
contract_version = 1
source_changes    = IMPLEMENTED
host_gate         = PROVIDED
android_ndk_build = TOKEN_VAZIO
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

## 7. Otimização numérica já aplicada

Em `raf_numbase.c`:

1. a busca de índice Fibonacci deixou de recalcular a sequência desde zero em cada passo;
2. passou a usar uma única iteração linear;
3. `LLONG_MIN` passou a ser convertido sem overflow de negação assinada;
4. o teste de primalidade deixou de usar `i * i`, evitando overflow intermediário;
5. a economia de base para `n_max = 1` passou a representar ao menos um dígito;
6. ponteiro de bases é validado quando `n_bases > 0`.

A mudança principal é:

```text
antes: Σ(k=0..86) O(k)  → O(n²)
agora: uma passagem     → O(n)
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
4. valida as flags nos módulos Android;
5. testa o classificador de warnings.

## 9. Limites de prova

O contrato e os testes de host não equivalem a:

- build completo do NDK;
- APK assinado;
- teste ARM32;
- teste ARM64;
- execução em Android 10/14/15;
- medição real de tamanho antes/depois;
- prova de que todo warning histórico foi resolvido.

Portanto:

```text
source_contract = IMPLEMENTED
host_logic      = TESTABLE
android_build   = TOKEN_VAZIO
runtime_device  = TOKEN_VAZIO
```

## 10. Próxima redução segura

O próximo candidato de alto valor é `raf_ecc32` em `rafaelia_bitraf_core.c`.

A implementação atual percorre seis bits de paridade × 32 posições. Ela pode ser substituída por máscaras constantes e paridade por palavra, desde que:

1. os vetores old/new sejam equivalentes;
2. ARM32 e ARM64 sejam testados;
3. o compilador não introduza dependência de runtime indesejada;
4. o resultado seja medido no binário final.

Até essa prova, o loop permanece válido e não deve ser apagado apenas por parecer repetitivo.

---


authorial_scope: RAFCODE-Φ / Rafael Melo Reis
upstream_scope: preserved per repository license map
release_decision: HUMAN_REVIEW_REQUIRED
