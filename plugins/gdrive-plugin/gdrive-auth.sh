#!/usr/bin/env bash
# Google Drive OAuth 2.0 handler for Termux RAFCODEΦ.
# Shell orchestrator with jq-based JSON handling and fail-closed HTTP checks.

set -euo pipefail
umask 077

CURL_BIN="${GDRIVE_CURL_BIN:-curl}"
JQ_BIN="${GDRIVE_JQ_BIN:-jq}"
GDRIVE_CONFIG_DIR="${GDRIVE_CONFIG_DIR:-${HOME}/.config/gdrive-plugin}"
TOKENS_FILE="${GDRIVE_TOKENS_FILE:-${GDRIVE_CONFIG_DIR}/tokens.json}"
CREDENTIALS_FILE="${GDRIVE_CREDENTIALS_FILE:-${GDRIVE_CONFIG_DIR}/credentials.json}"
LOG_FILE="${GDRIVE_AUTH_LOG_FILE:-${GDRIVE_CONFIG_DIR}/auth.log}"
OAUTH_AUTH_URL="https://accounts.google.com/o/oauth2/v2/auth"
OAUTH_TOKEN_URL="https://oauth2.googleapis.com/token"
OAUTH_REVOKE_URL="https://oauth2.googleapis.com/revoke"
DEFAULT_REDIRECT_URI="http://127.0.0.1:53682/"
DEFAULT_SCOPE="https://www.googleapis.com/auth/drive.file"
TOKEN_SKEW_SECONDS=60

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        printf 'ERROR: required command not found: %s\n' "$1" >&2
        return 1
    }
}

init_config() {
    mkdir -p "$GDRIVE_CONFIG_DIR"
    chmod 700 "$GDRIVE_CONFIG_DIR"
    touch "$LOG_FILE"
    chmod 600 "$LOG_FILE"
}

log_msg() {
    local msg="$1"
    local line
    line="[$(date -u +'%Y-%m-%dT%H:%M:%SZ')] $msg"
    printf '%s\n' "$line" >&2
    printf '%s\n' "$line" >> "$LOG_FILE"
}

urlencode() {
    "$JQ_BIN" -nr --arg value "$1" '$value|@uri'
}

random_hex() {
    od -An -N16 -tx1 /dev/urandom | tr -d ' \n'
}

url_decode() {
    local value="${1//+/ }"
    printf '%b' "${value//%/\\x}"
}

http_json() {
    local body_file http_code rc
    body_file=$(mktemp "${TMPDIR:-/tmp}/gdrive-http.XXXXXX")
    rc=0
    http_code=$("$CURL_BIN" -sS -o "$body_file" -w '%{http_code}' "$@") || rc=$?
    if (( rc != 0 )); then
        log_msg "HTTP transport failed (curl_rc=$rc)"
        cat "$body_file" >&2 || true
        rm -f "$body_file"
        return "$rc"
    fi
    if [[ ! "$http_code" =~ ^2[0-9][0-9]$ ]]; then
        log_msg "HTTP request rejected (status=$http_code)"
        cat "$body_file" >&2 || true
        rm -f "$body_file"
        return 22
    fi
    cat "$body_file"
    rm -f "$body_file"
}

save_credentials() {
    local client_id="$1" client_secret="$2" redirect_uri="$3" scope="$4"
    local tmp
    tmp=$(mktemp "${CREDENTIALS_FILE}.tmp.XXXXXX")
    "$JQ_BIN" -n \
        --arg client_id "$client_id" \
        --arg client_secret "$client_secret" \
        --arg redirect_uri "$redirect_uri" \
        --arg scope "$scope" \
        '{schema:"gdrive-plugin-credentials/v2",client_id:$client_id,client_secret:$client_secret,redirect_uri:$redirect_uri,scope:$scope}' \
        > "$tmp"
    chmod 600 "$tmp"
    mv -f "$tmp" "$CREDENTIALS_FILE"
}

save_tokens_from_response() {
    local response="$1" previous_refresh="${2:-}"
    local access_token refresh_token expires_in expiry created_at tmp

    access_token=$("$JQ_BIN" -er '.access_token | strings | select(length>0)' <<<"$response") || {
        log_msg "Token response has no access_token"
        "$JQ_BIN" -c '{error,error_description}' <<<"$response" >&2 || true
        return 1
    }
    refresh_token=$("$JQ_BIN" -r '.refresh_token // empty' <<<"$response")
    [[ -n "$refresh_token" ]] || refresh_token="$previous_refresh"
    expires_in=$("$JQ_BIN" -er '.expires_in | numbers' <<<"$response") || expires_in=3600
    expiry=$(( $(date +%s) + expires_in ))
    created_at=$(date -u +'%Y-%m-%dT%H:%M:%SZ')

    tmp=$(mktemp "${TOKENS_FILE}.tmp.XXXXXX")
    "$JQ_BIN" -n \
        --arg access_token "$access_token" \
        --arg refresh_token "$refresh_token" \
        --argjson expires_in "$expires_in" \
        --argjson expiry "$expiry" \
        --arg created_at "$created_at" \
        '{schema:"gdrive-plugin-tokens/v2",access_token:$access_token,refresh_token:$refresh_token,expires_in:$expires_in,expiry:$expiry,created_at:$created_at}' \
        > "$tmp"
    chmod 600 "$tmp"
    mv -f "$tmp" "$TOKENS_FILE"
}

