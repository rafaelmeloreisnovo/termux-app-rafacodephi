#!/usr/bin/env bash
# Optional read-only loopback status service. Sync writes remain CLI-only.
set -euo pipefail
umask 077
PYTHON_BIN="${GDRIVE_PYTHON_BIN:-python3}"
CFG="${GDRIVE_CONFIG_DIR:-${HOME}/.config/gdrive-plugin}"
STATUS="${GDRIVE_STATUS_FILE:-${CFG}/sync.status.json}"
SYNC_LOG="${GDRIVE_SYNC_LOG_FILE:-${CFG}/sync.log}"
WEB_LOG="${GDRIVE_WEB_LOG_FILE:-${CFG}/webservice.log}"
PID_FILE="${GDRIVE_WEB_PID_FILE:-${CFG}/webservice.pid}"
HOST="${GDRIVE_WEB_HOST:-127.0.0.1}"
PORT="${GDRIVE_WEB_PORT:-8080}"

init() { mkdir -p "$CFG"; chmod 700 "$CFG"; touch "$WEB_LOG"; chmod 600 "$WEB_LOG"; }
valid_port() { [[ "$1" =~ ^[0-9]+$ ]] && (( $1 >= 1024 && $1 <= 65535 )); }

serve() {
    local port="${1:-$PORT}"
    [[ "$HOST" == 127.0.0.1 || "$HOST" == ::1 || "$HOST" == localhost ]] || { echo 'ERROR: loopback only' >&2; return 1; }
    valid_port "$port" || { echo "ERROR: invalid port: $port" >&2; return 2; }
    command -v "$PYTHON_BIN" >/dev/null 2>&1 || { echo 'ERROR: python3 required' >&2; return 127; }
    exec "$PYTHON_BIN" - "$HOST" "$port" "$STATUS" "$SYNC_LOG" "$PID_FILE" <<'PY'
import html,json,os,signal,sys
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
host,port,status_name,log_name,pid_name=sys.argv[1],int(sys.argv[2]),*sys.argv[3:]
status_path,log_path,pid_path=map(Path,(status_name,log_name,pid_name))
pid_path.write_text(str(os.getpid())+'\n'); os.chmod(pid_path,0o600)
def status():
    if not status_path.exists(): return {'schema':'gdrive-plugin-status/v2','state':'NEVER_RUN','claim_allowed':False}
    try:
        value=json.loads(status_path.read_text()); assert isinstance(value,dict); return value
    except Exception as exc: return {'schema':'gdrive-plugin-status/v2','state':'INVALID_STATUS_FILE','error':type(exc).__name__,'claim_allowed':False}
def logs(): return log_path.read_text(errors='replace').splitlines()[-50:] if log_path.exists() else []
class H(BaseHTTPRequestHandler):
    def send(self,code,ctype,body):
        data=body.encode(); self.send_response(code); self.send_header('Content-Type',ctype); self.send_header('Content-Length',str(len(data))); self.send_header('Cache-Control','no-store'); self.send_header('X-Content-Type-Options','nosniff'); self.end_headers(); self.wfile.write(data)
    def do_GET(self):
        if self.path=='/api/status': return self.send(200,'application/json; charset=utf-8',json.dumps(status(),ensure_ascii=False))
        if self.path=='/api/logs': return self.send(200,'application/json; charset=utf-8',json.dumps({'logs':logs()},ensure_ascii=False))
        if self.path=='/':
            body='<h1>RAFCODEΦ Drive status V2</h1><p><b>Read-only.</b> Sync remains CLI-only.</p><pre>'+html.escape(json.dumps(status(),indent=2,ensure_ascii=False))+'</pre><pre>'+html.escape('\n'.join(logs()))+'</pre>'
            return self.send(200,'text/html; charset=utf-8',body)
        self.send(404,'application/json','{"error":"not_found"}')
    def do_POST(self): self.send(405,'application/json','{"error":"write_api_disabled","claim_allowed":false}')
    def log_message(self,fmt,*args): sys.stderr.write((fmt%args)+'\n')
def clean(*_): pid_path.unlink(missing_ok=True); raise SystemExit(0)
signal.signal(signal.SIGTERM,clean); signal.signal(signal.SIGINT,clean)
try: ThreadingHTTPServer((host,port),H).serve_forever()
finally: pid_path.unlink(missing_ok=True)
PY
}

daemon() {
    local port="${1:-$PORT}"
    if [[ -f "$PID_FILE" ]] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then echo "already running pid=$(cat "$PID_FILE")"; return; fi
    nohup "$0" start "$port" >>"$WEB_LOG" 2>&1 &
    local child=$!; sleep .3; kill -0 "$child" 2>/dev/null || { echo "ERROR: inspect $WEB_LOG" >&2; return 1; }
    echo "started http://${HOST}:${port} pid=$child"
}
stop() {
    [[ -f "$PID_FILE" ]] || { echo 'not running'; return; }
    local pid; pid=$(cat "$PID_FILE"); [[ "$pid" =~ ^[0-9]+$ ]] || { echo 'ERROR: invalid pid' >&2; return 1; }
    kill -0 "$pid" 2>/dev/null && kill "$pid" || rm -f "$PID_FILE"
}
show() {
    if [[ -f "$PID_FILE" ]] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then printf '{"state":"RUNNING","pid":%s,"write_api":false}\n' "$(cat "$PID_FILE")"; else printf '{"state":"STOPPED","write_api":false}\n'; fi
}
usage() { echo "Usage: $0 start|daemon [PORT]|stop|status"; }
main() { init; case "${1:-help}" in start) serve "${2:-$PORT}";; daemon) daemon "${2:-$PORT}";; stop) stop;; status) show;; *) usage;; esac; }
[[ "${BASH_SOURCE[0]}" != "$0" ]] || main "$@"
