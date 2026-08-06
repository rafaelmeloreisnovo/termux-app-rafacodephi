#!/usr/bin/env python3
"""Fail-closed Google Drive flat-folder sync core for Termux RAFCODEΦ."""
from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from urllib.parse import urlencode
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError

CFG = Path(os.environ.get("GDRIVE_CONFIG_DIR", Path.home() / ".config/gdrive-plugin"))
STATE = Path(os.environ.get("GDRIVE_STATE_FILE", CFG / "sync-state.json"))
STATUS = Path(os.environ.get("GDRIVE_STATUS_FILE", CFG / "sync.status.json"))
LOG = Path(os.environ.get("GDRIVE_SYNC_LOG_FILE", CFG / "sync.log"))
IGNORE = Path(os.environ.get("GDRIVE_IGNORE_FILE", CFG / ".gdrive-ignore"))
LOCK = Path(os.environ.get("GDRIVE_LOCK_DIR", CFG / "sync.lock"))
AUTH = Path(os.environ.get("GDRIVE_AUTH_SCRIPT", Path(__file__).with_name("gdrive-auth.sh")))
CURL = os.environ.get("GDRIVE_CURL_BIN", "curl")
API = "https://www.googleapis.com/drive/v3"
UPLOAD = "https://www.googleapis.com/upload/drive/v3"
SCHEMA = "gdrive-plugin-sync-state/v2"


def now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def log(message: str) -> None:
    line = f"[{now()}] {message}"
    print(line, file=sys.stderr)
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a", encoding="utf-8") as fh:
        fh.write(line + "\n")


def atomic_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as fh:
            json.dump(value, fh, ensure_ascii=False, sort_keys=True, indent=2)
            fh.write("\n")
        os.chmod(tmp_name, 0o600)
        os.replace(tmp_name, path)
    finally:
        if os.path.exists(tmp_name):
            os.unlink(tmp_name)


def init() -> dict:
    CFG.mkdir(parents=True, exist_ok=True)
    os.chmod(CFG, 0o700)
    for path in (LOG, IGNORE):
        path.touch(exist_ok=True)
        os.chmod(path, 0o600)
    if not STATE.exists():
        atomic_json(STATE, {"schema": SCHEMA, "files": {}})
    state = json.loads(STATE.read_text(encoding="utf-8"))
    if state.get("schema") != SCHEMA or not isinstance(state.get("files"), dict):
        raise RuntimeError(f"invalid state contract: {STATE}")
    return state


def token() -> str:
    proc = subprocess.run([str(AUTH), "token"], text=True, capture_output=True)
    if proc.returncode:
        sys.stderr.write(proc.stderr)
        raise RuntimeError("authentication token unavailable")
    value = proc.stdout.strip()
    if not value or "\n" in value:
        raise RuntimeError("authentication stdout is not one token")
    return value


def md5(path: Path) -> str:
    h = hashlib.md5()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def ignored(name: str) -> bool:
    for raw in IGNORE.read_text(encoding="utf-8", errors="replace").splitlines():
        pattern = raw.strip()
        if pattern and not pattern.startswith("#") and fnmatch.fnmatchcase(name, pattern):
            return True
    return False


def local_inventory(root: Path) -> dict[str, dict]:
    if not root.is_dir():
        raise RuntimeError(f"local directory not found: {root}")
    nested = [p for p in root.rglob("*") if p.is_file() and p.parent != root]
    if nested:
        raise RuntimeError("nested files detected; V2 refuses silent path flattening")
    result: dict[str, dict] = {}
    for path in root.iterdir():
        if path.is_file() and not ignored(path.name):
            result[path.name] = {"name": path.name, "path": str(path), "md5": md5(path), "size": path.stat().st_size}
    return result


