# bootstrap_rafaelia (experimental)

Módulo isolado experimental baseado em BOOTSTRAP_LOWLEVEL_RAFAELIA.txt.

- Não substitui o bootstrap real do Termux.
- Não substitui `TermuxInstaller.java`.
- ZIPRAF aqui é formato experimental próprio e **não** extrator ZIP real.
- Não usar ZIPRAF para `bootstrap.zip` do Termux.

## Build
- `make -C bootstrap_rafaelia host-smoke`
- `make -C bootstrap_rafaelia arm64-freestanding`

## Termux compat (auxiliar, sem substituir bootstrap real)
- `make -C bootstrap_rafaelia termux-compat`
- Gera:
  - `libbootstrap_rafaelia_arm32.a`
  - `libbootstrap_rafaelia_arm64.a`
  - payload auxiliar em `bootstrap_rafaelia/out-termux-compat/`
