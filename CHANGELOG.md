# Changelog

## Unreleased

### Android target SDK migration (API 34)

- Atualizado `targetSdkVersion` para **34** em `gradle.properties`.
- Revisado uso de `PendingIntent` para compatibilidade com targets recentes:
  - notificações de crash/plugin agora usam `FLAG_IMMUTABLE` (API 23+) junto com `FLAG_UPDATE_CURRENT`.
- Revisados fluxos de permissões e storage:
  - `POST_NOTIFICATIONS` continua solicitado apenas no Android 13+;
  - `READ/WRITE_EXTERNAL_STORAGE` ficam restritos ao fluxo legado (Android < 11);
  - Android 11+ mantém fluxo por `MANAGE_EXTERNAL_STORAGE` + SAF/`DocumentsProvider`.
- Criada documentação de migração em `docs/android-target-migration.md`.