build_authorization_url() {
    local client_id="$1" redirect_uri="$2" scope="$3" state="$4"
    printf '%s?client_id=%s&redirect_uri=%s&response_type=code&scope=%s&access_type=offline&prompt=consent&state=%s' \
        "$OAUTH_AUTH_URL" \
        "$(urlencode "$client_id")" \
        "$(urlencode "$redirect_uri")" \
        "$(urlencode "$scope")" \
        "$(urlencode "$state")"
}

get_authorization_code() {
    local client_id="$1" redirect_uri="$2" scope="$3"
    local state auth_url redirected pasted_state raw_code code
    state=$(random_hex)
    auth_url=$(build_authorization_url "$client_id" "$redirect_uri" "$scope" "$state")

    log_msg "OAuth authorization started; requested scope: $scope"
    printf '\nOpen this URL in a browser:\n%s\n\n' "$auth_url" >&2
    if command -v termux-open-url >/dev/null 2>&1; then
        termux-open-url "$auth_url" >/dev/null 2>&1 || true
    fi
    printf 'After Google redirects to 127.0.0.1, paste the COMPLETE redirected URL here:\n> ' >&2
    IFS= read -r redirected

    [[ "$redirected" == *"code="* ]] || {
        log_msg "Redirect URL does not contain an authorization code"
        return 1
    }
    [[ "$redirected" == *"state="* ]] || {
        log_msg "Redirect URL does not contain OAuth state"
        return 1
    }

    pasted_state="${redirected#*state=}"
    pasted_state="${pasted_state%%&*}"
    pasted_state=$(url_decode "$pasted_state")
    [[ "$pasted_state" == "$state" ]] || {
        log_msg "OAuth state mismatch; refusing authorization code"
        return 1
    }

    raw_code="${redirected#*code=}"
    raw_code="${raw_code%%&*}"
    code=$(url_decode "$raw_code")
    [[ -n "$code" ]] || {
        log_msg "Authorization code is empty"
        return 1
    }
    printf '%s\n' "$code"
}

exchange_code_for_token() {
    local client_id="$1" client_secret="$2" redirect_uri="$3" auth_code="$4"
    local response
    response=$(http_json -X POST "$OAUTH_TOKEN_URL" \
        -H 'Content-Type: application/x-www-form-urlencoded' \
        --data-urlencode "client_id=$client_id" \
        --data-urlencode "client_secret=$client_secret" \
        --data-urlencode "code=$auth_code" \
        --data-urlencode "redirect_uri=$redirect_uri" \
        --data-urlencode 'grant_type=authorization_code')
    save_tokens_from_response "$response"
    log_msg "OAuth tokens stored with mode 600"
}

refresh_access_token() {
    [[ -f "$CREDENTIALS_FILE" ]] || {
        log_msg "Credentials missing; run init again"
        return 1
    }
    [[ -f "$TOKENS_FILE" ]] || {
        log_msg "Tokens missing; run init first"
        return 1
    }

    local client_id client_secret refresh_token response
    client_id=$("$JQ_BIN" -er '.client_id' "$CREDENTIALS_FILE")
    client_secret=$("$JQ_BIN" -er '.client_secret' "$CREDENTIALS_FILE")
    refresh_token=$("$JQ_BIN" -er '.refresh_token | strings | select(length>0)' "$TOKENS_FILE") || {
        log_msg "No refresh token is available"
        return 1
    }

    response=$(http_json -X POST "$OAUTH_TOKEN_URL" \
        -H 'Content-Type: application/x-www-form-urlencoded' \
        --data-urlencode "client_id=$client_id" \
        --data-urlencode "client_secret=$client_secret" \
        --data-urlencode "refresh_token=$refresh_token" \
        --data-urlencode 'grant_type=refresh_token')
    save_tokens_from_response "$response" "$refresh_token"
    log_msg "Access token refreshed"
}

get_access_token() {
    [[ -f "$TOKENS_FILE" ]] || {
        log_msg "No tokens found; run init first"
        return 1
    }
    "$JQ_BIN" -e . "$TOKENS_FILE" >/dev/null

    local expiry now
    expiry=$("$JQ_BIN" -er '.expiry | numbers' "$TOKENS_FILE")
    now=$(date +%s)
    if (( expiry <= now + TOKEN_SKEW_SECONDS )); then
        refresh_access_token
    fi
    "$JQ_BIN" -er '.access_token | strings | select(length>0)' "$TOKENS_FILE"
}

