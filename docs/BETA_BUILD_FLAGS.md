# BETA_BUILD_FLAGS

## Flags propostas para beta
- `RAF_BETA_BUILD=ON`
- `RAF_ENABLE_RMR=OFF` (default seguro)
- `RAF_ENABLE_LOWLEVEL=ON`
- `RAF_ENABLE_EXPERIMENTAL=OFF`
- `RAF_ENABLE_ASM=OFF`
- `RAF_ENABLE_BITRAF=ON` (somente com selftest mínimo)

## Regra operacional
1. Fluxo terminal/base Termux é fonte de verdade.
2. Lowlevel/RMR não podem bloquear `assembleDebug`.
3. Features experimentais devem ser opt-in explícito em CI e local.
