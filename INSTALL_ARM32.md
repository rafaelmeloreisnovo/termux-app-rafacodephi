# INSTALL_ARM32.md

## Moto E7 Power 32-bit: qual APK instalar
Para Moto E7 Power com Android 32-bit, instale:
- `termux-rafcodephi-debug-armeabi-v7a.apk` (preferencial para ARM32)
- ou `termux-rafcodephi-debug-universal.apk` (contém ARM32 e ARM64)

Não instale `arm64-v8a` em sistema 32-bit, porque o loader do Android 32-bit não executa binários 64-bit.

## Conceitos rápidos
- **minSdk**: API mínima que pode instalar o app (neste projeto, 21).
- **targetSdk**: API alvo de comportamento/compatibilidade do app (neste projeto, 34).
- **compileSdk**: API usada para compilar (neste projeto, 35).
- **ABI**: arquitetura de CPU do binário nativo (`armeabi-v7a`, `arm64-v8a`, etc).

## Diagnóstico de erro de instalação (ADB)
Use:

```bash
adb install -r termux-rafcodephi-debug-armeabi-v7a.apk
```

Se falhar, colete:

```bash
adb shell getprop ro.product.cpu.abi
adb shell getprop ro.product.cpu.abilist
adb shell pm path com.termux.rafacodephi
adb logcat -d | rg -i "PackageManager|INSTALL_FAILED|No matching ABIs"
```

Erros comuns:
- `INSTALL_FAILED_NO_MATCHING_ABIS`: APK não contém `armeabi-v7a`.
- `INSTALL_FAILED_OLDER_SDK`: aparelho abaixo de API 21.
- `INSTALL_PARSE_FAILED_NO_CERTIFICATES`: APK sem assinatura válida para fluxo de instalação.

## Bootstrap ARM32
O build ARM32 depende de `app/src/main/cpp/bootstrap-arm.zip` presente e válido.
A CI `compatibility-arm32` valida a presença desse arquivo e publica relatório específico.

## Observação de variante bootstrap
O projeto preserva o bootstrap `apt-android-7` para Android 7+.
