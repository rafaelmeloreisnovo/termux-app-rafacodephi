#!/usr/bin/env python3
"""Bind API29 source-built ARM bootstraps to the separately pinned signed APT snapshot.

Only producer run 43126603251 (both ABI jobs green) is eligible.
Check artifact transport SHA256 AND the exact inner bootstrap SHA256; never
accept bridge substitutes or unreviewed source bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

# Artifacts from producer run 43126603251, commit
# 9b293d4a527dca67350f60a7344ec4b9b37709c2 (both source-build ABI jobs PASS_SCOPED).
# Each (artifact ZIP SHA256, internal bootstrap ZIP SHA256) was recorded in job logs.
INPUTS = {
    'arm': ('6c30a79d8a983acd2c64eb3ad6567846984aef0b31de989441ab5fedbd2ee922',
            'deee2b544b56f5611d5165d87a7305007903cddf78c9d0eef59798a5c5986d38'),
    'aarch64': ('c6f7e1a9b9a28912f57f8d574a1dd91cd5c3c16e4fdd88bc0042f1189617d105',
                '2e87b94548d0258b928428f851f7fd9d01fccdf2745cae10f83c3f8c837b1ff1'),
}
PRODUCER_RUN_ID = 43126603251
PRODUCER_COMMIT = '9b293d4a527dca67350f60a7344ec4b9b37709c2'
KEY_SHA256 = '12beed0a1af401641bade1ae0d02f94d593bfc6cf8bda76d737d558b92df5f6b'
REPOSITORY = 'https://raw.githubusercontent.com/rafaelmeloreisnovo/termux-packages/rafcodephi-apt-preview-42658185015/repo'
SEALER_SHA256 = 'da23843d0008379b5febd59f09b95bca816e7512792399cd55bfea3da88f178e'


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def prepare(inputs, output, sealer, key):
    inputs, output, sealer, key = map(Path, (inputs, output, sealer, key))
    if digest(key) != KEY_SHA256:
        raise ValueError('APT public key differs from independent pin')
    if digest(sealer) != SEALER_SHA256:
        raise ValueError('producer live-bootstrap sealer differs from immutable source')
    output.mkdir(parents=True, exist_ok=False)
    manifest = {}
    parents = {}
    for arch, (artifact_sha, zip_sha) in INPUTS.items():
        archive = inputs / (arch + '.zip')
        if digest(archive) != artifact_sha:
            raise ValueError('source artifact digest mismatch: ' + arch)
        with zipfile.ZipFile(archive) as z:
            if len(set(z.namelist())) != len(z.namelist()):
                raise ValueError('duplicate producer ZIP members')
            payload = z.read(f'artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-{arch}.zip')
            if hashlib.sha256(payload).hexdigest() != zip_sha:
                raise ValueError('source-built bootstrap digest mismatch: ' + arch)
            (output / f'rafcodephi-bootstrap-{arch}.zip').write_bytes(payload)
            raw_manifest = z.read('artifacts/rafcodephi-bootstrap/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt')
            fields = {}
            for line in raw_manifest.decode().splitlines():
                k, v = line.split('=', 1)
                if k in fields:
                    raise ValueError('duplicate source manifest key')
                fields[k] = v
            # New source manifest must bind exactly the known inner ZIP, or fail.
            if fields.get(f'sha256_{arch}') != zip_sha:
                raise ValueError('producer manifest bootstrap digest mismatch: ' + arch)
            if fields.get(f'android_api_target_{arch}') != '29':
                raise ValueError('producer did not attest API29: ' + arch)
            if fields.get(f'prefix_gate_state_{arch}') != 'PASS_SCOPED':
                raise ValueError('producer prefix gate not passed: ' + arch)
            fields[f'source_parent_manifest_sha256_{arch}'] = hashlib.sha256(raw_manifest).hexdigest()
            for k, v in fields.items():
                if k in manifest and manifest[k] != v:
                    raise ValueError('conflicting producer manifest field: ' + k)
                manifest[k] = v
            parents[arch] = artifact_sha
    path = output / 'RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt'
    path.write_text(''.join(k + '=' + v + '\n' for k, v in manifest.items()))
    source = sealer.read_text()
    marker = 'python3 - "$OUT_DIR" "$REPOSITORY_URL" "$TRUST_MODE" "$PUBLIC_KEY_FILE" "$TARGET_PREFIX" <<\'PY\'\n'
    if source.count(marker) != 1:
        raise ValueError('pinned sealer entrypoint changed')
    code = source.split(marker, 1)[1].split('\nPY\n', 1)[0]
    subprocess.run(['python3', '-', str(output), REPOSITORY, 'signed-by', str(key.resolve()),
                    '/data/data/com.termux.rafacodephi/files/usr'], input=code, text=True, check=True)
    (output / 'ci-input-custody.json').write_text(json.dumps({
        'schema': 'rafcodephi.ci-real-pkg-inputs/v2', 'parent_artifacts': parents,
        'source_run': PRODUCER_RUN_ID, 'source_commit': PRODUCER_COMMIT,
        'source_api_target': 29,
        'sealer_sha256': SEALER_SHA256, 'archive_key_sha256': KEY_SHA256,
        'repository_url': REPOSITORY, 'device_runtime': 'TOKEN_VAZIO', 'claim_allowed': False,
    }, indent=2) + '\n')


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    for arg in ('inputs', 'output', 'sealer', 'key'):
        p.add_argument('--' + arg, required=True)
    a = p.parse_args()
    prepare(a.inputs, a.output, a.sealer, a.key)
