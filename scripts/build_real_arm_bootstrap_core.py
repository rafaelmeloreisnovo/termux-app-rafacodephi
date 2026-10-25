#!/usr/bin/env python3
"""Build a real ARM Termux package-core bootstrap payload for RAFCODEPHI.

The script downloads Termux .deb packages for aarch64 and arm, verifies SHA-256,
extracts the dependency closure into a package-specific PREFIX, rewrites legacy
com.termux paths in text files, emits sources.list/resolv.conf/cert locations,
converts symlinks into TermuxInstaller's SYMLINKS.txt contract, and writes
bootstrap zip payloads suitable for app/src/main/cpp/rewritten-bootstrap-{aarch64,arm}.zip.

This is the concrete pkg path, not the pkg-help bridge. It still does not claim
DEVICE_VALIDATED: generated payloads need real-device `pkg update` and
`pkg install nano python git` smoke before docs can move TOKEN_VAZIO entries to
PROVADO.
"""
from __future__ import annotations

import argparse
import hashlib
import hmac
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
from urllib.parse import urlsplit
import zipfile
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPO = "https://packages.termux.dev/apt/termux-main"
PACKAGE_NAME = "com.termux.rafacodephi"
LEGACY_PREFIX = Path("data/data/com.termux/files/usr")
CORE_PACKAGES = (
    "apt",
    "bash",
    "busybox",
    "ca-certificates",
    "coreutils",
    "dpkg",
    "findutils",
    "gawk",
    "grep",
    "gzip",
    "ncurses-utils",
    "proot",
    "sed",
    "tar",
    "termux-tools",
)
SMOKE_PACKAGES = ("nano", "python", "git")
ARCHES = ("aarch64", "arm")
MINIMUM_COMMANDS = (
    "cat",
    "ls",
    "clear",
    "head",
    "tail",
    "grep",
    "sed",
    "awk",
    "cut",
    "tr",
    "wc",
    "sort",
    "uniq",
    "xargs",
    "tee",
    "mkdir",
    "rmdir",
    "rm",
    "cp",
    "mv",
    "ln",
    "chmod",
    "uname",
    "id",
    "pwd",
    "env",
    "dirname",
    "basename",
    "touch",
    "printf",
    "echo",
    "sleep",
    "date",
    "dd",
    "du",
    "df",
    "ps",
    "kill",
    "which",
    "find",
    "readlink",
    "realpath",
    "expr",
    "yes",
    "false",
    "true",
    "seq",
    "tar",
    "gzip",
    "gunzip",
    "zcat",
    "stat",
    "strings",
    "file",
    "whoami",
    "hostname",
    "printenv",
)


@dataclass(frozen=True)
class PackageRecord:
    name: str
    version: str
    filename: str
    sha256: str
    depends: tuple[str, ...]


def fail(message: str) -> None:
    raise SystemExit(f"[real-arm-bootstrap-core][ERROR] {message}")


def fetch_bytes(url: str) -> bytes:
    with urllib.request.urlopen(url, timeout=60) as response:
        return response.read()


def parse_packages(text: str) -> dict[str, PackageRecord]:
    records: dict[str, PackageRecord] = {}
    for block in re.split(r"\n(?=Package: )", text.strip()):
        fields: dict[str, str] = {}
        key = None
        for line in block.splitlines():
            if line.startswith(" ") and key:
                fields[key] += " " + line.strip()
                continue
            if ": " in line:
                key, value = line.split(": ", 1)
                fields[key] = value.strip()
        name = fields.get("Package")
        filename = fields.get("Filename")
        sha256 = fields.get("SHA256")
        version = fields.get("Version", "unknown")
        if not name or not filename or not sha256:
            continue
        records[name] = PackageRecord(
            name=name,
            version=version,
            filename=filename,
            sha256=sha256,
            depends=parse_depends(fields.get("Depends", "") + ", " + fields.get("Pre-Depends", "")),
        )
    return records


def parse_depends(raw: str) -> tuple[str, ...]:
    deps: list[str] = []
    for item in raw.split(","):
        item = item.strip()
        if not item:
            continue
        first = item.split("|", 1)[0]
        first = re.sub(r"\s*\(.*?\)", "", first).strip()
        first = re.sub(r":any$", "", first)
        if first and first not in deps:
            deps.append(first)
    return tuple(deps)


