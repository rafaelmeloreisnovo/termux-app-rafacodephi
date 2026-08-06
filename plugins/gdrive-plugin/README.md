# Google Drive Sync Plugin for Termux RAFCODEΦ

A low-level, pure bash implementation of Google Drive synchronization for Termux RAFCODEΦ. No external dependencies, no abstractions—just direct bash commands and Google Drive API calls.

## Features

✅ **OAuth 2.0 Authentication** - Secure Google account access  
✅ **Bidirectional Sync** - Mirror mode keeps directories synchronized  
✅ **Hash Tracking** - Shadow files system to detect changes efficiently  
✅ **Upload/Download** - Transfer files to and from Google Drive  
✅ **Web Interface** - Access via Chrome/browser on http://localhost:8080  
✅ **Low-Level Implementation** - Pure bash, minimal dependencies  
✅ **Ignore Patterns** - `.gdrive-ignore` file support  

## Installation

### 1. Prerequisites

Required commands (usually available in Termux):
```bash
bash, curl, sed, grep, awk, find, md5sum
```

Optional for better performance:
```bash
socat or nc (netcat) - for webservice
```

### 2. Copy Plugin

```bash
cp -r plugins/gdrive-plugin ~/.config/gdrive-plugin
chmod +x ~/.config/gdrive-plugin/gdrive-*.sh
```

### 3. Get Google OAuth Credentials

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project
3. Enable Google Drive API
4. Create OAuth 2.0 credentials (Desktop application)
5. Copy `Client ID` and `Client Secret`

## Quick Start

### 1. Authenticate with Google

```bash
./gdrive-auth.sh init <CLIENT_ID> <CLIENT_SECRET>
```

Follow the browser redirect and paste the authorization code.

### 2. Upload Files

```bash
./gdrive-sync.sh upload /path/to/local/dir <DRIVE_FOLDER_ID>
```

### 3. Download Files

```bash
./gdrive-sync.sh download <DRIVE_FOLDER_ID> /path/to/local/dir
```

### 4. Mirror Sync (Bidirectional)

```bash
./gdrive-sync.sh mirror /path/to/local/dir <DRIVE_FOLDER_ID>
```

### 5. Start Web Interface

```bash
./gdrive-webservice.sh start
# Open browser: http://localhost:8080
```

## Commands Reference

### Authentication (gdrive-auth.sh)

```bash
# Initialize OAuth flow
./gdrive-auth.sh init <CLIENT_ID> <CLIENT_SECRET> [REDIRECT_URI]

# Get current access token
./gdrive-auth.sh token

# Refresh token manually
./gdrive-auth.sh refresh <CLIENT_ID> <CLIENT_SECRET>

# Revoke tokens (logout)
./gdrive-auth.sh revoke

# Show token status
./gdrive-auth.sh status
```

### Sync Operations (gdrive-sync.sh)

```bash
# Upload local directory to Drive
./gdrive-sync.sh upload <LOCAL_DIR> <DRIVE_FOLDER_ID>

# Download from Drive to local directory
./gdrive-sync.sh download <DRIVE_FOLDER_ID> [LOCAL_DIR]

# Bidirectional mirror sync
./gdrive-sync.sh mirror <LOCAL_DIR> <DRIVE_FOLDER_ID>

# Show sync status
./gdrive-sync.sh status
```

### Webservice (gdrive-webservice.sh)

```bash
# Start on default port (8080)
./gdrive-webservice.sh start

# Start on custom port
./gdrive-webservice.sh port 9090

# Stop webservice
./gdrive-webservice.sh stop

# Show status
./gdrive-webservice.sh status
```

## Configuration

### Config Directory Structure

```
~/.config/gdrive-plugin/
├── tokens.json              # OAuth tokens (auto-created)
├── config.json              # Configuration file
├── .gdrive-shadow/          # Hash tracking directory
│   └── *.shadow             # File hash records
├── .gdrive-ignore           # Patterns to ignore
├── sync.log                 # Sync operations log
├── sync.status              # Last sync status
└── webservice.log           # Webservice logs
```

### Ignore Patterns (.gdrive-ignore)

Create `~/.config/gdrive-plugin/.gdrive-ignore`:

```
# Comments start with #
.git
node_modules
*.tmp
.DS_Store
__pycache__
```

### Configuration File (config.json)

Edit `~/.config/gdrive-plugin/config.json`:

```json
{
  "local_dir": "/path/to/sync",
  "drive_folder_id": "your-folder-id",
  "auto_sync_enabled": false,
  "sync_interval": 300,
  "log_level": "INFO"
}
```

## How It Works

### Shadow Files System

1. **First Sync**: Plugin creates hash records for each file
2. **Subsequent Syncs**: Compares current hashes with stored hashes
3. **Changed Files**: Only syncs files with different hashes
4. **Efficiency**: Reduces bandwidth and speeds up sync

