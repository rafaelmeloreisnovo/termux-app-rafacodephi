# Termux RAFCODE-Φ — Vectras IPC v2

## Estado

```yaml
discovery_protocol_v2: IMPLEMENTED
nonce_binding: IMPLEMENTED
private_path_disclosure: REMOVED
execution_transport: RUN_COMMAND_SERVICE
permission: com.termux.rafacodephi.permission.RUN_COMMAND
device_execution: TOKEN_VAZIO
claim_allowed: false
```

## Invariante

```text
descoberta sanitizada
→ capacidade e nome de binário
→ permissão concedida pelo usuário
→ RunCommandService
→ recibo
```

O receiver não entrega `$PREFIX`, `HOME` ou caminhos privados. Ele informa:

- nonce de transação validado e ecoado;
- versão do protocolo;
- bootstrap disponível;
- nomes dos binários QEMU executáveis;
- versão do Termux;
- modo de execução;
- nome da permissão;
- `private_paths_exposed=false`.

O `RunCommandService` continua condicionado a:

1. permissão `RUN_COMMAND`;
2. `allow-external-apps=true` escolhido pelo usuário;
3. caminho limitado ao `$PREFIX`, `$HOME` ou `$OPT`;
4. limites e auditoria do serviço.

## Gate local

```bash
python3 scripts/validate_vectras_ipc_v2.py
```

Resultado local limitado:

```yaml
static_validator: PASS
kotlin_syntax_with_android_stubs: PASS
android_build: TOKEN_VAZIO
device_permission_grant: TOKEN_VAZIO
```

## R3

```text
F_ok   = caminhos privados removidos, nonce e transporte explícito declarado
F_gap  = grant em aparelho, dispatch, retorno e exit code
F_next = executar o primeiro dispatch ARM e materializar recibo
```