def dependency_closure(index: dict[str, PackageRecord], roots: tuple[str, ...]) -> list[PackageRecord]:
    seen: set[str] = set()
    ordered: list[PackageRecord] = []
    stack = list(roots)
    while stack:
        name = stack.pop(0)
        if name in seen:
            continue
        record = index.get(name)
        if not record:
            fail(f"missing package in index: {name}")
        seen.add(name)
        ordered.append(record)
        for dep in record.depends:
            if dep not in seen:
                stack.append(dep)
    return ordered


def verify_sha256(path: Path, expected: str) -> None:
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        fail(f"sha256 mismatch for {path.name}: expected={expected} actual={actual}")


def download_deb(repo: str, cache: Path, record: PackageRecord) -> Path:
    out = cache / Path(record.filename).name
    if not out.exists():
        out.write_bytes(fetch_bytes(f"{repo.rstrip('/')}/{record.filename}"))
    verify_sha256(out, record.sha256)
    return out


def extract_deb_data(deb: Path, dest: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="rafdeb-") as tmp_s:
        tmp = Path(tmp_s)
        subprocess.run(["ar", "x", str(deb)], cwd=tmp, check=True, stdout=subprocess.DEVNULL)
        members = sorted(tmp.glob("data.tar.*"))
        if not members:
            fail(f"missing data.tar member in {deb}")
        subprocess.run(["tar", "-xf", str(members[0]), "-C", str(dest)], check=True, stdout=subprocess.DEVNULL)


def rewrite_text_file(path: Path, legacy: str, current: str) -> None:
    try:
        data = path.read_bytes()
    except OSError:
        return
    if b"\x00" in data:
        return
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        return
    if legacy in text or "com.termux" in text:
        path.write_text(
            text.replace(legacy, current).replace("/data/data/com.termux/", f"/data/data/{PACKAGE_NAME}/"),
            encoding="utf-8",
        )


def write_command_wrapper(path: Path, applet: str, current_prefix: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        f"#!/system/bin/sh\n"
        f"# RAFCODEPHI real-core fallback wrapper: {applet}\n"
        f"PREFIX=\"${{PREFIX:-{current_prefix}}}\"\n"
        f"if [ -x \"$PREFIX/bin/busybox\" ]; then\n"
        f"    exec \"$PREFIX/bin/busybox\" \"{applet}\" \"$@\"\n"
        f"fi\n"
        f"if [ -x /system/bin/toybox ]; then\n"
        f"    exec /system/bin/toybox \"{applet}\" \"$@\"\n"
        f"fi\n"
        f"if [ -x /system/bin/toolbox ]; then\n"
        f"    exec /system/bin/toolbox \"{applet}\" \"$@\"\n"
        f"fi\n"
        f"echo 'RAFCODEPHI real-core: missing command backend: {applet}' >&2\n"
        f"exit 127\n",
        encoding="utf-8",
    )
    path.chmod(0o700)


def ensure_minimum_commands(prefix: Path, current_prefix: str) -> None:
    bin_dir = prefix / "bin"
    for applet in MINIMUM_COMMANDS:
        command_path = bin_dir / applet
        if command_path.exists() or command_path.is_symlink():
            continue
        write_command_wrapper(command_path, applet, current_prefix)


def prepare_prefix(extract_root: Path, prefix: Path, arch: str, repo: str) -> None:
    src = extract_root / LEGACY_PREFIX
    if not src.exists():
        fail(f"extracted prefix missing: {src}")
    if prefix.exists():
        shutil.rmtree(prefix)
    shutil.copytree(src, prefix, symlinks=True)
    current_prefix = f"/data/data/{PACKAGE_NAME}/files/usr"
    for path in prefix.rglob("*"):
        if path.is_file() and not path.is_symlink():
            rewrite_text_file(path, "/data/data/com.termux/files/usr", current_prefix)
    apt_dir = prefix / "etc" / "apt"
    apt_dir.mkdir(parents=True, exist_ok=True)
    (apt_dir / "sources.list").write_text(f"deb {repo.rstrip('/')} stable main\n", encoding="utf-8")
    (prefix / "etc" / "resolv.conf").write_text(
        "nameserver 1.1.1.1\nnameserver 8.8.8.8\noptions timeout:2 attempts:2\n",
        encoding="utf-8",
    )
    (prefix / "etc" / "rafcodephi-core.env").write_text(
        f"RAFCODEPHI_CORE_READY=1\nTERMUX_ARCH={arch}\nTERMUX_PACKAGE_NAME={PACKAGE_NAME}\nTERMUX_PREFIX={current_prefix}\n",
        encoding="utf-8",
    )
    proot = prefix / "bin" / "proot"
    proot_real = prefix / "bin" / "proot.real"
    if proot.exists() and not proot_real.exists():
        proot.rename(proot_real)
        proot.write_text(f"#!{current_prefix}/bin/sh\nexec {current_prefix}/bin/proot.real \"$@\"\n", encoding="utf-8")
        proot.chmod(0o700)
    ensure_minimum_commands(prefix, current_prefix)


