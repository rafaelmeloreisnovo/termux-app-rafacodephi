#!/bin/bash

# Low-level Google Drive Sync Engine
# Bidirectional mirror sync with hash tracking (shadow files)
# Pure bash implementation using curl and sed/grep

set -euo pipefail

# Configuration
GDRIVE_CONFIG_DIR="${HOME}/.config/gdrive-plugin"
TOKENS_FILE="${GDRIVE_CONFIG_DIR}/tokens.json"
SHADOW_DIR="${GDRIVE_CONFIG_DIR}/.gdrive-shadow"
STATUS_FILE="${GDRIVE_CONFIG_DIR}/sync.status"
LOG_FILE="${GDRIVE_CONFIG_DIR}/sync.log"
IGNORE_FILE="${GDRIVE_CONFIG_DIR}/.gdrive-ignore"

# Google Drive API
DRIVE_API="https://www.googleapis.com/drive/v3"

# Initialize
init_sync() {
    mkdir -p "$GDRIVE_CONFIG_DIR" "$SHADOW_DIR"
    touch "$LOG_FILE" "$IGNORE_FILE" "$STATUS_FILE"
}

log_msg() {
    local msg="$1"
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $msg" | tee -a "$LOG_FILE"
}

# Get access token
get_token() {
    if [ ! -f "$TOKENS_FILE" ]; then
        log_msg "ERROR: No tokens found. Run gdrive-auth.sh first"
        return 1
    fi

    local access_token=$(grep -o '"access_token":"[^"]*' "$TOKENS_FILE" | cut -d'"' -f4)
    if [ -z "$access_token" ]; then
        log_msg "ERROR: Failed to parse access token"
        return 1
    fi

    echo "$access_token"
}

# Calculate file hash (MD5)
calc_hash() {
    local file="$1"
    if [ ! -f "$file" ]; then
        echo ""
        return 1
    fi

    # Use md5sum if available, fallback to sha256sum
    if command -v md5sum &> /dev/null; then
        md5sum "$file" | awk '{print $1}'
    elif command -v sha256sum &> /dev/null; then
        sha256sum "$file" | awk '{print $1}'
    else
        log_msg "WARNING: No hash command available, using file size"
        stat -f%z "$file" 2>/dev/null || stat -c%s "$file"
    fi
}

# Get file info from Drive API
get_drive_file_info() {
    local file_id="$1"
    local token="$2"

    curl -s \
        -H "Authorization: Bearer $token" \
        "$DRIVE_API/files/$file_id?fields=id,name,md5Checksum,size,modifiedTime,trashed"
}

# List files from Drive
list_drive_files() {
    local folder_id="$1"
    local token="$2"
    local page_token="${3:-}"

    local url="$DRIVE_API/files?q=trashed=false%20and%20'$folder_id'%20in%20parents&fields=files(id,name,md5Checksum,size,modifiedTime,mimeType)&pageSize=1000"

    if [ -n "$page_token" ]; then
        url="${url}&pageToken=${page_token}"
    fi

    curl -s \
        -H "Authorization: Bearer $token" \
        "$url"
}

# Upload file to Drive
upload_file() {
    local local_file="$1"
    local remote_name="$2"
    local parent_id="$3"
    local token="$4"

    if [ ! -f "$local_file" ]; then
        log_msg "ERROR: File not found: $local_file"
        return 1
    fi

    log_msg "Uploading: $remote_name"

    # Create metadata JSON
    local metadata="{\"name\":\"$remote_name\",\"parents\":[\"$parent_id\"]}"

    # Upload file
    local response=$(curl -s \
        -H "Authorization: Bearer $token" \
        -F "metadata=<-;type=application/json" \
        -F "file=@$local_file" \
        "$DRIVE_API/files?uploadType=multipart&fields=id,md5Checksum")

    local file_id=$(echo "$response" | grep -o '"id":"[^"]*' | head -1 | cut -d'"' -f4)

    if [ -z "$file_id" ]; then
        log_msg "ERROR: Upload failed for $remote_name"
        return 1
    fi

    log_msg "SUCCESS: Uploaded $remote_name (ID: $file_id)"

    # Save hash to shadow
    local hash=$(calc_hash "$local_file")
    save_shadow_hash "$local_file" "$hash" "$file_id"

    echo "$file_id"
}

