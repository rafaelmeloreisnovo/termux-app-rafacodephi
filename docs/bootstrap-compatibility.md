# Decisão arquitetural: compatibilidade de prefixo e bootstrap

## Contexto

Este projeto precisa equilibrar:

1. **Identidade canônica side-by-side** do fork (`com.termux.rafacodephi`).
2. **Compatibilidade controlada** com ecossistema legado de plugins/extensões Termux.
3. **Segurança de build** para evitar bootstrap incompatível com package/page size.

## Estratégia única de prefixo

### Estado canônico (release)

- `applicationId` padrão do app: `com.termux.rafacodephi`.
- Bootstrap esperado por padrão:
  - `TERMUX_PACKAGE_NAME=com.termux.rafacodephi`
  - `TERMUX_PAGE_SIZE=16384`
  - arquivo `BOOTSTRAP_INFO` embutido no zip.
- Override por ambiente continua disponível para trilhas de compatibilidade/validação:
  - `TERMUX_APP_PACKAGE_NAME`
  - `TERMUX_BOOTSTRAP_PACKAGE_NAME`
  - `TERMUX_BOOTSTRAP_PAGE_SIZE`

## Validação obrigatória no build

O `app/build.gradle` valida `BOOTSTRAP_INFO` para cada bootstrap baixado e **falha o build** quando:

- `BOOTSTRAP_INFO` está ausente.
- `TERMUX_PACKAGE_NAME` difere do valor esperado.
- `TERMUX_PAGE_SIZE` difere do valor esperado.

Isso bloqueia versões de bootstrap sem metadados compatíveis.

## Matriz de compatibilidade (packageId x bootstrap x plugins)

| applicationId (app) | Bootstrap (`BOOTSTRAP_INFO: TERMUX_PACKAGE_NAME`) | Suporte plugins legados | Status |
|---|---|---|---|
| `com.termux.rafacodephi` | `com.termux.rafacodephi` | Baixo/Médio (requer atualização do ecossistema) | **Padrão atual** |
| `com.termux` | `com.termux` | Alto (compatibilidade imediata) | Compat legado explícita |
| `com.termux` | `com.termux.rafacodephi` | Médio (depende do plugin e da forma de descoberta) | Trilho de validação |
| `com.termux.rafacodephi` | `com.termux` | Inconsistente (não recomendado) | Bloquear |

## Diretriz operacional

- Release oficial usa `applicationId=com.termux.rafacodephi`.
- Para trilhas legadas/experimentos no CI, definir explicitamente:

```bash
export TERMUX_APP_PACKAGE_NAME=com.termux
export TERMUX_BOOTSTRAP_PACKAGE_NAME=com.termux
export TERMUX_BOOTSTRAP_PAGE_SIZE=16384
```
