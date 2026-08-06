#!/usr/bin/env bash
# OAuth 2.0 + PKCE handler for Termux RAFCODEΦ. Logs use stderr; stdout is data-only.
set -euo pipefail
umask 077
CURL="${GDRIVE_CURL_BIN:-curl}"
JQ="${GDRIVE_JQ_BIN:-jq}"
PYTHON="${GDRIVE_PYTHON_BIN:-python3}"
CFG="${GDRIVE_CONFIG_DIR:-${HOME}/.config/gdrive-plugin}"
TOKENS="${GDRIVE_TOKENS_FILE:-${CFG}/tokens.json}"
CREDS="${GDRIVE_CREDENTIALS_FILE:-${CFG}/credentials.json}"
LOG="${GDRIVE_AUTH_LOG_FILE:-${CFG}/auth.log}"
AUTH_URL='https://accounts.google.com/o/oauth2/v2/auth'
TOKEN_URL='https://oauth2.googleapis.com/token'
REVOKE_URL='https://oauth2.googleapis.com/revoke'
REDIRECT_DEFAULT='http://127.0.0.1:53682/'
SCOPE_DEFAULT='https://www.googleapis.com/auth/drive.file'
SKEW=60

need(){ command -v "$1" >/dev/null 2>&1 || { echo "ERROR: missing $1" >&2; return 127; }; }
init(){ mkdir -p "$CFG"; chmod 700 "$CFG"; touch "$LOG"; chmod 600 "$LOG"; }
log(){ local x="[$(date -u +'%Y-%m-%dT%H:%M:%SZ')] $1"; echo "$x" >&2; echo "$x" >>"$LOG"; }
uri(){ "$JQ" -nr --arg v "$1" '$v|@uri'; }
decode(){ local v="${1//+/ }"; printf '%b' "${v//%/\\x}"; }
state(){ od -An -N16 -tx1 /dev/urandom | tr -d ' \n'; }
pkce(){ "$PYTHON" - <<'PY'
import base64,hashlib,secrets
v=secrets.token_urlsafe(64)[:86]
c=base64.urlsafe_b64encode(hashlib.sha256(v.encode()).digest()).rstrip(b'=').decode()
print(v);print(c)
PY
}

http(){
  local tmp code rc=0; tmp=$(mktemp "${TMPDIR:-/tmp}/gdrive-http.XXXXXX")
  code=$("$CURL" -sS -o "$tmp" -w '%{http_code}' "$@") || rc=$?
  if ((rc)); then log "curl transport failure rc=$rc"; cat "$tmp" >&2 || true; rm -f "$tmp"; return "$rc"; fi
  if [[ ! "$code" =~ ^2[0-9][0-9]$ ]]; then log "HTTP rejected status=$code"; cat "$tmp" >&2 || true; rm -f "$tmp"; return 22; fi
  cat "$tmp"; rm -f "$tmp"
}

atomic_json(){ local path="$1" value="$2" tmp; tmp=$(mktemp "${path}.tmp.XXXXXX"); printf '%s\n' "$value" >"$tmp"; chmod 600 "$tmp"; mv -f "$tmp" "$path"; }
save_creds(){
  local value; value=$("$JQ" -cn --arg id "$1" --arg secret "$2" --arg redirect "$3" --arg scope "$4" '{schema:"gdrive-plugin-credentials/v2",client_id:$id,client_secret:$secret,redirect_uri:$redirect,scope:$scope}')
  atomic_json "$CREDS" "$value"
}
save_tokens(){
  local response="$1" previous="${2:-}" access refresh expires expiry value
  access=$("$JQ" -er '.access_token|strings|select(length>0)' <<<"$response") || { log 'token response missing access_token'; return 1; }
  refresh=$("$JQ" -r '.refresh_token//empty' <<<"$response"); [[ -n "$refresh" ]] || refresh="$previous"
  expires=$("$JQ" -er '.expires_in|numbers' <<<"$response") || expires=3600; expiry=$(( $(date +%s)+expires ))
  value=$("$JQ" -cn --arg a "$access" --arg r "$refresh" --argjson e "$expires" --argjson x "$expiry" --arg t "$(date -u +'%Y-%m-%dT%H:%M:%SZ')" '{schema:"gdrive-plugin-tokens/v2",access_token:$a,refresh_token:$r,expires_in:$e,expiry:$x,created_at:$t}')
  atomic_json "$TOKENS" "$value"
}

