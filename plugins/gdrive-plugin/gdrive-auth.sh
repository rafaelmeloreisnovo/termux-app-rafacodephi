#!/bin/bash

# Low-level Google Drive OAuth 2.0 Handler
# Pure bash implementation, no external dependencies
# Stores tokens in ~/.config/gdrive-plugin/

set -euo pipefail

# Configuration
GDRIVE_CONFIG_DIR="${HOME}/.config/gdrive-plugin"
TOKENS_FILE="${GDRIVE_CONFIG_DIR}/tokens.json"
LOG_FILE="${GDRIVE_CONFIG_DIR}/auth.log"

# Google OAuth endpoints
OAUTH_AUTH_URL="https://accounts.google.com/o/oauth2/v2/auth"
OAUTH_TOKEN_URL="https://oauth2.googleapis.com/token"
OAUTH_REVOKE_URL="https://oauth2.googleapis.com/revoke"

# Initialize directories
init_config() {
    mkdir -p "$GDRIVE_CONFIG_DIR"
    touch "$LOG_FILE"
}

log_msg() {
    local msg="$1"
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $msg" | tee -a "$LOG_FILE"
}

# Get authorization code via browser redirect
get_authorization_code() {
    local client_id="$1"
    local redirect_uri="$2"
    local scope="$3"

    log_msg "Initiating OAuth 2.0 flow..."

    # Generate authorization URL
    local auth_url="${OAUTH_AUTH_URL}?client_id=${client_id}&redirect_uri=${redirect_uri}&response_type=code&scope=${scope}&access_type=offline&prompt=consent"

    log_msg "Open this URL in your browser:"
    echo "$auth_url"

    # Wait for user to input authorization code
    echo ""
    read -p "Enter the authorization code from redirect URL: " auth_code

    if [ -z "$auth_code" ]; then
        log_msg "ERROR: No authorization code provided"
        return 1
    fi

    echo "$auth_code"
}

# Exchange authorization code for access token
exchange_code_for_token() {
    local client_id="$1"
    local client_secret="$2"
    local redirect_uri="$3"
    local auth_code="$4"

    log_msg "Exchanging authorization code for access token..."

    # Make token request
    local response=$(curl -s -X POST "$OAUTH_TOKEN_URL" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        -d "client_id=${client_id}" \
        -d "client_secret=${client_secret}" \
        -d "code=${auth_code}" \
        -d "redirect_uri=${redirect_uri}" \
        -d "grant_type=authorization_code")

    # Extract tokens using sed/grep (pure bash parsing)
    local access_token=$(echo "$response" | grep -o '"access_token":"[^"]*' | cut -d'"' -f4 | head -1)
    local refresh_token=$(echo "$response" | grep -o '"refresh_token":"[^"]*' | cut -d'"' -f4 | head -1)
    local expires_in=$(echo "$response" | grep -o '"expires_in":[0-9]*' | cut -d':' -f2)

    if [ -z "$access_token" ]; then
        log_msg "ERROR: Failed to get access token"
        log_msg "Response: $response"
        return 1
    fi

    # Save tokens
    save_tokens "$access_token" "$refresh_token" "$expires_in"
    log_msg "Tokens saved successfully"

    echo "$access_token"
}

# Save tokens to file
save_tokens() {
    local access_token="$1"
    local refresh_token="$2"
    local expires_in="$3"

    local expiry=$(($(date +%s) + ${expires_in:-3600}))

    # Create JSON token file (pure bash)
    cat > "$TOKENS_FILE" << EOF
{
  "access_token": "$access_token",
  "refresh_token": "$refresh_token",
  "expires_in": $expires_in,
  "expiry": $expiry,
  "created_at": "$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
}
EOF

    chmod 600 "$TOKENS_FILE"
}

# Load tokens from file
load_tokens() {
    if [ ! -f "$TOKENS_FILE" ]; then
        log_msg "ERROR: No tokens found. Run 'gdrive-auth.sh init' first"
        return 1
    fi

    # Parse JSON using grep/sed
    local access_token=$(grep -o '"access_token":"[^"]*' "$TOKENS_FILE" | cut -d'"' -f4)
    local refresh_token=$(grep -o '"refresh_token":"[^"]*' "$TOKENS_FILE" | cut -d'"' -f4)
    local expiry=$(grep -o '"expiry":[0-9]*' "$TOKENS_FILE" | cut -d':' -f2)

    # Check if token is expired
    local now=$(date +%s)
    if [ "$expiry" -lt "$now" ]; then
        log_msg "Access token expired, refreshing..."
        refresh_access_token "$refresh_token" || return 1
        access_token=$(grep -o '"access_token":"[^"]*' "$TOKENS_FILE" | cut -d'"' -f4)
    fi

    echo "$access_token"
}

