# BETA_SECURITY_CHECK

| Item | Classificação | Nota |
|---|---|---|
| Permissões Android | RISK_BETA | Manifest contém permissões amplas (ex: MANAGE_EXTERNAL_STORAGE, QUERY_ALL_PACKAGES). |
| Execução shell | OK_BETA | comportamento esperado para app terminal; requer políticas de uso claro. |
| Exposição de dados sensíveis | NEEDS_REVIEW | revisar logs e integrações plugin em teste real. |
| Logs excessivos | RISK_BETA | logging detalhado em falhas; revisar nível para release. |
| Crash com stack sensível | NEEDS_REVIEW | presença de stacktrace em logs, depende de sanitização release. |
| Comandos perigosos | RISK_BETA | ambiente shell permite operações destrutivas por design; documentar responsabilidade do usuário. |
