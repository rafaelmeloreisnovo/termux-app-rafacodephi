# Auditoria de Documentação — Raiz do Repositório

**Data da auditoria:** 2026-04-07 (UTC)
**Escopo:** `README.md` (raiz) vs estado real dos arquivos e configuração.

## Achados

### 1) Versão do fork agora explicitada
- **Tipo:** Compatibilidade documental
- **Local:** `README.md` (seção Installation)
- **Descrição:** A documentação agora diferencia versão de referência upstream (`v0.118.3`) da versão declarada no fork (`0.118.0-rafacodephi`).
- **Status:** Resolvido

### 2) Inventário global de markdowns exigido para auditoria total
- **Tipo:** Rastreabilidade de documentação
- **Local:** `docs/MARKDOWN_MAPA_ABSOLUTO.md`
- **Descrição:** Foi adicionado um mapa absoluto com todos os arquivos `.md` detectados no repositório.
- **Status:** Resolvido

### 3) Revisão completa case-insensitive de extensão markdown
- **Tipo:** Cobertura total de documentação
- **Local:** `docs/REVISAO_COMPLETA_MARKDOWN.md`
- **Descrição:** Revisão executada para todos os arquivos com sufixo `.md/.MD/.Md/.mD`, com tabela de status por arquivo.
- **Status:** Resolvido
