# Termux RAFCODEΦ — contrato do servidor `/health`

## Estado

```yaml
server_implementation: IMPLEMENTED
unit_http_tests: IMPLEMENTED
local_isolated_smoke: PASS
local_client_server_loopback: PASS
workflow_execution: TOKEN_VAZIO_STARTUP
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
    "health.readonly"
  ],
  "commit": "TOKEN_VAZIO",
  "pid": 1234,
  "runtime": "termux-rafcodephi",
  "schema": "raf.termux-health.v1",
  "status": "ok",
  "uptime_ms": 42000
}
```

Somente capacidades com handler e teste podem ser anunciadas. `job.submit.readonly`, `artifact.inspect` e `rafpolimata.status` permanecem fora do snapshot até implementação real.

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

A execução independente também fechou a ponte local:

```text
PASS termux-health-end-to-end-local code=200 state=PASS bytes=176
```

Esse resultado é host/local. Não substitui execução Termux Android ARM32/ARM64.

## Fronteira de segurança

```yaml
HEALTH_READ: IMPLEMENTED
JOB_SUBMIT_READONLY: NOT_ADVERTISED_TOKEN_VAZIO
ARTIFACT_INSPECT: NOT_ADVERTISED_TOKEN_VAZIO
RAFPOLIMATA_STATUS: NOT_ADVERTISED_TOKEN_VAZIO
REMOTE_BIND: PROHIBITED
MUTATION: PROHIBITED
```

## Próximos gates

```yaml
H0_LOCAL_PYTHON_SMOKE: PASS
H1_LOCAL_RAFGITTOOLS_CLIENT_COMPILE: PASS
H2_LOCAL_END_TO_END: PASS
H3_GITHUB_ACTIONS: TOKEN_VAZIO_STARTUP
H4_TERMUX_ARM32: TOKEN_VAZIO
H5_TERMUX_ARM64: TOKEN_VAZIO
H6_AUTO_START_POLICY: TOKEN_VAZIO
H7_LATENCY_P50_P95_P99: TOKEN_VAZIO
```

Autostart não deve ser adicionado antes de decidir consumo de bateria, lifecycle, lockfile, porta ocupada e política de desligamento.

## Retroalimentação

```text
F_ok   = servidor local read-only, cliente Kotlin e loopback ponta a ponta executados
F_gap  = execução nos devices e lifecycle/autostart
F_next = validar em Termux ARM32/ARM64 e medir p50/p95/p99
```