# Refresh access token using refresh token
refresh_access_token() {
    local refresh_token="$1"
    local client_id="$2"
    local client_secret="$3"

    log_msg "Refreshing access token..."

    local response=$(curl -s -X POST "$OAUTH_TOKEN_URL" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        -d "client_id=${client_id}" \
        -d "client_secret=${client_secret}" \
        -d "refresh_token=${refresh_token}" \
        -d "grant_type=refresh_token")

    local new_access_token=$(echo "$response" | grep -o '"access_token":"[^"]*' | cut -d'"' -f4 | head -1)
    local expires_in=$(echo "$response" | grep -o '"expires_in":[0-9]*' | cut -d':' -f2)

    if [ -z "$new_access_token" ]; then
        log_msg "ERROR: Failed to refresh token"
        log_msg "Response: $response"
        return 1
    fi

    # Update tokens file with new access token
    local old_refresh=$(grep -o '"refresh_token":"[^"]*' "$TOKENS_FILE" | cut -d'"' -f4)
    save_tokens "$new_access_token" "$old_refresh" "$expires_in"

    log_msg "Token refreshed successfully"
    return 0
}

# Get current access token (refresh if needed)
get_access_token() {
    load_tokens
}

# Revoke tokens (logout)
revoke_tokens() {
    local access_token=$(grep -o '"access_token":"[^"]*' "$TOKENS_FILE" | cut -d'"' -f4)

    if [ -z "$access_token" ]; then
        log_msg "No tokens to revoke"
        return 1
    fi

    log_msg "Revoking tokens..."

    curl -s -X POST "$OAUTH_REVOKE_URL" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        -d "token=${access_token}" > /dev/null

    rm -f "$TOKENS_FILE"
    log_msg "Tokens revoked and deleted"
}

# Main command dispatcher
main() {
    local command="${1:-help}"

    init_config

    case "$command" in
        init)
            # Initialize OAuth flow
            local client_id="${2:-}"
            local client_secret="${3:-}"
            local redirect_uri="${4:-http://localhost:8080}"

            if [ -z "$client_id" ] || [ -z "$client_secret" ]; then
                echo "Usage: $0 init <CLIENT_ID> <CLIENT_SECRET> [REDIRECT_URI]"
                return 1
            fi

            local scope="https://www.googleapis.com/auth/drive"
            local auth_code=$(get_authorization_code "$client_id" "$redirect_uri" "$scope")
            exchange_code_for_token "$client_id" "$client_secret" "$redirect_uri" "$auth_code"
            ;;

        token)
            # Get current access token
            get_access_token
            ;;

        refresh)
            # Manually refresh token
            local client_id="${2:-}"
            local client_secret="${3:-}"

            if [ -z "$client_id" ] || [ -z "$client_secret" ]; then
                echo "Usage: $0 refresh <CLIENT_ID> <CLIENT_SECRET>"
                return 1
            fi

            local refresh_token=$(grep -o '"refresh_token":"[^"]*' "$TOKENS_FILE" | cut -d'"' -f4)
            refresh_access_token "$refresh_token" "$client_id" "$client_secret"
            ;;

        revoke)
            # Logout and delete tokens
            revoke_tokens
            ;;

        status)
            # Show token status
            if [ -f "$TOKENS_FILE" ]; then
                local expiry=$(grep -o '"expiry":[0-9]*' "$TOKENS_FILE" | cut -d':' -f2)
                local now=$(date +%s)
                local expires_in=$((expiry - now))

                echo "Tokens file: $TOKENS_FILE"
                echo "Status: OK"
                echo "Expires in: $expires_in seconds"
            else
                echo "Status: NO TOKENS"
                echo "Run '$0 init' to authenticate"
            fi
            ;;

        help|*)
            cat << EOF
Google Drive OAuth 2.0 Handler

Usage: $0 <command> [options]

Commands:
  init <CLIENT_ID> <CLIENT_SECRET> [REDIRECT_URI]
      Initialize OAuth flow and get tokens

  token
      Get current access token (refresh if needed)

  refresh <CLIENT_ID> <CLIENT_SECRET>
      Manually refresh access token

  revoke
      Revoke tokens and logout

  status
      Show token status and expiry

  help
      Show this help message

Config Directory: $GDRIVE_CONFIG_DIR
Tokens File: $TOKENS_FILE
Log File: $LOG_FILE

EOF
            ;;
    esac
}

main "$@"