def json_request(method: str, url: str, access_token: str) -> dict:
    request = Request(url, method=method, headers={"Authorization": f"Bearer {access_token}", "Accept": "application/json"})
    try:
        with urlopen(request, timeout=60) as response:
            if not 200 <= response.status < 300:
                raise RuntimeError(f"HTTP {response.status}")
            value = json.load(response)
            if not isinstance(value, dict):
                raise RuntimeError("response is not a JSON object")
            return value
    except HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"HTTP {exc.code}: {body[:1000]}") from exc
    except URLError as exc:
        raise RuntimeError(f"network error: {exc.reason}") from exc


def remote_inventory(folder_id: str, access_token: str) -> dict[str, list[dict]]:
    page_token = ""
    result: dict[str, list[dict]] = {}
    while True:
        query = {
            "q": f"trashed=false and '{folder_id}' in parents",
            "fields": "nextPageToken,files(id,name,md5Checksum,size,modifiedTime,mimeType)",
            "pageSize": "1000",
            "spaces": "drive",
            "supportsAllDrives": "true",
            "includeItemsFromAllDrives": "true",
        }
        if page_token:
            query["pageToken"] = page_token
        page = json_request("GET", API + "/files?" + urlencode(query), access_token)
        files = page.get("files")
        if not isinstance(files, list):
            raise RuntimeError("Drive list response has no files array")
        for item in files:
            if isinstance(item, dict) and isinstance(item.get("name"), str):
                result.setdefault(item["name"], []).append(item)
        page_token = str(page.get("nextPageToken") or "")
        if not page_token:
            return result


def plan_actions(local: dict, remote: dict, state: dict, root: Path) -> list[dict]:
    actions: list[dict] = []
    names = sorted(set(local) | set(remote) | set(state.get("files", {})))
    for name in names:
        l = local.get(name)
        rs = remote.get(name, [])
        s = state["files"].get(name)
        if name in (".", "..") or "/" in name:
            actions.append({"name": name, "action": "unsafe_name"}); continue
        if len(rs) > 1:
            actions.append({"name": name, "action": "conflict_duplicate_remote", "remote_ids": [x.get("id") for x in rs]}); continue
        r = rs[0] if rs else None
        if r and str(r.get("mimeType", "")).startswith("application/vnd.google-apps."):
            actions.append({"name": name, "action": "unsupported_google_native", "remote": r}); continue
        if l and not r:
            actions.append({"name": name, "action": "upload" if s is None else "conflict_remote_deleted", "local": l, "state": s}); continue
        if r and not l:
            action = "download_remote" if s is None else "conflict_local_deleted"
            actions.append({"name": name, "action": action, "local": {"path": str(root / name), "md5": "", "size": 0}, "remote": r, "state": s}); continue
        if l and r:
            remote_md5 = str(r.get("md5Checksum") or "")
            if not remote_md5:
                actions.append({"name": name, "action": "unsupported_no_remote_md5", "local": l, "remote": r}); continue
            if s is None:
                action = "baseline" if l["md5"] == remote_md5 else "conflict_initial_mismatch"
            else:
                local_changed = l["md5"] != s.get("local_md5", "")
                remote_changed = remote_md5 != s.get("remote_md5", "")
                if l["md5"] == remote_md5: action = "baseline"
                elif local_changed and remote_changed: action = "conflict_both_changed"
                elif local_changed: action = "update_remote"
                elif remote_changed: action = "download_remote"
                else: action = "noop"
            actions.append({"name": name, "action": action, "local": l, "remote": r, "state": s}); continue
        if s is not None:
            actions.append({"name": name, "action": "state_orphan", "state": s})
    return actions


