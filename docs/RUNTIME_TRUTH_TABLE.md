# RUNTIME_TRUTH_TABLE — mapa operacional RAFCODEΦ

| Recurso | Estado real | Evidência | Lacuna |
|---|---|---|---|
| APK build | PROVADO | Gradle/CI; `scripts/build_apk_matrix.sh` | — |
| release assinado | PROVADO | `./gradlew verifyReleaseContract` | segredo oficial no ambiente |
| bootstrap instala | PARCIAL | `TermuxInstaller` com staging/rollback | teste real em device |
| `sh` | PARCIAL | bootstrap/wrapper | validar no device |
| wrappers `cat/ls/clear/grep` | PROVADO ESTRUTURAL | `scripts/build_rafaelia_bootstraps.sh`, `scripts/bootstrap_zip_builder.c`, `tests/test_bootstrap_busybox_applet_wrappers.py`, inspeção zip no CI | smoke em device com APK novo |
| `pkg help` | PROVADO ESTRUTURAL | wrappers explícitos + `scripts/device_pkg_smoke.sh` camada mínima | smoke em device com APK novo |
| payload ARM real | BLOQUEADO | o gerador montou 73 pacotes por ABI, mas o validador encontrou `LEGACY_PREFIX_BINARY_RISK` em ELFs upstream compilados para `/data/data/com.termux/files/usr` | recompilar o fechamento de dependências para o prefixo RAFCODEΦ |
| `pkg` real | TOKEN_VAZIO | `scripts/build_real_arm_bootstrap_core.py` monta candidato com `apt`, `dpkg`, `coreutils`, `termux-tools`; a promoção permanece bloqueada pelo risco de prefixo binário | pacote core recompilado + `DEVICE_REAL_PKG_VALIDATED` |
| `apt` | TOKEN_VAZIO | candidato upstream contém prefixo legado em binários; o build seguro não o instala por padrão | backend recompilado para RAFCODEΦ e validado em device |
| `apt-get` | TOKEN_VAZIO | candidato upstream bloqueado por auditoria binária conservadora | backend recompilado para RAFCODEΦ e validado em device |
| `dpkg` | TOKEN_VAZIO | candidato upstream não pode ser promovido enquanto houver `LEGACY_PREFIX_BINARY_RISK` | binário recompilado e validado em device |
| `libapt` | TOKEN_VAZIO | dependency closure existe, mas bibliotecas upstream precisam rebuild para o prefixo RAFCODEΦ | teste dynamic-link em device |
| `busybox` | PARCIAL | bridge/delegação exige applet explícito; fallback usa toybox/toolbox | busybox real recompilado e validado em device |
| `proot` | TOKEN_VAZIO | candidato upstream gera `proot.real`, mas a pilha binária ainda contém prefixo legado | `proot --version` após rebuild RAFCODEΦ |
| certificados | TOKEN_VAZIO | candidato inclui `ca-certificates`, sem pilha TLS promovível ainda | TLS/package update em device |
| DNS/network básico | TOKEN_VAZIO | candidato escreve `etc/resolv.conf`, sem backend real promovido | network test em device |
| repositório configurado | TOKEN_VAZIO | candidato escreve `etc/apt/sources.list`, mas pacotes upstream não são prefix-safe | repositório RAFCODEΦ recompilado |
| device pkg smoke | PARCIAL | `scripts/device_pkg_smoke.sh` gera `reports/device_pkg_smoke.{json,md,log}` | `REQUIRE_REAL_PKG=true` após payload prefix-safe |
| `pkg update` | FUTURO | contrato definido em `scripts/device_pkg_smoke.sh` | teste real bloqueante |
| `pkg install` | FUTURO | contrato definido em `scripts/device_pkg_smoke.sh` | instalar nano/python/git |
| RAFAELIA JNI | PROVADO ESTRUTURAL | C/JNI | benchmark |
| CTI | PROVADO ESTRUTURAL | C scanner | teste com arquivos grandes |
| ZIPRAF | PROVADO ESTRUTURAL | manifesto | documentação clara; não é compressão |
| VCPU | PROVADO ESTRUTURAL | C state machine | falta VM completa |
| device smoke | PARCIAL | `scripts/device_runtime_smoke.sh`; auditoria manual em `docs/audits/DEVICE_BOOTSTRAP_COMMAND_WRAPPERS_AUDIT.md` | `DEVICE_SMOKE_REQUIRED=true` em CI real |

## Frase canônica do bootstrap

O build padrão gera payload bridge validável e não afirma `pkg` real. A geração ARM real é opt-in e permanece bloqueada quando o validador encontra prefixos legados dentro de ELFs; a promoção exige pacotes recompilados para RAFCODEΦ e relatório `DEVICE_REAL_PKG_VALIDATED` em dispositivo.
