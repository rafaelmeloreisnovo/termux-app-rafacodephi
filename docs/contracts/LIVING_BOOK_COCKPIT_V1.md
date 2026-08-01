# Contrato do Cockpit Livro Vivo V1

A V1 materializa no Termux um menu/configuração **somente de inspeção** para:

```text
Perfil ativo
Domínio e semente
Espelhos humano / IA
Módulos
Propostas
Permissões
Receipts
Privacidade e segurança
Sincronização
Auditoria
```

O script `scripts/living_book_cockpit.py` valida um bundle descriptor-only, apresenta seu estado e pode emitir um template de receipt. Ele não despacha, executa, publica, mescla, apaga, compartilha, sincroniza escrita, abre alvo de rede ou avalia shell.

## Exemplo

```bash
python3 scripts/living_book_cockpit.py \
  --contract configs/living-book-cockpit-v1.json \
  --bundle /caminho/LBB-MUSIC-0001.bundle.json \
  --device-profile armv7l-termux \
  --receipt-out COMPILA/living-book/LBB-MUSIC-0001.inspect-receipt.json
```

O receipt produzido nesta etapa declara:

```text
execution_performed=false
claim_allowed=false
decision=INSPECT_ONLY_NO_DISPATCH
```

## Fronteira

A integração visual no menu Android permanece:

```text
TOKEN_VAZIO_ANDROID_UI_WIRING
```

A execução em dispositivo permanece:

```text
TOKEN_VAZIO_RUNTIME_NOT_EXECUTED
```

A prova atual cobre contrato, validação e CLI local equivalente. Não demonstra build de APK, instalação, UI renderizada, autorização humana real ou execução física.
