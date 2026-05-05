# BETA Bootstrap Baremetal Guard

## Objetivo
O guard baremetal valida o prefix já instalado pelo fluxo oficial do `TermuxInstaller`.

## O que valida
- Prefix não nulo e não vazio.
- Prefix contendo `/files/usr`.
- Page size de runtime: `4096` ou `16384`.
- Presença e bit executável owner em:
  - `bin/sh`
  - `bin/pkg`
  - `bin/busybox`
  - `bin/proot`
- Presença de diretórios base: `bin`, `etc`, `lib`, `tmp`, `var`.
- Geração de JSON de status sem estouro de buffer.

## O que não substitui
- **Não substitui** `TermuxInstaller.java`.
- **Não substitui** extração com `ZipInputStream`.
- **Não usa** ZIPRAF como extrator ZIP real.
- **Não apaga** prefix e não altera pipeline oficial de bootstrap.

## Erros detectados
- Prefix inválido (`NULL`, vazio, sem `/files/usr`).
- Page size fora da política.
- Binários obrigatórios ausentes/inexecutáveis.
- Diretórios essenciais ausentes.
- Buffer insuficiente para JSON.

## Hook atual
Após mover staging para prefix final, o installer chama:
- `BootstrapBaremetalGuard.selftest()`
- `BootstrapBaremetalGuard.validateAfterBootstrap(TERMUX_PREFIX_DIR_PATH)`

No beta atual, falha é **WARN non-blocking**.
`BuildConfig.BOOTSTRAP_BAREMETAL_STRICT=false` por padrão.

## Rodando via ADB
1. Instalar APK beta.
2. Iniciar app e observar logcat:
```bash
adb logcat | grep -E "BootstrapBaremetalGuard|TermuxInstaller"
```
3. Confirmar payload JSON emitido pelo guard.
