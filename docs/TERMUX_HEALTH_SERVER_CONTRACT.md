# Termux RAFCODEΦ — contrato do servidor `/health`

## Estado

```yaml
server_implementation: IMPLEMENTED
unit_http_tests: IMPLEMENTED
workflow_execution: TOKEN_VAZIO
rafgittools_client: IMPLEMENTED_IN_PR_297
end_to_end_device: TOKEN_VAZIO
mutating_commands: false
```

O servidor fornece uma única observação sanitizada do runtime local:

```text
GET http://127.0.0.1:8765/health
```

Ele não é terminal remoto, shell, executor de job ou API administrativa.

## Invariante

```text
saúde observável
+
nenhuma mutação
+
nenhuma credencial
+
nenhum bind externo
```

## Execução

```sh
python3 scripts/termux_health_server.py
```

Outras formas:

```sh
# um único request e encerra
python3 scripts/termux_health_server.py --once

# somente materializa o snapshot sem abrir socket
RAF_TERMUX_COMMIT="$(git rev-parse HEAD)" \
  python3 scripts/termux_health_server.py --print-snapshot

# porta local alternativa
python3 scripts/termux_health_server.py --port 9876
```

Hosts aceitos:

- `127.0.0.1`;
- `localhost`, normalizado para IPv4 loopback;
- `::1`, com socket IPv6.

`0.0.0.0`, IP da LAN e portas privilegiadas são rejeitados.

## Resposta

```json
{
  "abi": "armv7l",
  "capabilities": [
    "health.readonly",
    "job.submit.readonly",
    "artifact.inspect",
    "rafpolimata.status"
  ],
  "commit": "TOKEN_VAZIO",
  "pid": 1234,
  "runtime": "termux-rafcodephi",
  "schema": "raf.termux-health.v1",
  "status": "ok",
  "uptime_ms": 42000
}
```

A chave `commit` só aceita hexadecimal de 7–64 caracteres via `RAF_TERMUX_COMMIT`. Qualquer outro valor vira `TOKEN_VAZIO`.

O snapshot nunca enumera:

- ambiente completo;
- `HOME` ou `PREFIX`;
- caminhos;
- tokens;
- senhas;
- conteúdo de arquivos;
- processos de terceiros;
- comandos executados.

## Métodos e caminhos

| Entrada | Resultado |
|---|---|
| `GET /health` | `200` + snapshot |
| `GET /v1/health` | `200` + snapshot |
| `HEAD` nos caminhos acima | `200`, sem corpo |
| outro caminho/query | `404 path_not_allowed` |
| `POST`, `PUT`, `PATCH`, `DELETE` | `405 read_only_endpoint` |

Cabeçalhos:

```text
Cache-Control: no-store
X-Content-Type-Options: nosniff
Connection: close
```

O handler não registra path/query em log, reduzindo risco de persistir material indevido.

## Relação com RafGitTools

```text
RafGitTools PR #297
  GovernanceGate
  → TermuxHealthProbe
  → localhost HTTP
  → este servidor
```

Estados ponta a ponta:

| Condição | Cliente |
|---|---|
| servidor responde 2xx | `PASS` |
| servidor responde 4xx/5xx | `FAIL` |
| socket não responde | `TOKEN_VAZIO` |
| endpoint externo/inválido | `ERROR` |

Assim:

```text
servidor ausente != Termux quebrado
```

## Testes

`tests/test_termux_health_server.py` cobre:

- snapshot determinístico com relógio/PID injetáveis;
- sanitização de ABI e commit;
- não vazamento de ambiente;
- bloqueio de bind não loopback;
- bloqueio de porta privilegiada;
- GET real em porta efêmera;
- 404 para query/path não permitido;
- 405 para mutação;
- HEAD sem corpo;
- encerramento limpo da thread de teste.

## Fronteira de segurança

A capacidade declarada `job.submit.readonly` é apenas um identificador de capacidade futura. Este servidor não implementa submissão de job nesta PR.

```yaml
HEALTH_READ: IMPLEMENTED
JOB_SUBMIT_READONLY: DECLARED_ONLY
ARTIFACT_INSPECT: DECLARED_ONLY
RAFPOLIMATA_STATUS: DECLARED_ONLY
REMOTE_BIND: PROHIBITED
MUTATION: PROHIBITED
```

## Próximos gates

```yaml
H0_PYTHON_TESTS: TOKEN_VAZIO_UNTIL_RUN
H1_RAFGITTOOLS_CLIENT_TESTS: TOKEN_VAZIO_UNTIL_RUN
H2_LOCAL_END_TO_END: TOKEN_VAZIO
H3_TERMUX_ARM32: TOKEN_VAZIO
H4_TERMUX_ARM64: TOKEN_VAZIO
H5_AUTO_START_POLICY: TOKEN_VAZIO
H6_LATENCY_P50_P95_P99: TOKEN_VAZIO
```

Autostart não deve ser adicionado antes de decidir consumo de bateria, lifecycle, lockfile, porta ocupada e política de desligamento.

## Retroalimentação

```text
F_ok   = servidor local read-only, schema sanitizado, métodos bloqueados e testes implementados
F_gap  = execução nos devices e lifecycle/autostart
F_next = validar PRs cliente+servidor localmente e medir integração ARM32/ARM64
```
