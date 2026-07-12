# REAL_PKG_CORE_BOOTSTRAP_IMPLEMENTATION

## Correção operacional

O build padrão não pode fingir que pacotes upstream compilados para outro prefixo formam um `pkg` real utilizável. O caminho seguro permanece bridge-only:

```bash
RAFCODEPHI_REAL_PKG_BOOTSTRAP=false bash scripts/build_rafaelia_bootstraps.sh
```

A geração do candidato ARM real é opt-in:

```bash
RAFCODEPHI_REAL_PKG_BOOTSTRAP=true bash scripts/build_rafaelia_bootstraps.sh
```

Esse comando primeiro gera os zips bridge para todos os ABIs e depois tenta sobrescrever:

```text
app/src/main/cpp/rewritten-bootstrap-aarch64.zip
app/src/main/cpp/rewritten-bootstrap-arm.zip
```

com candidato gerado por:

```bash
python3 scripts/build_real_arm_bootstrap_core.py --arch all
```

## Resultado observado no CI

O gerador resolveu e montou 73 pacotes para cada ABI (`aarch64` e `arm`). A validação bloqueou a promoção porque dezenas de ELFs e bibliotecas ainda contêm:

```text
/data/data/com.termux/files/usr
```

Exemplos observados incluem `bin/bash`, `bin/apt-cache`, `bin/find`, `bin/gzip`, `lib/libgnutls.so` e outras dependências.

O prefixo RAFCODEΦ é maior:

```text
/data/data/com.termux.rafacodephi/files/usr
```

Logo, replace binário em-place não é seguro: mudaria o tamanho das strings e poderia corromper offsets, seções ou dados internos do ELF.

## Conteúdo pretendido do payload ARM real

O candidato contém fechamento de dependências para:

```text
apt
apt-get
dpkg
pkg
bash
busybox
coreutils
findutils
grep
sed
gawk
tar
gzip
ncurses-utils
ca-certificates
proot/proot.real
termux-tools
sources.list
resolv.conf
```

## Gate

A validação continua obrigatória:

```bash
python3 scripts/validate_real_arm_bootstrap_core.py \
  app/src/main/cpp/rewritten-bootstrap-aarch64.zip \
  app/src/main/cpp/rewritten-bootstrap-arm.zip
```

Qualquer `LEGACY_PREFIX_BINARY_RISK` bloqueia promoção. O hotfix não ignora o erro e não faz replace automático.

## Caminho válido para promoção

1. recompilar os 73 pacotes e seu fechamento de dependências com o prefixo RAFCODEΦ;
2. publicar repositório APT RAFCODEΦ com hashes e manifests fixados;
3. gerar os zips ARM usando esse repositório;
4. passar o validador sem risco binário;
5. instalar APK em device real;
6. executar:

```bash
REQUIRE_REAL_PKG=true ./scripts/device_pkg_smoke.sh
```

## Veredito

F_ok: o gerador, a resolução de dependências, os hashes e o detector binário funcionaram e impediram uma promoção insegura.

F_gap: pacotes upstream não são relocáveis para o prefixo RAFCODEΦ por substituição textual.

F_next: rebuild prefix-aware do fechamento de 73 pacotes por ABI; até lá, build padrão bridge-only e `pkg real = TOKEN_VAZIO`.
