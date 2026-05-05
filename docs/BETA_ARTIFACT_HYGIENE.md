# BETA_ARTIFACT_HYGIENE

## Classification

### OK_FOR_INTERNAL_BETA
- `dist/apk-matrix/signed/*.apk`
- `dist/apk-matrix/unsigned/*.apk`
- `dist/apk-matrix/SHA256SUMS.txt`
- `dist/apk-matrix/APK_SIZE_REPORT.tsv`
- `dist/apk-matrix/APK_SIZE_DIFF_RELEASE.tsv`
- `dist/apk-matrix/ARTIFACT_MANIFEST.txt`

### NOT_FOR_PUBLIC_RELEASE
- `dist/local-release.keystore`

## Policy
- Local keystore must not be uploaded in final beta/release artifact.
- Keep keystore only in runner local FS for ephemeral signing during CI validation.
