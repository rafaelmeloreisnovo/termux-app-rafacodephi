# Termux RAFCODE-Φ — Provider Contract for Vectras IPC v3

**Base Termux:** `508efb3d01594cc20d51b7fb01d5f2a790169bf2`  
**Base Vectras:** `a29392e65948463ab9cb6dbfefe64eb060e23a07`  
**Estado:** `STATIC_CONTRACT_IMPLEMENTED / EXECUTION_PENDING`  
**Claim:** `claim_allowed=false`

## Finalidade

Este documento congela o lado provedor do protocolo Vectras → Termux → QEMU. Ele não altera o comportamento do `RunCommandService`; registra e verifica o comportamento já presente no commit fixado.

## Identidade do provedor

```text
package default = com.termux.rafacodephi
service = com.termux.app.RunCommandService
action = com.termux.rafacodephi.RUN_COMMAND
permission = com.termux.rafacodephi.permission.RUN_COMMAND
protection level = dangerous
```

O serviço é exportado porque precisa aceitar pedidos de outra aplicação, mas é protegido pela permissão dedicada.

## Entradas públicas

```text
com.termux.rafacodephi.RUN_COMMAND_PATH
com.termux.rafacodephi.RUN_COMMAND_ARGUMENTS
com.termux.rafacodephi.RUN_COMMAND_WORKDIR
com.termux.rafacodephi.RUN_COMMAND_RUNNER
com.termux.rafacodephi.RUN_COMMAND_PENDING_INTENT
```

O runner canônico do C07 é `app-shell`. A compatibilidade histórica `RUN_COMMAND_BACKGROUND` permanece no Termux, mas o consumidor v3 não depende dela.

## Resultado

O `PendingIntent` recebe um bundle `result` com:

```text
stdout
stdout_original_length
stderr
stderr_original_length
exitCode
err
errmsg
```

`exitCode` é o código do processo executado. `err` e `errmsg` representam erro interno do serviço/plugin. Eles não podem ser fundidos no mesmo campo.

Os comprimentos originais permitem detectar truncamento do stdout/stderr. Um consumidor que ignore esses campos não pode afirmar que recebeu a saída completa.

## Canonicalização de paths

O serviço canonicaliza executable path e workdir por meio das utilidades Termux. O provedor não recebe uma string de shell concatenada: command path e array de argumentos são campos separados.

## Manifesto legível por máquina

`docs/contracts/VECTRAS_TERMUX_PROVIDER_V3.json` registra pacote, serviço, permission, action, request keys, result keys, runners e limites epistemológicos.

## Verificador

```sh
python3 scripts/verify_vectras_termux_provider_v3.py \
  --output artifacts/c07/termux-provider-v3.json
```

Ele cruza:

- `app/build.gradle`;
- AndroidManifest;
- `TermuxConstants.java`;
- `ExecutionCommand.java`;
- `RunCommandService.java`;
- `TermuxPluginUtils.java`;
- `ResultSender.java`;
- manifesto JSON do provedor.

O gate falha se package, permission, action, runner ou result keys mudarem, se PendingIntent deixar de ser suportado, se metadados de truncamento desaparecerem ou se erro interno for confundido com exit code.

## Fronteira epistemológica

Um `PASS_STATIC_PROVIDER_CONTRACT` prova apenas que o commit possui a superfície esperada. Não prova:

```yaml
android_build: TOKEN_VAZIO
provider_installed: TOKEN_VAZIO
permission_granted: TOKEN_VAZIO
real_result_bundle: TOKEN_VAZIO
qemu_execution: TOKEN_VAZIO
guest_boot: TOKEN_VAZIO
claim_allowed: false
```

## Gate físico

O C07 exige, posteriormente:

1. APK Termux identificado por hash;
2. instalação observada;
3. permission grant observado;
4. request real recebido;
5. processo executado;
6. bundle enviado;
7. receipt Vectras reconciliado ao request hash.

Guest boot e validação da VM pertencem ao C08.