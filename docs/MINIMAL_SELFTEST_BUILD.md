# RAFCODEΦ Minimal Self-Test Build

Estado: `VERIFIED_LIMITED_LOCAL`  
Política: `claim_allowed=false`

## Finalidade

Fornecer a menor rota de compilação verificável do núcleo `bootstrap_rafaelia`, sem Gradle, Java, Python, Make, Git, rede ou Android SDK.

## Dependências mínimas

Obrigatórias:

- shell POSIX;
- `clang` ou `cc`;
- `mkdir`, `uname`, `tee`, `tr`.

Opcional:

- `sha256sum` ou `shasum` para digest do binário.

## Execução

```sh
sh scripts/build_minimal_selftest.sh
```

Saídas:

```text
build/minimal/raf_selftest
build/minimal/selftest.log
build/minimal/receipt.env
```

O receipt usa estado `PASS_LOCAL_LIMITED`; ele demonstra apenas que o núcleo C compilou e o self-test retornou zero falhas no ambiente registrado.

## Evidência local de criação

Fonte analisada: `termux-app-rafacodephi-master (4).zip`  
SHA-256 do ZIP: `c2549ba985b804dcda3a75261f97a28972aa1ededc873883156cb2e0f3cf05b5`

Execução de referência:

```text
architecture=x86_64
compiler=clang
test_summary=ok=21 fail=0
binary_sha256=55cb3e65ca794b81292364fe50215882141a7f0cbd436da7df8ceabfedb5beac
```

## Limites

Esta rota não prova:

- APK compilado;
- bootstrap apt/dpkg real;
- execução ARM32 ou ARM64;
- instalação Android;
- CI remoto;
- desempenho;
- compatibilidade de release.

Esses estados permanecem `TOKEN_VAZIO` até receipts específicos.

## Próximo gate

Executar o mesmo script no Termux ARM32 e ARM64, preservar `receipt.env`, binário, logs, commit e SHA-256 e comparar com uma segunda execução independente.
