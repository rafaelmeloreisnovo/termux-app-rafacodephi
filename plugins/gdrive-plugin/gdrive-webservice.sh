#!/bin/bash

# Low-level Google Drive Webservice
# Simple HTTP server for Chrome/browser access
# Pure bash using socat or nc (netcat)

set -euo pipefail

# Configuration
GDRIVE_CONFIG_DIR="${HOME}/.config/gdrive-plugin"
PORT="${GDRIVE_PORT:-8080}"
LOG_FILE="${GDRIVE_CONFIG_DIR}/webservice.log"
STATUS_FILE="${GDRIVE_CONFIG_DIR}/sync.status"

# Colors for terminal output
C_GREEN='\033[0;32m'
C_BLUE='\033[0;34m'
C_RED='\033[0;31m'
C_NC='\033[0m' # No Color

# Initialize
init_webservice() {
    mkdir -p "$GDRIVE_CONFIG_DIR"
    touch "$LOG_FILE" "$STATUS_FILE"
}

log_msg() {
    local msg="$1"
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $msg" | tee -a "$LOG_FILE"
}

# Check dependencies
check_deps() {
    if ! command -v socat &> /dev/null && ! command -v nc &> /dev/null; then
        echo -e "${C_RED}ERROR: Neither socat nor nc found${C_NC}"
        return 1
    fi
    return 0
}

# Parse HTTP request
parse_http_request() {
    local request="$1"

    # Extract method, path, and headers using grep/sed
    local method=$(echo "$request" | head -1 | awk '{print $1}')
    local path=$(echo "$request" | head -1 | awk '{print $2}')
    local content_length=$(echo "$request" | grep -i "content-length" | awk -F': ' '{print $2}' | tr -d '\r')

    echo "METHOD=$method"
    echo "PATH=$path"
    echo "CONTENT_LENGTH=$content_length"
}

