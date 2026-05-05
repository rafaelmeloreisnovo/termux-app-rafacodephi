# Plano de integração experimental: bootstrap_rafaelia

- `TermuxInstaller.java` permanece bootstrap real.
- `bootstrap_rafaelia/` é apenas módulo experimental auxiliar.
- Não substituir extração de `bootstrap.zip`.
- Não substituir `SYMLINKS.txt`.
- Não substituir validação de `bin/sh`, `bin/pkg`, `busybox`, `proot`.
- Não substituir verificação BLAKE3 do bootstrap real.
- ZIPRAF experimental não é ZIP real e não deve ser ligado ao pipeline de bootstrap atual.

- Compatibilidade Termux adicionada como payload auxiliar (`make -C bootstrap_rafaelia termux-compat`) para arm32/arm64, sem hook no bootstrap real.