def zip_prefix(prefix: Path, out_zip: Path, arch: str) -> None:
    out_zip.parent.mkdir(parents=True, exist_ok=True)
    symlinks: list[str] = []
    info = (
        f"TERMUX_PACKAGE_NAME={PACKAGE_NAME}\nTERMUX_ARCH={arch}\nTERMUX_MIN_API={'28' if arch == 'arm' else '21'}\n"
        "RAFCODEPHI_BOOTSTRAP=real-arm-core\nBOOTSTRAP_REAL_APT_READY=1\nBOOTSTRAP_REAL_DPKG_READY=1\n"
        "BOOTSTRAP_REAL_PROOT_READY=1\nBOOTSTRAP_REAL_COREUTILS_READY=1\nBOOTSTRAP_CA_CERTIFICATES_READY=1\n"
        "BOOTSTRAP_DNS_RESOLVER_READY=1\nBOOTSTRAP_MINIMUM_COMMANDS_READY=1\n"
    )
    written: set[str] = set()
    with zipfile.ZipFile(out_zip, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        zf.writestr("BOOTSTRAP_INFO", info)
        written.add("BOOTSTRAP_INFO")
        for directory in ("bin/", "etc/", "lib/", "libexec/", "tmp/", "var/"):
            if directory not in written:
                zf.writestr(directory, b"")
                written.add(directory)
        for path in sorted(prefix.rglob("*")):
            rel = path.relative_to(prefix).as_posix()
            if rel.startswith("../") or rel.startswith("/") or ".." in rel.split("/"):
                fail(f"unsafe path while zipping: {rel}")
            if path.is_symlink():
                target = os.readlink(path)
                target = target.replace("/data/data/com.termux/files/usr", f"/data/data/{PACKAGE_NAME}/files/usr")
                symlinks.append(f"{target}←{rel}")
                continue
            if path.is_dir():
                dir_rel = rel.rstrip("/") + "/"
                if dir_rel not in written:
                    zf.writestr(dir_rel, b"")
                    written.add(dir_rel)
                continue
            info_obj = zipfile.ZipInfo(rel)
            mode = 0o700 if rel.startswith(("bin/", "libexec/", "lib/apt/methods/")) else 0o600
            info_obj.external_attr = (0o100000 | mode) << 16
            if rel not in written:
                zf.writestr(info_obj, path.read_bytes())
                written.add(rel)
        if "bin/sh" not in {p.relative_to(prefix).as_posix() for p in prefix.rglob("*") if p.is_file() or p.is_symlink()}:
            fail("generated prefix lacks bin/sh")
        zf.writestr("SYMLINKS.txt", "\n".join(symlinks) + "\n")


def write_manifest(out_dir: Path, arch: str, records: list[PackageRecord], zip_path: Path) -> None:
    lines = [
        f"# RAFCODEPHI real ARM bootstrap core manifest ({arch})",
        "",
        f"zip: `{zip_path}`",
        f"sha256: `{hashlib.sha256(zip_path.read_bytes()).hexdigest()}`",
        "",
        "## Core packages",
        "",
    ]
    lines.extend(f"- `{r.name}` `{r.version}` — `{r.filename}`" for r in records)
    lines += [
        "",
        "## Minimum command fallback",
        "",
        "- Missing command applets are filled with small wrappers that call `$PREFIX/bin/busybox <applet>` first and Android toybox/toolbox second.",
        "- Real binaries from core packages are never overwritten by fallback wrappers.",
        "",
        "## Binary prefix audit limitation",
        "",
        "- Text files are rewritten from legacy Termux prefix to RAFCODEΦ prefix.",
        "- Binary/non-UTF-8 files are never rewritten in-place because prefix lengths differ and that can corrupt ELF or data payloads.",
        "- `scripts/validate_real_arm_bootstrap_core.py` must pass with no `LEGACY_PREFIX_BINARY_RISK`; otherwise rebuild the affected package with the RAFCODEΦ prefix or use a safe compatibility strategy.",
        "",
        "## Required real-device promotion tests",
        "",
    ]
    lines.extend(f"- `pkg install {pkg}` then `{pkg} --version` or package-specific version probe" for pkg in SMOKE_PACKAGES)
    (out_dir / f"real_arm_bootstrap_core_{arch}.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


# S17: hosted supply-chain admission, not part of the freestanding kernel.
# A matching SHA-256 binds *bytes*, but the human must separately authenticate
# the Release/InRelease signing key and provenance of each supplied digest.
# These independent trust roots are not established by the script itself.
def require_pinned_index(repo: str, arch: str) -> str:
    if arch not in ARCHES:
        fail("UNSUPPORTED_INDEX_ARCH: " + arch)
    parsed = urlsplit(repo)
    if (parsed.scheme != "https" or not parsed.hostname or
        parsed.username is not None or parsed.password is not None or
        parsed.fragment or parsed.port not in (None, 443)):
        fail("HTTPS_TRUSTED_REPO_REQUIRED")
    authorized_host = os.environ.get("RAFCODEPHI_APPROVED_REPO_HOST", "packages.termux.dev")
    if parsed.hostname != authorized_host:
        fail("UNAPPROVED_REPOSITORY_HOST: " + parsed.hostname)
    key = "RAFCODEPHI_INDEX_SHA256_" + arch.upper()
    expected = os.environ.get(key, "").strip().lower()
    if not re.fullmatch(r"[0-9a-f]{64}", expected):
        fail("INDEPENDENT_PACKAGES_INDEX_SHA256_PIN_REQUIRED: " + key)
    return expected


def fetch_pinned_index(repo: str, arch: str) -> bytes:
    expected = require_pinned_index(repo, arch)
    content = fetch_bytes(f"{repo.rstrip('/')}/dists/stable/main/binary-{arch}/Packages")
    actual = hashlib.sha256(content).hexdigest()
    if not hmac.compare_digest(actual, expected):
        fail("PACKAGES_INDEX_DIGEST_MISMATCH: " + arch)
    return content


def build_arch(repo: str, arch: str, output: Path, cache: Path) -> None:
    print(f"[real-arm-bootstrap-core] fetch index arch={arch}")
    index = parse_packages(fetch_pinned_index(repo, arch).decode("utf-8"))
    records = dependency_closure(index, CORE_PACKAGES)
    arch_cache = cache / arch
    arch_cache.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f"rafcore-{arch}-") as tmp_s:
        tmp = Path(tmp_s)
        extract_root = tmp / "extract"
        extract_root.mkdir()
        for record in records:
            deb = download_deb(repo, arch_cache, record)
            extract_deb_data(deb, extract_root)
        prefix = output / arch / "usr"
        prepare_prefix(extract_root, prefix, arch, repo)
        zip_path = ROOT / "app" / "src" / "main" / "cpp" / f"rewritten-bootstrap-{arch}.zip"
        zip_prefix(prefix, zip_path, arch)
        write_manifest(output / arch, arch, records, zip_path)
        print(f"[real-arm-bootstrap-core] wrote {zip_path} packages={len(records)}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default=DEFAULT_REPO)
    parser.add_argument("--arch", choices=ARCHES + ("all",), default="all")
    parser.add_argument("--output", type=Path, default=ROOT / "out" / "real-arm-bootstrap-core")
    parser.add_argument("--cache", type=Path, default=ROOT / "out" / "termux-deb-cache")
    args = parser.parse_args()
    if not shutil.which("ar") or not shutil.which("tar"):
        fail("required host tools missing: ar and tar")
    arches = ARCHES if args.arch == "all" else (args.arch,)
    # Fail before package/network/cache modifications if even one trust pin is absent.
    for arch in arches:
        require_pinned_index(args.repo, arch)
    for arch in arches:
        build_arch(args.repo, arch, args.output, args.cache)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
