# Inventário Beta Termux Rafacodephi

Data: 2026-05-05

## Escopo mínimo beta

- App inicia sem crash (validação de build e testes unitários concluída).
- Sessão terminal inicializa com tratamento de erro e cleanup defensivo.
- Encerramento terminal com finalização de grupo de processo para reduzir risco de zumbi.
- Build com artefatos unsigned e signed locais gerados por compilação real.

## Inventário de binários gerados por compilação

Diretório: `dist/apk-matrix/`

### Unsigned
- `unsigned/termux-app_apt-android-7-release_armeabi-v7a.apk`
- `unsigned/termux-app_apt-android-7-release_arm64-v8a.apk`
- `unsigned/termux-app_apt-android-7-release_universal.apk`
- `unsigned/termux-app_apt-android-7-release_x86.apk`
- `unsigned/termux-app_apt-android-7-release_x86_64.apk`
- `unsigned/termux-app_apt-android-7-debug_armeabi-v7a.apk`
- `unsigned/termux-app_apt-android-7-debug_arm64-v8a.apk`
- `unsigned/termux-app_apt-android-7-debug_universal.apk`
- `unsigned/termux-app_apt-android-7-debug_x86.apk`
- `unsigned/termux-app_apt-android-7-debug_x86_64.apk`

### Signed (validação interna)
- `signed/termux-app_apt-android-7-release_armeabi-v7a-signed.apk`
- `signed/termux-app_apt-android-7-release_arm64-v8a-signed.apk`
- `signed/termux-app_apt-android-7-release_universal-signed.apk`
- `signed/termux-app_apt-android-7-release_x86-signed.apk`
- `signed/termux-app_apt-android-7-release_x86_64-signed.apk`

## Hashes SHA-256 oficiais desta execução

Fonte de verdade: `dist/apk-matrix/SHA256SUMS.txt`

- `c3185ae28fa46d7ae701b352d59117b28a35457dda7777cf5def92b6c45935d3  unsigned/termux-app_apt-android-7-release_armeabi-v7a.apk`
- `79a1efd89f1cb06f6782a91394883a89cc24f90b4fa77f7519f213b42540b656  unsigned/termux-app_apt-android-7-release_arm64-v8a.apk`
- `68f8fc167a45968fa546c732f2c1ae7bf238613a65a46896fbc85883f1cd110e  signed/termux-app_apt-android-7-release_armeabi-v7a-signed.apk`
- `b7d5513964fdf11fc6ea703e9caf5b022c103ec808923539e602c4698271afee  signed/termux-app_apt-android-7-release_arm64-v8a-signed.apk`

## Estado beta

- Build reproduzível em trilha local: **OK_BETA**.
- arm32 + arm64 release unsigned e signed local: **OK_BETA**.
- Recursos teóricos RAFAELIA fora do gate de release: **EXPERIMENTAL_NOT_BLOCKING**.
- Validação em device real para abertura interativa e ausência de zumbi: **RISK_BETA** (pendente execução em Android runtime).
