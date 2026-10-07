#!/usr/bin/env python3
"""Smoke/falsifier for the exact portable custody step; stdlib-only."""
from __future__ import annotations

import hashlib
import subprocess
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/freestanding-enterprise-closure.yml"
START = "      - name: Pack portable per-arch DEB custody\n"
END = "      - name: Upload enterprise candidate and custody evidence\n"


def exact_script(text: str) -> str:
    assert text.count(START) == 1 and text.count(END) == 1
    chunk = text.split(START, 1)[1].split(END, 1)[0]
    assert "        run: |\n" in chunk
    script = chunk.split("        run: |\n", 1)[1]
    script = "\n".join(
        line[10:] if line.startswith("          ") else line
        for line in script.splitlines()
    )
    assert 'sha256sum -c SHA256SUMS' in script
    assert "--format=posix" in script
    return script


def execute(script: str, cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["bash", "-euo", "pipefail", "-c", script],
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
    )


def main() -> int:
    workflow = WORKFLOW.read_text(encoding="utf-8")
    assert "            termux-packages/artifacts/rafcodephi-bootstrap/\n" not in workflow
    script = exact_script(workflow)
    with tempfile.TemporaryDirectory(prefix="rafcodephi-custody-") as temp:
        workdir = Path(temp)
        debdir = workdir / "termux-packages/artifacts/rafcodephi-bootstrap/debs/arm"
        debdir.mkdir(parents=True)
        filename = "ca-certificates-java_1:2026.05.14_all.deb"
        body = b"source-built DEB fixture, not a real package\n"
        (debdir / filename).write_bytes(body)
        expected = hashlib.sha256(body).hexdigest()
        (debdir / "SHA256SUMS").write_text(
            f"{expected}  {filename}\n", encoding="utf-8"
        )

        first = execute(script, workdir)
        assert first.returncode == 0, first.stderr
        archive = workdir / "build/reports/rafcodephi-deb-custody-arm.tar"
        digest_path = archive.with_suffix(".tar.sha256")
        assert archive.is_file() and digest_path.is_file()
        with tarfile.open(archive, "r") as tf:
            members = {m.name.lstrip("./"): m for m in tf.getmembers()}
            assert filename in members
            source = tf.extractfile(members[filename])
            assert source and source.read() == body
        tar_digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        assert digest_path.read_text(encoding="utf-8").startswith(tar_digest)

        second = execute(script, workdir)
        assert second.returncode == 0, second.stderr
        assert hashlib.sha256(archive.read_bytes()).hexdigest() == tar_digest

        (debdir / filename).write_bytes(b"tampered evidence\n")
        falsifier = execute(script, workdir)
        assert falsifier.returncode != 0, "modified DEB must fail SHA256SUMS"

    print("RAFCODEPHI_PORTABLE_CUSTODY_TEST=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