auth_code(){
  local id="$1" redirect="$2" scope="$3" challenge="$4" st url pasted ps raw code
  st=$(state)
  url="$AUTH_URL?client_id=$(uri "$id")&redirect_uri=$(uri "$redirect")&response_type=code&scope=$(uri "$scope")&access_type=offline&prompt=consent&state=$(uri "$st")&code_challenge=$(uri "$challenge")&code_challenge_method=S256"
  log "OAuth started scope=$scope PKCE=S256"; printf '\nOpen:\n%s\n\nPaste COMPLETE redirected URL:\n> ' "$url" >&2
  command -v termux-open-url >/dev/null 2>&1 && termux-open-url "$url" >/dev/null 2>&1 || true
  IFS= read -r pasted
  [[ "$pasted" == *code=* && "$pasted" == *state=* ]] || { log 'redirect lacks code/state'; return 1; }
  ps=${pasted#*state=}; ps=${ps%%&*}; ps=$(decode "$ps"); [[ "$ps" == "$st" ]] || { log 'OAuth state mismatch'; return 1; }
  raw=${pasted#*code=}; raw=${raw%%&*}; code=$(decode "$raw"); [[ -n "$code" ]] || return 1; printf '%s\n' "$code"
}

exchange(){
  local id="$1" secret="$2" redirect="$3" code="$4" verifier="$5" response; local args
  args=(-X POST "$TOKEN_URL" -H 'Content-Type: application/x-www-form-urlencoded' --data-urlencode "client_id=$id" --data-urlencode "code=$code" --data-urlencode "code_verifier=$verifier" --data-urlencode "redirect_uri=$redirect" --data-urlencode 'grant_type=authorization_code')
  [[ -z "$secret" ]] || args+=(--data-urlencode "client_secret=$secret")
  response=$(http "${args[@]}"); save_tokens "$response"; log 'tokens stored mode=600'
}
refresh(){
  [[ -f "$CREDS" && -f "$TOKENS" ]] || { log 'credentials/tokens missing'; return 1; }
  local id secret r response; local args
  id=$("$JQ" -er .client_id "$CREDS"); secret=$("$JQ" -r '.client_secret//empty' "$CREDS"); r=$("$JQ" -er '.refresh_token|strings|select(length>0)' "$TOKENS")
  args=(-X POST "$TOKEN_URL" -H 'Content-Type: application/x-www-form-urlencoded' --data-urlencode "client_id=$id" --data-urlencode "refresh_token=$r" --data-urlencode 'grant_type=refresh_token')
  [[ -z "$secret" ]] || args+=(--data-urlencode "client_secret=$secret")
  response=$(http "${args[@]}"); save_tokens "$response" "$r"; log 'token refreshed'
}
token(){
  [[ -f "$TOKENS" ]] || { log 'no tokens'; return 1; }; "$JQ" -e . "$TOKENS" >/dev/null
  local expiry; expiry=$("$JQ" -er '.expiry|numbers' "$TOKENS"); ((expiry>$(date +%s)+SKEW)) || refresh
  "$JQ" -er '.access_token|strings|select(length>0)' "$TOKENS"
}
revoke(){
  [[ -f "$TOKENS" ]] || return 0; local t; t=$("$JQ" -er .access_token "$TOKENS")
  http -X POST "$REVOKE_URL" -H 'Content-Type: application/x-www-form-urlencoded' --data-urlencode "token=$t" >/dev/null || { log 'remote revoke failed; retaining local file'; return 1; }
  rm -f "$TOKENS"; log 'token revoked'
}
status(){
  [[ -f "$TOKENS" ]] || { "$JQ" -n '{status:"NO_TOKENS",claim_allowed:false}'; return; }
  local e n; e=$("$JQ" -er '.expiry|numbers' "$TOKENS"); n=$(date +%s)
  "$JQ" -n --arg s "$((e-n>60))" --argjson expiry "$e" --argjson remaining "$((e-n))" '{status:(if $s=="1" then "VALID" else "EXPIRED_OR_EXPIRING" end),expiry:$expiry,seconds_remaining:$remaining,token_redacted:true,claim_allowed:false}'
}
selftest(){
  local old="$CFG" tmp pair v c expected got; tmp=$(mktemp -d); CFG="$tmp"; TOKENS="$tmp/tokens.json"; CREDS="$tmp/credentials.json"; LOG="$tmp/auth.log"; init
  save_tokens '{"access_token":"SELFTEST_TOKEN","refresh_token":"SELFTEST_REFRESH","expires_in":3600}'
  pair=$(pkce); v=${pair%%$'\n'*}; c=${pair#*$'\n'}; expected=$("$PYTHON" - "$v" <<'PY'
import base64,hashlib,sys
print(base64.urlsafe_b64encode(hashlib.sha256(sys.argv[1].encode()).digest()).rstrip(b'=').decode())
PY
); [[ ${#v} -ge 43 && ${#v} -le 128 && "$c" == "$expected" ]]; got=$(token); [[ "$got" == SELFTEST_TOKEN ]]; "$JQ" -e '.expiry>0' "$TOKENS" >/dev/null
  rm -rf "$tmp"; CFG="$old"; printf '{"selftest":"PASS","pkce":"S256","claim_allowed":false}\n'
}
usage(){ cat <<'USAGE'
Usage: gdrive-auth.sh init CLIENT_ID [REDIRECT_URI] | token | refresh | revoke | status | selftest
Environment: GDRIVE_CLIENT_SECRET(optional), GDRIVE_SCOPE(default drive.file)
USAGE
}
main(){
  need "$CURL"; need "$JQ"; need "$PYTHON"; init
  case "${1:-help}" in
    init) local id="${2:-}" redirect="${3:-$REDIRECT_DEFAULT}" secret="${GDRIVE_CLIENT_SECRET:-}" scope="${GDRIVE_SCOPE:-$SCOPE_DEFAULT}" pair verifier challenge code; [[ -n "$id" ]] || { usage >&2; return 2; }; pair=$(pkce); verifier=${pair%%$'\n'*}; challenge=${pair#*$'\n'}; save_creds "$id" "$secret" "$redirect" "$scope"; code=$(auth_code "$id" "$redirect" "$scope" "$challenge"); exchange "$id" "$secret" "$redirect" "$code" "$verifier"; status;;
    token) token;; refresh) refresh; status;; revoke) revoke;; status) status;; selftest) selftest;; help|-h|--help) usage;; *) usage >&2; return 2;;
  esac
}
[[ "${BASH_SOURCE[0]}" != "$0" ]] || main "$@"
