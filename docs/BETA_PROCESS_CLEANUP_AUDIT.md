# BETA_PROCESS_CLEANUP_AUDIT

| Arquivo | Função | Risco | Evidência | Correção | Status |
|---|---|---|---|---|---|
| `app/src/main/java/com/termux/app/TermuxService.java` | lifecycle + kill sessões/processos | médio | `onDestroy()` chama `killAllTermuxExecutionCommands()` quando necessário | sem mudança; requer teste em dispositivo | RISK_BETA |
| `app/src/main/java/com/termux/app/TermuxActivity.java` | bind/unbind service | baixo | binding controlado e estados de visibilidade | sem mudança | OK_BETA |
| `app/src/main/java/com/termux/app/terminal/TermuxTerminalSessionServiceClient.java` | associação PID sessão | baixo | grava PID em `ExecutionCommand` | sem mudança | OK_BETA |
| `app/src/main/cpp/termux-bootstrap.c` | bootstrap nativo | baixo | sem criação de subprocesso persistente | sem mudança | OK_BETA |

**Conclusão**: sem blocker confirmado por código estático, mas falta validação dinâmica de zumbis em aparelho real.