# Download file from Drive
download_file() {
    local file_id="$1"
    local remote_name="$2"
    local local_path="$3"
    local token="$4"

    log_msg "Downloading: $remote_name to $local_path"

    # Create directory if needed
    mkdir -p "$(dirname "$local_path")"

    # Download file
    curl -s -L -o "$local_path" \
        -H "Authorization: Bearer $token" \
        "$DRIVE_API/files/$file_id?alt=media"

    if [ ! -f "$local_path" ]; then
        log_msg "ERROR: Download failed for $remote_name"
        return 1
    fi

    log_msg "SUCCESS: Downloaded $remote_name"

    # Save hash to shadow
    local hash=$(calc_hash "$local_path")
    save_shadow_hash "$local_path" "$hash" "$file_id"

    echo "OK"
}

# Delete file from Drive
delete_drive_file() {
    local file_id="$1"
    local file_name="$2"
    local token="$3"

    log_msg "Deleting from Drive: $file_name (ID: $file_id)"

    curl -s -X DELETE \
        -H "Authorization: Bearer $token" \
        "$DRIVE_API/files/$file_id" > /dev/null

    log_msg "SUCCESS: Deleted $file_name from Drive"
}

# Save file hash to shadow directory
save_shadow_hash() {
    local file_path="$1"
    local hash="$2"
    local drive_id="$3"

    local shadow_name=$(echo "$file_path" | sed 's|/|_|g').shadow
    local shadow_file="$SHADOW_DIR/$shadow_name"

    cat > "$shadow_file" << EOF
{
  "path": "$file_path",
  "hash": "$hash",
  "drive_id": "$drive_id",
  "synced_at": "$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
}
EOF
}

# Load hash from shadow
load_shadow_hash() {
    local file_path="$1"
    local shadow_name=$(echo "$file_path" | sed 's|/|_|g').shadow
    local shadow_file="$SHADOW_DIR/$shadow_name"

    if [ -f "$shadow_file" ]; then
        grep -o '"hash":"[^"]*' "$shadow_file" | cut -d'"' -f4
    else
        echo ""
    fi
}

# Check if file should be ignored
is_ignored() {
    local file="$1"

    # Simple pattern matching against ignore file
    while IFS= read -r pattern; do
        [ -z "$pattern" ] && continue
        [ "${pattern:0:1}" = "#" ] && continue

        if [[ "$file" == *"$pattern"* ]]; then
            return 0
        fi
    done < "$IGNORE_FILE"

    return 1
}

# Sync directory: upload new/changed files
sync_upload() {
    local local_dir="$1"
    local remote_folder_id="$2"
    local token="$3"

    if [ ! -d "$local_dir" ]; then
        log_msg "ERROR: Directory not found: $local_dir"
        return 1
    fi

    log_msg "Starting upload sync from: $local_dir"

    # Find all files in directory
    find "$local_dir" -type f | while read -r file; do
        if is_ignored "$file"; then
            log_msg "IGNORED: $file"
            continue
        fi

        local basename=$(basename "$file")
        local old_hash=$(load_shadow_hash "$file")
        local new_hash=$(calc_hash "$file")

        if [ "$old_hash" != "$new_hash" ]; then
            upload_file "$file" "$basename" "$remote_folder_id" "$token"
        else
            log_msg "UNCHANGED: $file"
        fi
    done
}