revoke_tokens() {
    [[ -f "$TOKENS_FILE" ]] || {
        log_msg "No tokens to revoke"
        return 0
    }
    local token
    token=$("$JQ_BIN" -er '.access_token' "$TOKENS_FILE")
    http_json -X POST "$OAUTH_REVOKE_URL" \
        -H 'Content-Type: application/x-www-form-urlencoded' \
        --data-urlencode "token=$token" >/dev/null || {
            log_msg "Remote revocation failed; local tokens retained"
            return 1
        }
    rm -f "$TOKENS_FILE"
    log_msg "Remote token revoked and local token file removed"
}

show_status() {
    if [[ ! -f "$TOKENS_FILE" ]]; then
        "$JQ_BIN" -n '{status:"NO_TOKENS",claim_allowed:false}'
        return 0
    fi
    local expiry now remaining
    expiry=$("$JQ_BIN" -er '.expiry | numbers' "$TOKENS_FILE")
    now=$(date +%s)
    remaining=$(( expiry - now ))
    "$JQ_BIN" -n \
        --arg status "$([[ $remaining -gt 60 ]] && printf VALID || printf EXPIRED_OR_EXPIRING)" \
        --argjson expiry "$expiry" \
        --argjson remaining "$remaining" \
        '{status:$status,expiry:$expiry,seconds_remaining:$remaining,token_redacted:true,claim_allowed:false}'
}

selftest() {
    local old_dir="$GDRIVE_CONFIG_DIR" tmp_dir token
    tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/gdrive-auth-selftest.XXXXXX")
    GDRIVE_CONFIG_DIR="$tmp_dir"
    TOKENS_FILE="$tmp_dir/tokens.json"
    CREDENTIALS_FILE="$tmp_dir/credentials.json"
    LOG_FILE="$tmp_dir/auth.log"
    init_config
    save_tokens_from_response '{"access_token":"SELFTEST_TOKEN","refresh_token":"SELFTEST_REFRESH","expires_in":3600}'
    token=$(get_access_token)
    [[ "$token" == "SELFTEST_TOKEN" ]]
    "$JQ_BIN" -e '.access_token=="SELFTEST_TOKEN" and .expiry>0' "$TOKENS_FILE" >/dev/null
    rm -rf "$tmp_dir"
    GDRIVE_CONFIG_DIR="$old_dir"
    printf '{"selftest":"PASS","claim_allowed":false}\n'
}

usage() {
    cat <<EOF_USAGE
Google Drive OAuth 2.0 Handler

Usage:
  $0 init <CLIENT_ID> [REDIRECT_URI]
  $0 token
  $0 refresh
  $0 revoke
  $0 status
  $0 selftest

Environment:
  GDRIVE_CLIENT_SECRET  OAuth desktop client secret; prompted securely if absent
  GDRIVE_SCOPE          Defaults to drive.file; set to .../auth/drive only for explicit full-Drive access

Security boundary:
  - logs go to stderr and never contain access/refresh tokens;
  - tokens and credentials are stored with mode 600;
  - OAuth state is verified;
  - the complete loopback redirect URL is required.
EOF_USAGE
}

main() {
    require_cmd "$CURL_BIN"
    require_cmd "$JQ_BIN"
    init_config

    local command="${1:-help}"
    case "$command" in
        init)
            local client_id="${2:-}" redirect_uri="${3:-$DEFAULT_REDIRECT_URI}"
            local client_secret="${GDRIVE_CLIENT_SECRET:-}" scope="${GDRIVE_SCOPE:-$DEFAULT_SCOPE}" auth_code
            [[ -n "$client_id" ]] || { usage >&2; return 2; }
            if [[ -z "$client_secret" ]]; then
                printf 'OAuth desktop client secret: ' >&2
                IFS= read -r -s client_secret
                printf '\n' >&2
            fi
            [[ -n "$client_secret" ]] || { log_msg "Client secret is empty"; return 2; }
            save_credentials "$client_id" "$client_secret" "$redirect_uri" "$scope"
            auth_code=$(get_authorization_code "$client_id" "$redirect_uri" "$scope")
            exchange_code_for_token "$client_id" "$client_secret" "$redirect_uri" "$auth_code"
            show_status
            ;;
        token) get_access_token ;;
        refresh) refresh_access_token; show_status ;;
        revoke) revoke_tokens ;;
        status) show_status ;;
        selftest) selftest ;;
        help|-h|--help) usage ;;
        *) usage >&2; return 2 ;;
    esac
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    main "$@"
fi
