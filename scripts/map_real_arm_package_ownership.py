#!/usr/bin/env python3
"""Map real ARM bootstrap PREFIX entries back to their originating Termux packages.

This does not rebuild or rewrite binaries. It turns a file-level prefix audit into
package-level provenance so a BLOCKED gate can name the exact packages that need
a source rebuild for the RAFCODEPHI prefix.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
from pathlib import Path

from build_real_arm_bootstrap_core import (
    CORE_PACKAGES,
    DEFAULT_REPO,
    LEGACY_PREFIX,
    dependency_closure,
    download_deb,
    fetch_bytes,
    parse_packages,
)

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = "rafcodephi-real-pkg-package-ownership/v1"
PACKAGE_NAME = "com.termux.rafacodephi"


def data_member(deb: Path) -> tuple[Path, tempfile.TemporaryDirectory[str]]:
    holder = tempfile.TemporaryDirectory(prefix="raf-own-")
    tmp = Path(holder.name)
    subprocess.run(["ar", "x", str(deb)], cwd=tmp, check=True, stdout=subprocess.DEVNULL)
    members = sorted(tmp.glob("data.tar.*"))
    if not members:
        holder.cleanup()
        raise RuntimeError(f"missing data.tar member in {deb}")
    return members[0], holder


def package_prefix_entries(deb: Path) -> set[str]:
    member, holder = data_member(deb)
    try:
        proc = subprocess.run(
            ["tar", "-tf", str(member)],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        prefix = LEGACY_PREFIX.as_posix().rstrip("/") + "/"
        out: set[str] = set()
        for raw in proc.stdout.splitlines():
            normalized = raw.strip().lstrip("./")
            if not normalized.startswith(prefix):
                continue
            rel = normalized[len(prefix):].rstrip("/")
            if rel:
                out.add(rel)
        return out
    finally:
        holder.cleanup()


def build_ownership(repo: str, arch: str, cache: Path) -> dict:
    index_url = f"{repo.rstrip('/')}/dists/stable/main/binary-{arch}/Packages"
    index = parse_packages(fetch_bytes(index_url).decode("utf-8"))
    records = dependency_closure(index, CORE_PACKAGES)
    arch_cache = cache / arch
    arch_cache.mkdir(parents=True, exist_ok=True)

    entries: dict[str, dict] = {}
    packages: dict[str, dict] = {}
    for record in records:
        deb = download_deb(repo, arch_cache, record)
        owner = {
            "package": record.name,
            "version": record.version,
            "filename": record.filename,
            "deb_sha256": record.sha256,
        }
        packages[record.name] = owner
        # Extraction order in build_real_arm_bootstrap_core.py is identical to
        # this dependency-closure order; later packages therefore correctly
        # win ownership for any path that is overwritten in the merged prefix.
        for rel in package_prefix_entries(deb):
            entries[rel] = owner

    # prepare_prefix() renames the upstream proot binary before creating a
    # generated wrapper. Preserve the original package provenance under the
    # final archive name observed by the prefix auditor.
    if "bin/proot" in entries:
        entries["bin/proot.real"] = entries["bin/proot"]

    return {
        "schema": SCHEMA,
        "arch": arch,
        "package_name": PACKAGE_NAME,
        "canonical_prefix": f"/data/data/{PACKAGE_NAME}/files/usr",
        "repository": repo.rstrip("/"),
        "root_package_count": len(CORE_PACKAGES),
        "dependency_closure_package_count": len(records),
        "owned_entry_count": len(entries),
        "packages": dict(sorted(packages.items())),
        "entries": dict(sorted(entries.items())),
        "claim_allowed_runtime": False,
        "device_validation": "TOKEN_VAZIO",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default=DEFAULT_REPO)
    parser.add_argument("--arch", choices=("arm", "aarch64"), required=True)
    parser.add_argument("--cache", type=Path, default=ROOT / "out" / "termux-deb-cache")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    payload = build_ownership(args.repo, args.arch, args.cache)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(payload, ensure_ascii=False, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(
        "package_ownership=PASS "
        f"arch={args.arch} packages={payload['dependency_closure_package_count']} "
        f"entries={payload['owned_entry_count']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