# Sync directory: download new files
sync_download() {
    local local_dir="$1"
    local remote_folder_id="$2"
    local token="$3"

    log_msg "Starting download sync to: $local_dir"

    mkdir -p "$local_dir"

    # List files in Drive folder
    local response=$(list_drive_files "$remote_folder_id" "$token")

    # Parse files from JSON (pure bash)
    echo "$response" | grep -o '"id":"[^"]*\|"name":"[^"]*' | \
    sed 's/"id":"\|"name":"\|"//g' | \
    paste -d '|' - - | while IFS='|' read -r file_id file_name; do
        if [ -z "$file_id" ] || [ -z "$file_name" ]; then
            continue
        fi

        local local_file="$local_dir/$file_name"

        if is_ignored "$local_file"; then
            log_msg "IGNORED: $file_name"
            continue
        fi

        if [ -f "$local_file" ]; then
            # File exists locally, check hash
            local old_hash=$(calc_hash "$local_file")
            local drive_info=$(get_drive_file_info "$file_id" "$token")
            local drive_hash=$(echo "$drive_info" | grep -o '"md5Checksum":"[^"]*' | cut -d'"' -f4)

            if [ "$old_hash" = "$drive_hash" ]; then
                log_msg "UNCHANGED: $file_name"
                continue
            fi
        fi

        download_file "$file_id" "$file_name" "$local_file" "$token"
    done
}

# Mirror sync: bidirectional
mirror_sync() {
    local local_dir="$1"
    local remote_folder_id="$2"

    local token=$(get_token) || return 1

    log_msg "Starting mirror sync: $local_dir <-> Drive:$remote_folder_id"

    # Update status
    echo "status=running" > "$STATUS_FILE"
    echo "started=$(date -u +'%Y-%m-%dT%H:%M:%SZ')" >> "$STATUS_FILE"

    # Upload changes
    sync_upload "$local_dir" "$remote_folder_id" "$token" || true

    # Download changes
    sync_download "$local_dir" "$remote_folder_id" "$token" || true

    # Update status
    echo "status=success" > "$STATUS_FILE"
    echo "completed=$(date -u +'%Y-%m-%dT%H:%M:%SZ')" >> "$STATUS_FILE"

    log_msg "Mirror sync completed"
}

# Show sync status
show_status() {
    if [ -f "$STATUS_FILE" ]; then
        cat "$STATUS_FILE"
    else
        echo "status=never_run"
    fi
}

# Main command dispatcher
main() {
    local command="${1:-help}"

    init_sync

    case "$command" in
        upload)
            # Upload directory to Drive
            local local_dir="${2:-.}"
            local remote_id="${3:-}"

            if [ -z "$remote_id" ]; then
                echo "Usage: $0 upload <local_dir> <drive_folder_id>"
                return 1
            fi

            sync_upload "$local_dir" "$remote_id" "$(get_token)"
            ;;

        download)
            # Download from Drive to directory
            local remote_id="${2:-}"
            local local_dir="${3:-.}"

            if [ -z "$remote_id" ]; then
                echo "Usage: $0 download <drive_folder_id> [local_dir]"
                return 1
            fi

            sync_download "$local_dir" "$remote_id" "$(get_token)"
            ;;

        mirror)
            # Bidirectional sync
            local local_dir="${2:-.}"
            local remote_id="${3:-}"

            if [ -z "$remote_id" ]; then
                echo "Usage: $0 mirror <local_dir> <drive_folder_id>"
                return 1
            fi

            mirror_sync "$local_dir" "$remote_id"
            ;;

        status)
            # Show sync status
            show_status
            ;;

        help|*)
            cat << EOF
Google Drive Sync Engine

Usage: $0 <command> [options]

Commands:
  upload <local_dir> <drive_folder_id>
      Upload local directory to Google Drive

  download <drive_folder_id> [local_dir]
      Download from Google Drive to local directory

  mirror <local_dir> <drive_folder_id>
      Bidirectional sync (mirror mode)

  status
      Show last sync status

  help
      Show this help message

Config: $GDRIVE_CONFIG_DIR
Tokens: $TOKENS_FILE
Shadow: $SHADOW_DIR
Log: $LOG_FILE
Ignore: $IGNORE_FILE

EOF
            ;;
    esac
}

main "$@"