def curl_json(args: list[str]) -> dict:
    fd, out_name = tempfile.mkstemp(prefix="gdrive-curl-", suffix=".json")
    os.close(fd)
    try:
        proc = subprocess.run([CURL, "-sS", "-o", out_name, "-w", "%{http_code}", *args], text=True, capture_output=True)
        if proc.returncode:
            raise RuntimeError(f"curl failed: {proc.stderr.strip()}")
        if not proc.stdout.startswith("2"):
            body = Path(out_name).read_text(encoding="utf-8", errors="replace")
            raise RuntimeError(f"HTTP {proc.stdout}: {body[:1000]}")
        value = json.loads(Path(out_name).read_text(encoding="utf-8"))
        if not isinstance(value, dict):
            raise RuntimeError("response is not a JSON object")
        return value
    finally:
        Path(out_name).unlink(missing_ok=True)


def upload(path: Path, name: str, folder_id: str, access_token: str) -> dict:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", delete=False) as meta:
        json.dump({"name": name, "parents": [folder_id]}, meta, ensure_ascii=False)
        meta_name = meta.name
    try:
        return curl_json(["-X", "POST", f"{UPLOAD}/files?uploadType=multipart&fields=id,name,md5Checksum,size,modifiedTime",
                          "-H", f"Authorization: Bearer {access_token}",
                          "-F", f"metadata=@{meta_name};type=application/json;charset=UTF-8",
                          "-F", f"file=@{path};type=application/octet-stream"])
    finally:
        Path(meta_name).unlink(missing_ok=True)


def update(path: Path, file_id: str, access_token: str) -> dict:
    return curl_json(["-X", "PATCH", f"{UPLOAD}/files/{file_id}?uploadType=media&fields=id,name,md5Checksum,size,modifiedTime",
                      "-H", f"Authorization: Bearer {access_token}", "-H", "Content-Type: application/octet-stream", "--data-binary", f"@{path}"])


def download(file_id: str, expected_md5: str, destination: Path, access_token: str) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(prefix=destination.name + ".", suffix=".tmp", dir=destination.parent)
    os.close(fd)
    try:
        proc = subprocess.run([CURL, "-sS", "-L", "--fail-with-body", "-o", tmp_name,
                               "-H", f"Authorization: Bearer {access_token}", f"{API}/files/{file_id}?alt=media&supportsAllDrives=true"])
        if proc.returncode:
            raise RuntimeError(f"download failed with curl rc={proc.returncode}")
        if md5(Path(tmp_name)) != expected_md5:
            raise RuntimeError("downloaded checksum mismatch")
        os.chmod(tmp_name, 0o600)
        os.replace(tmp_name, destination)
    finally:
        Path(tmp_name).unlink(missing_ok=True)


def state_entry(action: dict) -> dict:
    l, r = action["local"], action["remote"]
    return {"local_md5": l["md5"], "remote_md5": r["md5Checksum"], "drive_id": r["id"],
            "size": int(r.get("size") or l.get("size") or 0), "modified_time": r.get("modifiedTime", ""), "synced_at": now()}


def write_status(state: str, mode: str, message: str, actions: list[dict]) -> None:
    counts: dict[str, int] = {}
    for item in actions: counts[item["action"]] = counts.get(item["action"], 0) + 1
    atomic_json(STATUS, {"schema": "gdrive-plugin-status/v2", "state": state, "mode": mode, "message": message,
                         "timestamp": now(), "counts": counts, "claim_allowed": False, "release_allowed": False})


