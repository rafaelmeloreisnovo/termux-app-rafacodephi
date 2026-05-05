# BETA_TERMINAL_LIFECYCLE_AUDIT

| Item | Classificação | Evidência resumida |
|---|---|---|
| criação de sessão | OK_BETA | `TermuxActivity` + `TermuxService` inicializam sessão via clientes de terminal. |
| inicialização do shell | OK_BETA | `TermuxService` usa `TermuxShellManager`/`TermuxSession`. |
| abertura PTY | OK_BETA | fluxo `TerminalSession`/`TermuxSession` padrão Termux. |
| stdout/stderr | OK_BETA | callbacks `TerminalSessionClient` ativos. |
| stdin | OK_BETA | terminal view envia input para sessão ativa. |
| resize | OK_BETA | integração `TerminalView` padrão Termux. |
| encerramento sessão | OK_BETA | `actionStopService()` e kill explícito. |
| kill e cleanup | RISK_BETA | depende de execução completa de `onDestroy()` + kill all commands. |
| exit code | OK_BETA | PID/sessão rastreados no `TermuxTerminalSessionServiceClient`. |
| exceções/log | OK_BETA | `Logger.logStackTraceWithMessage` presente. |
| rotação/lifecycle | NEEDS_MANUAL_TEST | requer validação em device real. |
| background | NEEDS_MANUAL_TEST | serviço foreground protege processos, mas precisa teste real Android 12+. |
| fechamento app | RISK_BETA | limpeza declarada, validar com stress test e inspeção de processos. |
