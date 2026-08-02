# PA embedded runtime V1

O arquivo `pa.zip` enviado por Rafael é incorporado diretamente ao APK Termux RAFCODEΦ e instalado sem rede pelo próprio ciclo de bootstrap do aplicativo.

## Contrato

- Fonte: `pa.zip`
- Tamanho: `48.366` bytes
- Entradas ZIP: `84`
- SHA-256: `1655dc886b549a006e28553ddf3e76dcee9d4838956aa81a1cac4f56b594a08f`
- Cópia embutida: `$PREFIX/libexec/rafcodephi-pa-v1`
- Comando público: `$PREFIX/bin/pa`
- Prioridade do wrapper: `$HOME/PEDRA_ANGULAR/bin/pa` existente; caso contrário, cópia embutida.

## Instalação

`BootstrapBaremetalGuard.validateAfterBootstrap()` chama `PaPayloadInstaller.ensureInstalled(prefix)` depois de validar o filesystem e o shell do bootstrap. O instalador:

1. remonta o payload Base64;
2. valida tamanho e SHA-256 antes da extração;
3. rejeita caminhos absolutos, `..`, `.` e barras invertidas;
4. limita quantidade de entradas e bytes extraídos;
5. extrai numa árvore de staging;
6. adapta shebangs para o package/prefixo real do APK;
7. promove a árvore de forma atômica;
8. grava o marcador de proveniência;
9. publica o comando `pa` em `$PREFIX/bin`.

A árvore autoral já existente em `$HOME/PEDRA_ANGULAR` não é apagada nem substituída.

## Fronteira de evidência

- Payload local reconstruído: `PASS`
- Tamanho, SHA-256 e 84 entradas: `PASS`
- Compilação Android e CI: aguardando execução da PR
- Instalação em aparelho físico: `TOKEN_VAZIO_NOT_EXECUTED`
- `claim_allowed=false`
