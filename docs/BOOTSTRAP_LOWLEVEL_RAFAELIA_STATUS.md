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
- selftest expandido com vetores golden de CRC32C/FNV-1a.

## Experimental
- ZIPRAF apenas round-trip de formato próprio.
- sem substituir bootstrap real do Termux.
- workflow NDK dedicado para objetos freestanding AArch64 com upload de artifacts.

## Pode entrar na beta
- apenas como módulo isolado e opcional.

## Não pode entrar na beta
- qualquer substituição do fluxo real de `TermuxInstaller` / `bootstrap.zip`.

## Riscos
- confundir ZIPRAF com ZIP real.
- tentar usar freestanding fora da trilha experimental.

## Próximos passos
- adicionar target freestanding sem libc para selftest binário mínimo (sem printf).
- validar execução em dispositivo ARM64 via shell de teste isolado.