### Hash Algorithm

- Primary: MD5 (fast, good for sync comparison)
- Fallback: SHA256 (if md5sum unavailable)
- Fallback: File size (if no hash command available)

### Mirror Sync Flow

```
Upload Phase:
  1. Find all local files
  2. Check if ignored
  3. Compare hash with shadow
  4. Upload if changed

Download Phase:
  1. List all files in Drive folder
  2. Check if ignored
  3. Compare hash with Drive metadata
  4. Download if changed
```

## Examples

### Example 1: Sync Downloads Folder

```bash
# Set up
./gdrive-auth.sh init YOUR_CLIENT_ID YOUR_CLIENT_SECRET

# Get Drive folder ID (visible in URL: docs.google.com/drive/folders/FOLDER_ID)

# First sync
./gdrive-sync.sh mirror ~/storage/downloads FOLDER_ID

# Check status
./gdrive-sync.sh status
```

### Example 2: Automated Sync Script

```bash
#!/bin/bash
PLUGIN_DIR="$HOME/.config/gdrive-plugin"
LOCAL_DIR="$HOME/storage/downloads"
DRIVE_ID="your-folder-id"

# Run mirror sync every 5 minutes
while true; do
    "$PLUGIN_DIR/gdrive-sync.sh" mirror "$LOCAL_DIR" "$DRIVE_ID"
    sleep 300
done
```

### Example 3: Web Interface

```bash
# Start webservice
./gdrive-webservice.sh start &

# Open in Chrome/browser
# Type in address bar: http://localhost:8080

# Configure and click "Mirror Sync"
```

## Troubleshooting

### "No tokens found"

```bash
# Solution: Run authentication
./gdrive-auth.sh init <CLIENT_ID> <CLIENT_SECRET>
```

### "Authentication failed"

```bash
# Check token status
./gdrive-auth.sh status

# Refresh manually
./gdrive-auth.sh refresh <CLIENT_ID> <CLIENT_SECRET>
```

### "Upload failed"

```bash
# Check logs
tail -f ~/.config/gdrive-plugin/sync.log

# Verify file exists
ls -la /path/to/file

# Verify Drive folder ID
echo "Folder ID should be visible in Drive URL"
```

### "Webservice won't start"

```bash
# Check if port is available
netstat -tuln | grep 8080

# Try different port
./gdrive-webservice.sh port 9090

# Check dependencies
which socat nc
```

## Performance Tips

1. **Use Mirror Sync** - More efficient than separate upload/download
2. **Set Up Ignore Patterns** - Skip unnecessary files
3. **Regular Syncs** - Small incremental syncs faster than full syncs
4. **Close Browser** - If not using webservice, close it to save resources

## API Reference

### REST API Endpoints (Webservice)

```
GET  /                      HTML interface
GET  /api/status            Get sync status (JSON)
POST /api/sync/upload       Start upload
POST /api/sync/download     Start download
POST /api/sync/mirror       Start mirror sync
POST /api/config            Save configuration
GET  /api/logs              Get recent logs
POST /api/logs/clear        Clear logs
```

### JSON Response Format

```json
{
  "status": "success|error|running",
  "message": "Operation description",
  "timestamp": "2024-08-06T12:34:56Z",
  "duration": 42.5
}
```

## Security Notes

- **Tokens File**: Stored with restricted permissions (mode 600)
- **No Plaintext**: Never stores credentials in plaintext
- **OAuth 2.0**: Uses secure OAuth 2.0 flow
- **HTTPS**: All API calls use HTTPS
- **Revoke**: Run `gdrive-auth.sh revoke` to logout

## Limitations

- Single folder sync (not recursive into subfolders)
- Requires manually configured Drive folder IDs
- No built-in scheduling (use cron/systemd timer)
- Webservice is HTTP only (use behind proxy for HTTPS)

## Development

### File Structure

```
plugins/gdrive-plugin/
├── gdrive-auth.sh           # OAuth handler
├── gdrive-sync.sh           # Sync engine
├── gdrive-webservice.sh     # HTTP server
├── gdrive-config.json       # Configuration schema
└── README.md               # This file
```

### Adding New Features

1. Edit appropriate shell script
2. Keep pure bash implementation
3. No external dependencies
4. Test thoroughly
5. Update docs

## License

This plugin is part of Termux RAFCODEΦ and follows the same GPLv3 license.

## Support

For issues and feature requests, visit:
- GitHub Issues: [termux-app-rafacodephi](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi)
- Documentation: See docs/plugins/

## Changelog

### v1.0.0 (2024-08-06)
- Initial release
- OAuth 2.0 authentication
- Upload/Download/Mirror operations
- Shadow files hash tracking
- Web interface
- Low-level bash implementation
- Ignore patterns support

---

**Made with ❤️ for Termux RAFCODEΦ**
