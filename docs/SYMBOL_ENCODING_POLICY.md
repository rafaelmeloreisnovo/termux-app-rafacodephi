# Symbol Encoding Policy

## Naming Fields
- `human_name`: human-readable name; Unicode allowed.
- `technical_name`: ASCII-safe canonical technical identifier.
- `file_slug`: lowercase ASCII for file and artifact names.
- `env_prefix`: uppercase ASCII prefix for environment variables.
- `url_slug`: URL-safe slug; percent-encode when needed.
- `json_key`: ASCII-safe key for JSON structures.

## Rules
1. `human_name` may contain Unicode for documentation.
2. `technical_name` must be ASCII-safe.
3. `file_slug` must be lowercase ASCII.
4. `env_prefix` must be uppercase ASCII.
5. `url_slug` must use percent-encoding when required.
6. `json_key` must be ASCII-safe.
7. HTML must declare UTF-8 (`<meta charset="UTF-8">`).
8. Query string composition must use `URLSearchParams` or `encodeURIComponent` in web contexts.

## Canonical Mapping Table
| human_name | technical_name | file_slug | env_prefix | url_slug | json_key |
|---|---|---|---|---|---|
| RAFCODEΦ | rafcodephi | rafcodephi | RAFCODEPHI | rafcodephi | rafcodephi |
| Termux RAFCODEΦ | termux_rafcodephi | termux_rafcodephi | TERMUX_RAFCODEPHI | termux_rafcodephi | termux_rafcodephi |
| BitΩ | bitomega | bitomega | BITOMEGA | bitomega | bitomega |
| BLAKE3/RMR | blake3_rmr | blake3_rmr | BLAKE3_RMR | blake3_rmr | blake3_rmr |
| Vectra/RMR | vectra_rmr | vectra_rmr | VECTRA_RMR | vectra_rmr | vectra_rmr |

## Safety Notes
- Do not use Unicode in paths, env vars, Gradle task names, artifact names, or technical identifiers.
- Avoid raw symbol concatenation in shell/URL/JSON/Markdown generation.
- Record encoding, escaping, normalization, and direction risks in reports only; do not infer semantic meaning.