# Generate HTML response
generate_html_page() {
    cat << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Google Drive Sync</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; padding: 20px; }
        .header { background: #1f2937; color: white; padding: 20px; border-radius: 8px; margin-bottom: 20px; }
        .header h1 { font-size: 28px; margin-bottom: 10px; }
        .header p { opacity: 0.8; }
        .card { background: white; border-radius: 8px; padding: 20px; margin-bottom: 20px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
        .button { background: #3b82f6; color: white; border: none; padding: 10px 20px; border-radius: 6px; cursor: pointer; font-size: 14px; margin-right: 10px; margin-bottom: 10px; }
        .button:hover { background: #2563eb; }
        .button.danger { background: #ef4444; }
        .button.danger:hover { background: #dc2626; }
        .status { padding: 10px; border-radius: 6px; margin-top: 10px; }
        .status.success { background: #d1fae5; color: #065f46; }
        .status.error { background: #fee2e2; color: #991b1b; }
        .status.running { background: #dbeafe; color: #0c4a6e; }
        .input-group { margin-bottom: 15px; }
        .input-group label { display: block; margin-bottom: 5px; font-weight: 500; }
        .input-group input { width: 100%; padding: 8px; border: 1px solid #d1d5db; border-radius: 4px; font-size: 14px; }
        .logs { background: #1f2937; color: #10b981; padding: 15px; border-radius: 6px; font-family: monospace; font-size: 12px; max-height: 300px; overflow-y: auto; }
        .log-line { margin-bottom: 5px; }
        .separator { border-top: 1px solid #e5e7eb; margin: 20px 0; }
        @media (prefers-color-scheme: dark) {
            body { background: #1f2937; }
            .card { background: #374151; color: white; }
            .input-group input { background: #4b5563; color: white; border-color: #6b7280; }
            .logs { background: #000; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚀 Google Drive Sync</h1>
            <p>Termux RAFCODEΦ Plugin</p>
        </div>

        <div class="card">
            <h2>Sync Operations</h2>
            <div>
                <button class="button" onclick="startMirrorSync()">🔄 Mirror Sync</button>
                <button class="button" onclick="startUpload()">⬆️ Upload</button>
                <button class="button" onclick="startDownload()">⬇️ Download</button>
            </div>
            <div id="sync-status" class="status" style="display:none;"></div>
        </div>

        <div class="card">
            <h2>Configuration</h2>
            <div class="input-group">
                <label>Local Directory:</label>
                <input type="text" id="local-dir" value="/data/data/com.termux.rafacodephi/files/home/storage/downloads" placeholder="/path/to/directory">
            </div>
            <div class="input-group">
                <label>Drive Folder ID:</label>
                <input type="text" id="drive-folder-id" placeholder="Paste Google Drive folder ID">
            </div>
            <button class="button" onclick="saveConfig()">💾 Save Configuration</button>
        </div>

        <div class="card">
            <h2>Status</h2>
            <div id="status-info" style="font-family: monospace; white-space: pre-wrap;">
                Loading status...
            </div>
            <button class="button danger" onclick="refreshStatus()">🔄 Refresh</button>
        </div>

        <div class="card">
            <h2>Recent Logs</h2>
            <div class="logs" id="logs">
                <div class="log-line">Initializing...</div>
            </div>
            <button class="button" onclick="clearLogs()" style="margin-top: 10px;">🗑️ Clear Logs</button>
        </div>
    </div>

    <script>
        function startMirrorSync() {
            const localDir = document.getElementById('local-dir').value;
            const driveId = document.getElementById('drive-folder-id').value;

            if (!localDir || !driveId) {
                alert('Please configure both local directory and Drive folder ID');
                return;
            }

            const statusDiv = document.getElementById('sync-status');
            statusDiv.className = 'status running';
            statusDiv.textContent = '⏳ Sync in progress...';
            statusDiv.style.display = 'block';

            fetch('/api/sync/mirror', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ local_dir: localDir, drive_folder_id: driveId })
            }).then(r => r.json()).then(data => {
                statusDiv.className = data.status === 'success' ? 'status success' : 'status error';
                statusDiv.textContent = '✅ ' + data.message;
                refreshStatus();
            }).catch(e => {
                statusDiv.className = 'status error';
                statusDiv.textContent = '❌ Error: ' + e.message;
            });
        }

        function startUpload() {
            const localDir = document.getElementById('local-dir').value;
            const driveId = document.getElementById('drive-folder-id').value;

            if (!localDir || !driveId) {
                alert('Please configure both local directory and Drive folder ID');
                return;
            }

            fetch('/api/sync/upload', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ local_dir: localDir, drive_folder_id: driveId })
            }).then(r => r.json()).then(data => {
                alert(data.message);
                refreshStatus();
            });
        }

        function startDownload() {
            const localDir = document.getElementById('local-dir').value;
            const driveId = document.getElementById('drive-folder-id').value;

            if (!localDir || !driveId) {
                alert('Please configure both local directory and Drive folder ID');
                return;
            }

            fetch('/api/sync/download', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ local_dir: localDir, drive_folder_id: driveId })
            }).then(r => r.json()).then(data => {
                alert(data.message);
                refreshStatus();
            });
        }

        function saveConfig() {
            const localDir = document.getElementById('local-dir').value;
            const driveId = document.getElementById('drive-folder-id').value;

            fetch('/api/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ local_dir: localDir, drive_folder_id: driveId })
            }).then(r => r.json()).then(data => {
                alert('Configuration saved!');
            });
        }

        function refreshStatus() {
            fetch('/api/status').then(r => r.json()).then(data => {
                document.getElementById('status-info').textContent = JSON.stringify(data, null, 2);
            });
        }

        function clearLogs() {
            fetch('/api/logs/clear', { method: 'POST' }).then(r => r.json()).then(() => {
                document.getElementById('logs').innerHTML = '<div class="log-line">Logs cleared</div>';
            });
        }

        // Auto-refresh every 5 seconds
        setInterval(refreshStatus, 5000);
        refreshStatus();
    </script>
</body>
</html>
EOF
}

# HTTP Response helper
send_http_response() {
    local status_code="$1"
    local content_type="$2"
    local body="$3"

    local status_text="OK"
    [ "$status_code" != "200" ] && status_text="ERROR"

    cat << EOF
HTTP/1.1 $status_code $status_text
Content-Type: $content_type
Content-Length: ${#body}
Connection: close
Access-Control-Allow-Origin: *

$body
EOF
}

# API: Get status
api_status() {
    local status_file="$STATUS_FILE"
    local response="{\"status\":\"unknown\"}"

    if [ -f "$status_file" ]; then
        # Parse status file (simple key=value format)
        response="{"
        while IFS='=' read -r key value; do
            response="${response}\"${key}\":\"${value}\","
        done < "$status_file"
        response="${response%,}}"
    fi

    send_http_response "200" "application/json" "$response"
}

# API: Start sync
api_sync() {
    local method="$1"  # upload, download, mirror
    local local_dir="$2"
    local drive_folder_id="$3"

    log_msg "API: Starting $method sync"

    # Call gdrive-sync.sh (requires it to be in PATH or same directory)
    local sync_script="$(dirname "$0")/gdrive-sync.sh"

    if [ ! -x "$sync_script" ]; then
        send_http_response "404" "application/json" "{\"status\":\"error\",\"message\":\"Sync script not found\"}"
        return
    fi

    # Run sync in background
    "$sync_script" "$method" "$local_dir" "$drive_folder_id" &

    send_http_response "200" "application/json" "{\"status\":\"success\",\"message\":\"$method sync started\"}"
}

# API: Save configuration
api_config() {
    local local_dir="$1"
    local drive_folder_id="$2"

    local config_file="${GDRIVE_CONFIG_DIR}/config.json"

    cat > "$config_file" << EOF
{
  "local_dir": "$local_dir",
  "drive_folder_id": "$drive_folder_id",
  "updated_at": "$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
}
EOF

    send_http_response "200" "application/json" "{\"status\":\"success\",\"message\":\"Configuration saved\"}"
}

# API: Get logs
api_logs() {
    local response="{\"logs\":["

    if [ -f "$LOG_FILE" ]; then
        tail -50 "$LOG_FILE" | while read -r line; do
            # Escape quotes and newlines
            line=$(echo "$line" | sed 's/"/\\"/g' | sed "s/'/\\\\'/g")
            response="${response}\"${line}\","
        done
    fi

    response="${response%,}]}"

    send_http_response "200" "application/json" "$response"
}

# Main HTTP server loop
run_server() {
    echo -e "${C_GREEN}Starting Google Drive Webservice on port $PORT${C_NC}"
    echo -e "${C_BLUE}Open in browser: http://localhost:$PORT${C_NC}"
    log_msg "Server started on port $PORT"

    # Function to handle HTTP requests
    handle_request() {
        local request=""
        local line

        # Read HTTP request
        while IFS= read -r -t 0.1 line; do
            request="$request$line"$'\n'
            [ -z "$line" ] && break  # Empty line = end of headers
        done

        # Parse request
        local method=$(echo "$request" | head -1 | awk '{print $1}')
        local path=$(echo "$request" | head -1 | awk '{print $2}')

        log_msg "Request: $method $path"

        # Route requests
        case "$path" in
            /)
                send_http_response "200" "text/html; charset=utf-8" "$(generate_html_page)"
                ;;
            /api/status)
                api_status
                ;;
            /api/sync/upload)
                api_sync "upload" "/data/data/com.termux.rafacodephi/files/home/storage/downloads" "root"
                ;;
            /api/sync/download)
                api_sync "download" "/data/data/com.termux.rafacodephi/files/home/storage/downloads" "root"
                ;;
            /api/sync/mirror)
                api_sync "mirror" "/data/data/com.termux.rafacodephi/files/home/storage/downloads" "root"
                ;;
            /api/config)
                api_config "" ""
                ;;
            /api/logs)
                api_logs
                ;;
            /api/logs/clear)
                > "$LOG_FILE"
                send_http_response "200" "application/json" "{\"status\":\"success\"}"
                ;;
            *)
                send_http_response "404" "text/plain" "Not Found"
                ;;
        esac
    }

    # Use netcat or socat to listen for connections
    if command -v socat &> /dev/null; then
        socat TCP-LISTEN:$PORT,reuseaddr,fork EXEC:'bash -c "handle_request"' &
        wait
    elif command -v nc &> /dev/null; then
        while true; do
            handle_request | nc -l -p $PORT -q 1
        done
    fi
}

# Main
main() {
    local command="${1:-start}"

    init_webservice
    check_deps || exit 1

    case "$command" in
        start)
            run_server
            ;;
        port)
            export GDRIVE_PORT="${2:-8080}"
            run_server
            ;;
        stop)
            pkill -f gdrive-webservice
            log_msg "Server stopped"
            ;;
        status)
            if pgrep -f gdrive-webservice > /dev/null; then
                echo "Server is running on port $PORT"
            else
                echo "Server is not running"
            fi
            ;;
        help|*)
            cat << EOF
Google Drive Webservice

Usage: $0 <command> [options]

Commands:
  start           Start webservice on default port (8080)
  port <PORT>     Start webservice on specific port
  stop            Stop running webservice
  status          Show webservice status
  help            Show this help message

Environment:
  GDRIVE_PORT     Set default port (default: 8080)

Access in browser: http://localhost:8080

EOF
            ;;
    esac
}

main "$@"
