# GDrive Plugin V2 — Local Contract Receipt

```text
state=LOCAL_CONTRACT_PASS
tests=9/9
claim_allowed=false
release_allowed=false
merge_allowed_by_receipt=false
```

## Invariante executada

```text
SUCCESS ⇔ AUTH ∧ HTTP_2XX ∧ UNIQUE_DRIVE_ID ∧ NO_CONFLICT ∧ HASH_CONFIRMED ∧ STATE_COMMITTED
```

A execução local comprovou contratos e testes sintéticos em Linux x86_64, incluindo OAuth `state` e PKCE S256. Não acessou conta Google real, Android físico, APK ou bootstrap.

## TOKEN_VAZIO preservados

- OAuth com conta real;
- upload/update/download em pasta real;
- falhas de rede e retry/backoff;
- Termux ARMv7 e ARM64;
- integração APK/bootstrap;
- assinatura, revisão de segurança e release.

## Arquivos e hashes

- `configs/gdrive-plugin-profile.json` — `bc7e2695a8ca98343328ee8ae50570f1e0e1c25079a332e7e3602758a617f3ec`
- `plugins/gdrive-plugin/README.md` — `7cf6af13da6e48605adb976a23840a3ac92eca1b57fe55bbfa2aca539a49359c`
- `plugins/gdrive-plugin/gdrive-auth.sh` — `9f4c970b34edab8a5602efa2882d93d94fe30a9bdc2af45b01db3616640a2b59`
- `plugins/gdrive-plugin/gdrive-config.json` — `ff1e27680cefb1bd97176df98735474800fd6f32d3adef3566f1afb0911c116e`
- `plugins/gdrive-plugin/gdrive-sync.sh` — `4b0cab34629b845f320d64f44fcdaaf15832ec579dba926726e2ff09dd256a51`
- `plugins/gdrive-plugin/gdrive-sync-core.py` — `1115a3220953c91c524b165e9f93af69dcd5642d3eb71735870c044e109234dc`
- `plugins/gdrive-plugin/gdrive-webservice.sh` — `76b975fac84d8d456e8214f6ecce832b97a2d0a2a268f0e359e66f99701a5ddd`
- `plugins/gdrive-plugin/tests/test_gdrive_plugin.sh` — `c635213b69eef377607d52706a72000813d3b482f3e349dd12d2b8bdd927ccd8`

## Resultado

`F_ok`: núcleo fail-closed, PKCE S256 e 9/9 contratos locais.  
`F_gap`: prova remota/física, fault injection e empacotamento.  
`F_next`: conta/pasta isolada + dispositivos reais + receipts, sem merge antecipado.
