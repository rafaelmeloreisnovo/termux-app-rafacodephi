# Bootstrap Rewrite Pipeline (RAFCODEΦ)

## Developer bootstrap vs Runtime bootstrap
- `scripts/generate_developer_bootstraps.sh` gera bootstrap mínimo para smoke local (`BUILD_ONLY`) e **não** serve para runtime real.
- Runtime usa `rewriteBootstraps` para converter bootstraps completos por ABI e produzir `BOOTSTRAP_RUNTIME_READY=1`.

## Por que não patch binário ingênuo
- ELF/`.so`/`.a`/`.o` não recebem substituição textual cega.
- O pipeline detecta ELF por magic bytes `0x7F 45 4C 46`, valida arquitetura e falha se houver hardcode de `/data/data/com.termux`.
- Reescrita ocorre somente em arquivos textuais seguros.

## Gerar bootstraps (Moto E7 Power ARM32)
```bash
./gradlew :app:rewriteBootstraps :app:validateRewrittenBootstraps
ls -lh app/src/main/cpp/rewritten-bootstrap-arm.zip
```

## Validar com adb install + smoke runtime
```bash
./gradlew :app:assembleDebug
adb install -r app/build/outputs/apk/debug/termux-rafcodephi-debug-universal.apk
adb shell monkey -p com.termux.rafacodephi -c android.intent.category.LAUNCHER 1
adb shell run-as com.termux.rafacodephi ls files/usr/bin
adb shell run-as com.termux.rafacodephi sh -lc 'echo ok'
```
