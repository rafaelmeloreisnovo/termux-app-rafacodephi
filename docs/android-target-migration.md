# Migração de target SDK para Android recente (API 34)

Este documento resume os ajustes aplicados para elevar o `targetSdkVersion` e reduzir riscos funcionais.

## 1) Alteração de build

- `targetSdkVersion` atualizado para **34** em `gradle.properties`.
- `compileSdkVersion` permanece em **35**.

## 2) Pontos sensíveis revisados

### 2.1 PendingIntent mutável/imutável

Com target recente, `PendingIntent` deve declarar mutabilidade explicitamente.

Revisões aplicadas:

- `termux-shared/.../TermuxCrashUtils.java`
- `termux-shared/.../TermuxPluginUtils.java`

Ambos agora usam:

- `FLAG_UPDATE_CURRENT` para manter comportamento de atualização;
- `FLAG_IMMUTABLE` em API 23+ para atender exigências de segurança/compatibilidade.

> Observação: nos fluxos atuais dessas notificações não há necessidade de `FLAG_MUTABLE`.

### 2.2 Permissões runtime recentes

Situação revisada:

- `POST_NOTIFICATIONS` já é tratada para Android 13+.
- `READ_EXTERNAL_STORAGE`/`WRITE_EXTERNAL_STORAGE` permanecem apenas em fluxo legado (`maxSdkVersion=32` no Manifest e uso para Android < 11).
- Android 11+ usa `MANAGE_EXTERNAL_STORAGE` quando necessário para acesso amplo.
- Permissões de mídia (`READ_MEDIA_*`) já estão declaradas no Manifest para API 33+.

### 2.3 Storage / Document Provider

Situação revisada:

- O projeto já expõe `TermuxDocumentsProvider` para integração com SAF (`DocumentsProvider`).
- O fluxo de storage para Android recente mantém prioridade em SAF/Provider e usa permissões legadas apenas quando o SO realmente permite.
- A lógica de fallback para “All files access” (`MANAGE_EXTERNAL_STORAGE`) permanece ativa para casos que exigem acesso amplo ao filesystem.

## 3) Checklist de validação funcional recomendado

Antes de subir para target 35, validar pelo menos:

1. Notificações (abrir/fechar ações) sem exceções de `PendingIntent`.
2. Primeira execução em Android 13+ solicitando `POST_NOTIFICATIONS`.
3. Fluxos de compartilhamento/exportação em Android 10, 11 e 13+.
4. Acesso via SAF (seleção de arquivos/documentos) e `TermuxDocumentsProvider`.
5. Operações que dependem de “All files access” em Android 11+ com permissão concedida e negada.

## 4) Próximo passo sugerido (opcional)

Após validar checklist em dispositivos reais/emuladores:

- considerar atualizar para `targetSdkVersion=35`;
- repetir o checklist acima, com foco em regressões de permissões e intents implícitas.
