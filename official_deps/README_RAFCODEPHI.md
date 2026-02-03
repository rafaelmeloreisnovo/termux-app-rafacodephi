# RAFCODEΦ Official Deps (vendored)

Este diretório contém dependências upstream **vendorizadas** para uso offline no Termux RAFCODEΦ.

- `termux-packages/` (upstream base) com defaults ajustados para:
  - `TERMUX_APP__PACKAGE_NAME=com.termux.rafacodephi`
  - `TERMUX__REPOS_HOST_ORG_NAME=instituto-Rafael`

- `proot-distro/` (upstream base) + scripts `rafaelia/` para perf/audit.

> Nota: Você pode sobrescrever qualquer default exportando variáveis antes dos scripts.
