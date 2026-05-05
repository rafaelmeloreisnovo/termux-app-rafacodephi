# Status Bootstrap Lowlevel Rafaelia

## Compila
- host-smoke (`make selftest`) no módulo experimental.
- arm64-freestanding (objetos `raf_main.o` e `entry_arm64.o`).

## Não compila / não implementado
- integração completa Android runtime sem libc no app principal.
- extração ZIP real (fora de escopo e proibido nesta fase).

## Corrigido
- hex inválidos substituídos por hex válidos.
- macro `RAF_LOG_WRN` adicionada.
- entry separado em `entry_arm64.S`.
- `RAF_TERMUX_PREFIX` configurável por macro.

## Experimental
- ZIPRAF apenas round-trip de formato próprio.
- sem substituir bootstrap real do Termux.

## Pode entrar na beta
- apenas como módulo isolado e opcional.

## Não pode entrar na beta
- qualquer substituição do fluxo real de `TermuxInstaller` / `bootstrap.zip`.

## Riscos
- confundir ZIPRAF com ZIP real.
- tentar usar freestanding fora da trilha experimental.

## Próximos passos
- adicionar testes de golden vectors adicionais.
- validar toolchain NDK em CI com runner Android/NDK.
