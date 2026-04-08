# Decisão arquitetural: compatibilidade de prefixo e bootstrap

## Contexto

Este projeto precisa equilibrar:

1. **Compatibilidade imediata** com ecossistema legado de plugins/extensões Termux.
2. **Migração controlada** para um bootstrap customizado com metadados explícitos.
3. **Segurança de build** para evitar bootstrap incompatível com package/page size.

## Estratégia única de prefixo

### Curto prazo (desenvolvimento ativo)

- `applicationId` do app permanece em `com.termux`.
- Objetivo: manter compatibilidade imediata com integrações legadas que resolvem pacote por prefixo histórico.

### Médio/longo prazo (migração planejada)

- Produzir bootstrap customizado via `termux-packages` com:
  - `TERMUX_PACKAGE_NAME=com.termux.rafacodephi`
  - `TERMUX_PAGE_SIZE=16384`
  - arquivo `BOOTSTRAP_INFO` embutido no zip.
- No app, a expectativa de metadados é configurável por ambiente:
  - `TERMUX_BOOTSTRAP_PACKAGE_NAME`
  - `TERMUX_BOOTSTRAP_PAGE_SIZE`
- Assim, o pipeline pode migrar sem alterar código-fonte novamente.

## Validação obrigatória no build

O `app/build.gradle` valida `BOOTSTRAP_INFO` para cada bootstrap baixado e **falha o build** quando:

- `BOOTSTRAP_INFO` está ausente.
- `TERMUX_PACKAGE_NAME` difere do valor esperado.
- `TERMUX_PAGE_SIZE` difere do valor esperado.

Isso bloqueia versões de bootstrap sem metadados compatíveis.

## Matriz de compatibilidade (packageId x bootstrap x plugins)

| applicationId (app) | Bootstrap (`BOOTSTRAP_INFO: TERMUX_PACKAGE_NAME`) | Suporte plugins legados | Status |
|---|---|---|---|
| `com.termux` | `com.termux` | Alto (compatibilidade imediata) | **Padrão atual** |
| `com.termux` | `com.termux.rafacodephi` | Médio (depende do plugin e da forma de descoberta) | Transição planejada |
| `com.termux.rafacodephi` | `com.termux.rafacodephi` | Baixo/Médio (requer atualização do ecossistema) | Futuro possível |
| `com.termux.rafacodephi` | `com.termux` | Inconsistente (não recomendado) | Bloquear |

## Diretriz operacional

- Enquanto o ecossistema de plugins não estiver totalmente adaptado, manter `applicationId=com.termux`.
- Para validar bootstrap customizado no CI, definir:

```bash
export TERMUX_BOOTSTRAP_PACKAGE_NAME=com.termux.rafacodephi
export TERMUX_BOOTSTRAP_PAGE_SIZE=16384
```

- Só avançar para `applicationId=com.termux.rafacodephi` quando a matriz de compatibilidade for reavaliada e os plugins críticos estiverem cobertos.