def execute(mode: str, root: Path, folder_id: str) -> dict:
    state = init(); access_token = token()
    actions = plan_actions(local_inventory(root), remote_inventory(folder_id, access_token), state, root)
    blockers = {"unsafe_name", "state_orphan"}
    if any(a["action"].startswith(("conflict_", "unsupported_")) or a["action"] in blockers for a in actions):
        write_status("BLOCKED", mode, "plan contains conflict/unsupported state", actions)
        return {"schema": "gdrive-plugin-sync-plan/v2", "actions": actions}
    allowed = {"mirror": {"upload", "update_remote", "download_remote", "baseline", "noop"},
               "upload": {"upload", "update_remote", "baseline", "noop"},
               "download": {"download_remote", "baseline", "noop"}}[mode]
    if any(a["action"] not in allowed for a in actions):
        write_status("BLOCKED", mode, "plan exceeds selected direction", actions)
        return {"schema": "gdrive-plugin-sync-plan/v2", "actions": actions}
    try: LOCK.mkdir()
    except FileExistsError as exc: raise RuntimeError("another sync is active") from exc
    try:
        write_status("RUNNING", mode, "applying verified actions", actions)
        for a in actions:
            name, kind = a["name"], a["action"]
            if kind == "upload":
                response = upload(Path(a["local"]["path"]), name, folder_id, access_token)
                if response.get("md5Checksum") != a["local"]["md5"]: raise RuntimeError(f"upload receipt mismatch: {name}")
                a["remote"] = response
            elif kind == "update_remote":
                response = update(Path(a["local"]["path"]), a["remote"]["id"], access_token)
                if response.get("md5Checksum") != a["local"]["md5"]: raise RuntimeError(f"update receipt mismatch: {name}")
                a["remote"] = response
            elif kind == "download_remote":
                download(a["remote"]["id"], a["remote"]["md5Checksum"], Path(a["local"]["path"]), access_token)
                a["local"]["md5"] = md5(Path(a["local"]["path"]))
            if kind in {"upload", "update_remote", "download_remote", "baseline", "noop"}:
                state["files"][name] = state_entry(a); atomic_json(STATE, state)
        write_status("SUCCESS", mode, "all actions verified", actions)
    except Exception as exc:
        write_status("FAIL", mode, str(exc), actions); raise
    finally:
        shutil.rmtree(LOCK, ignore_errors=True)
    return {"schema": "gdrive-plugin-sync-plan/v2", "actions": actions}


def selftest() -> dict:
    local = {"x": {"name": "x", "path": "/tmp/x", "md5": "new", "size": 1}, "y": {"name": "y", "path": "/tmp/y", "md5": "same", "size": 1}}
    remote = {"x": [{"id": "1", "name": "x", "md5Checksum": "old", "mimeType": "text/plain"}],
              "y": [{"id": "2", "name": "y", "md5Checksum": "same", "mimeType": "text/plain"}]}
    state = {"schema": SCHEMA, "files": {"x": {"local_md5": "old", "remote_md5": "old"}}}
    actions = plan_actions(local, remote, state, Path("/tmp"))
    found = {a["name"]: a["action"] for a in actions}
    if found != {"x": "update_remote", "y": "baseline"}: raise RuntimeError(found)
    return {"selftest": "PASS", "planner": "PASS", "claim_allowed": False}


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("plan", "mirror", "upload"):
        p = sub.add_parser(name); p.add_argument("local_dir", type=Path); p.add_argument("folder_id")
    p = sub.add_parser("download"); p.add_argument("folder_id"); p.add_argument("local_dir", type=Path)
    sub.add_parser("status"); sub.add_parser("selftest")
    args = parser.parse_args()
    try:
        if args.command == "status":
            value = json.loads(STATUS.read_text()) if STATUS.exists() else {"state": "NEVER_RUN", "claim_allowed": False, "release_allowed": False}
        elif args.command == "selftest": value = selftest()
        elif args.command == "plan":
            state = init(); access_token = token(); value = {"schema": "gdrive-plugin-sync-plan/v2",
                "actions": plan_actions(local_inventory(args.local_dir), remote_inventory(args.folder_id, access_token), state, args.local_dir)}
        elif args.command == "download": value = execute("download", args.local_dir, args.folder_id)
        else: value = execute(args.command, args.local_dir, args.folder_id)
    except Exception as exc:
        print(json.dumps({"state": "FAIL", "error": str(exc), "claim_allowed": False}), file=sys.stderr); return 1
    print(json.dumps(value, ensure_ascii=False, indent=2)); return 0


if __name__ == "__main__":
    raise SystemExit(main())
