# INSTALL_ARM32

## APK correto para Moto E7 Power 32-bit

Para Moto E7 Power com Android 32-bit (userspace ARM32), instale:

- `termux-rafcodephi-debug-armeabi-v7a.apk` (preferencial)
- ou `termux-rafcodephi-debug-universal.apk`

Não instale `arm64-v8a` em sistema Android 32-bit: o PackageManager rejeita por incompatibilidade de ABI.

## Por que arm64-v8a não funciona em Android 32-bit

Mesmo que o SoC suporte 64-bit, se o Android foi instalado como 32-bit, o processo app roda ABI 32-bit (`armeabi-v7a`).
APK `arm64-v8a` exige userspace 64-bit e falha na instalação.

## Diferença entre minSdk, targetSdk, compileSdk e ABI

- `minSdk`: menor versão Android permitida para instalar/executar.
- `targetSdk`: versão alvo de comportamento/runtime e políticas de compatibilidade.
- `compileSdk`: API usada para compilar.
- `ABI`: arquitetura nativa embarcada no APK (`armeabi-v7a`, `arm64-v8a`, etc.).

## Diagnóstico de erro com adb install

Comandos úteis:

```bash
adb install -r termux-rafcodephi-debug-armeabi-v7a.apk
adb shell getprop ro.product.cpu.abi
adb shell getprop ro.product.cpu.abilist
adb logcat -d | rg -n "PackageManager|INSTALL_FAILED|abi"
```

Falhas comuns:

- `INSTALL_FAILED_NO_MATCHING_ABIS`: APK sem `armeabi-v7a` para sistema 32-bit.
- `INSTALL_PARSE_FAILED_*`: APK corrompido ou assinatura inválida.

## Bootstrap ARM32

Este projeto preserva `apt-android-7` e exige presença de `app/src/main/cpp/bootstrap-arm.zip`, validada em CI no job `compatibility-arm32`.
