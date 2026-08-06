#!/usr/bin/env bash
set -euo pipefail
umask 077

ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
PLUGIN="$ROOT/plugins/gdrive-plugin"
PROFILE="$ROOT/configs/gdrive-plugin-profile.json"
PASS=0

ok() { PASS=$((PASS + 1)); printf 'ok %02d - %s\n' "$PASS" "$1"; }
fail() { printf 'not ok - %s\n' "$1" >&2; exit 1; }

for command in bash curl jq md5sum awk find stat base64 od tr python3; do
    command -v "$command" >/dev/null 2>&1 || fail "missing test dependency: $command"
done
ok "test dependencies present"

for script in "$PLUGIN"/gdrive-auth.sh "$PLUGIN"/gdrive-sync.sh "$PLUGIN"/gdrive-webservice.sh; do
    bash -n "$script" || fail "bash syntax: $script"
done
python3 -m py_compile "$PLUGIN/gdrive-sync-core.py" || fail "python syntax"
rm -rf "$PLUGIN/__pycache__"
ok "Bash and Python syntax pass"

jq -e '.schema=="gdrive-plugin-config/v2" and .claim_allowed==false and .release_allowed==false and (.dependencies.required|index("python3"))!=null' \
    "$PLUGIN/gdrive-config.json" >/dev/null || fail "plugin config contract"
jq -e '.schema=="gdrive-plugin-profile-contract/v2" and .claim_allowed==false and .release_allowed==false and (.required_entries|index("bin/python3"))!=null' \
    "$PROFILE" >/dev/null || fail "profile contract"
ok "JSON contracts parse and remain fail-closed"

TMP_AUTH=$(mktemp -d)
HOME="$TMP_AUTH" GDRIVE_CONFIG_DIR="$TMP_AUTH/config" "$PLUGIN/gdrive-auth.sh" selftest | \
    jq -e '.selftest=="PASS" and .claim_allowed==false' >/dev/null || fail "auth selftest"
rm -rf "$TMP_AUTH"
ok "OAuth token JSON roundtrip passes without stdout contamination"

TMP_SYNC=$(mktemp -d)
HOME="$TMP_SYNC" GDRIVE_CONFIG_DIR="$TMP_SYNC/config" "$PLUGIN/gdrive-sync.sh" selftest | \
    jq -e '.selftest=="PASS" and .planner=="PASS" and .claim_allowed==false' >/dev/null || fail "sync planner selftest"
rm -rf "$TMP_SYNC"
ok "planner distinguishes local, remote and concurrent changes"

TMP_NEST=$(mktemp -d)
mkdir -p "$TMP_NEST/local/nested" "$TMP_NEST/config"
printf x > "$TMP_NEST/local/nested/file.txt"
PLUGIN_CORE="$PLUGIN/gdrive-sync-core.py" TMP_NEST="$TMP_NEST" python3 - <<'PY' || fail "nested file rejection"
import importlib.util, os
from pathlib import Path
os.environ['GDRIVE_CONFIG_DIR']=str(Path(os.environ['TMP_NEST'])/'config')
spec=importlib.util.spec_from_file_location('gdrive_sync_core', os.environ['PLUGIN_CORE'])
mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
mod.init()
try:
    mod.local_inventory(Path(os.environ['TMP_NEST'])/'local')
except RuntimeError as exc:
    assert 'nested files detected' in str(exc)
else:
    raise SystemExit('nested path was accepted')
PY
rm -rf "$TMP_NEST"
ok "nested files are rejected instead of silently flattened"

TMP_META=$(mktemp -d)
printf data > "$TMP_META/file.txt"
PLUGIN_CORE="$PLUGIN/gdrive-sync-core.py" TMP_META="$TMP_META" python3 - <<'PY' || fail "upload endpoint and metadata"
import importlib.util, json, os
from pathlib import Path
spec=importlib.util.spec_from_file_location('gdrive_sync_core', os.environ['PLUGIN_CORE'])
mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
captured={}
def fake(args):
    captured['args']=args
    meta_arg=next(x for x in args if x.startswith('metadata=@'))
    meta_name=meta_arg[len('metadata=@'):].split(';',1)[0]
    captured['metadata']=json.loads(Path(meta_name).read_text())
    return {'id':'mock-id','name':'safe "name".txt','md5Checksum':mod.md5(Path(os.environ['TMP_META'])/'file.txt'),'size':'4','modifiedTime':'2026-08-06T00:00:00Z'}
mod.curl_json=fake
result=mod.upload(Path(os.environ['TMP_META'])/'file.txt','safe "name".txt','folder-1','token-1')
assert any(x.startswith('https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart') for x in captured['args'])
assert captured['metadata']=={'name':'safe "name".txt','parents':['folder-1']}
assert result['id']=='mock-id'
PY
rm -rf "$TMP_META"
ok "multipart upload uses upload endpoint and escaped JSON metadata"

TMP_DL=$(mktemp -d)
PLUGIN_CORE="$PLUGIN/gdrive-sync-core.py" TMP_DL="$TMP_DL" python3 - <<'PY' || fail "three-state conflicts"
import importlib.util, os
from pathlib import Path
spec=importlib.util.spec_from_file_location('gdrive_sync_core', os.environ['PLUGIN_CORE'])
mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
root=Path(os.environ['TMP_DL'])
local={'both':{'name':'both','path':str(root/'both'),'md5':'local-new','size':1},'initial':{'name':'initial','path':str(root/'initial'),'md5':'a','size':1}}
remote={'both':[{'id':'1','name':'both','md5Checksum':'remote-new','mimeType':'text/plain'}], 'initial':[{'id':'2','name':'initial','md5Checksum':'b','mimeType':'text/plain'}]}
state={'schema':mod.SCHEMA,'files':{'both':{'local_md5':'old','remote_md5':'old'}}}
a={x['name']:x['action'] for x in mod.plan_actions(local,remote,state,root)}
assert a=={'both':'conflict_both_changed','initial':'conflict_initial_mismatch'}, a
PY
rm -rf "$TMP_DL"
ok "three-state planner fails closed on initial and concurrent mismatch"

TMP_WEB=$(mktemp -d)
export GDRIVE_CONFIG_DIR="$TMP_WEB/config"
export GDRIVE_WEB_PORT=18081
"$PLUGIN/gdrive-webservice.sh" daemon 18081 >/dev/null
for _ in 1 2 3 4 5 6 7 8 9 10; do
    curl -fsS http://127.0.0.1:18081/api/status > "$TMP_WEB/status.json" 2>/dev/null && break
    sleep 0.2
done
jq -e '.state=="NEVER_RUN" and .claim_allowed==false' "$TMP_WEB/status.json" >/dev/null || fail "read-only status"
code=$(curl -sS -o "$TMP_WEB/post.json" -w '%{http_code}' -X POST http://127.0.0.1:18081/api/status)
[[ "$code" == 405 ]] || fail "write API disabled"
jq -e '.error=="write_api_disabled" and .claim_allowed==false' "$TMP_WEB/post.json" >/dev/null || fail "write API body"
"$PLUGIN/gdrive-webservice.sh" stop >/dev/null
rm -rf "$TMP_WEB"
unset GDRIVE_CONFIG_DIR GDRIVE_WEB_PORT
ok "status service binds locally and rejects writes"

printf '1..%d\n' "$PASS"
printf '{"tests":%d,"passed":%d,"state":"LOCAL_CONTRACT_PASS","claim_allowed":false,"device_runtime":"TOKEN_VAZIO"}\n' "$PASS" "$PASS"
