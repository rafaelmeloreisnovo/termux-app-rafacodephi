#!/usr/bin/env python3
"""Bind recovered source-built ARM bootstraps to the pinned signed APT snapshot.

The recovery artifacts expire on GitHub; an unavailable input is an error, never
permission to replace the real package runtime with bridge stubs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

INPUTS = {
    'arm': ('633bbc8c4f1f7867c252c2b97e770bf7ca3e34a1951ef8c858626f3939286b6f',
            '256069e587fc1da5172d619554f58a2a6ec603fbd1269a3b68af2ca07674bb54'),
    'aarch64': ('742dc05cb8cba0ae5126ce8fec25a424a92ebce9b0cb4f8a5883f0f4b8f70c4d',
                '18b44b4065c42439ea67214eb10e88d815ee11bc0518231da82c77dac04ecde2'),
}
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
            raise ValueError('recovery artifact digest mismatch: ' + arch)
        with zipfile.ZipFile(archive) as z:
            if len(set(z.namelist())) != len(z.namelist()):
                raise ValueError('duplicate recovery ZIP members')
            payload = z.read(f'bootstrap/rafcodephi-bootstrap-{arch}.zip')
            if hashlib.sha256(payload).hexdigest() != zip_sha:
                raise ValueError('source-built bootstrap digest mismatch: ' + arch)
            (output / f'rafcodephi-bootstrap-{arch}.zip').write_bytes(payload)
            raw_manifest = z.read('bootstrap/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt')
            fields = {}
            for line in raw_manifest.decode().splitlines():
                k, v = line.split('=', 1)
                if k in fields:
                    raise ValueError('duplicate source manifest key')
                fields[k] = v
            # Preserve the historical exporter omission, then derive a successor
            # from immutable archive bytes. Do not weaken the app importer.
            fields[f'sha256_{arch}'] = zip_sha
            fields[f'recovered_parent_manifest_sha256_{arch}'] = hashlib.sha256(raw_manifest).hexdigest()
            for k, v in fields.items():
                if k in manifest and manifest[k] != v:
                    raise ValueError('conflicting producer manifest field: ' + k)
                manifest[k] = v
            parents[arch] = artifact_sha
    path = output / 'RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt'
    path.write_text(''.join(k + '=' + v + '\n' for k, v in manifest.items()))
    seal_signed_snapshot(output, sealer, key, parents)


def seal_signed_snapshot(output, sealer, key, parents):
    """Keep the existing independent APT key, URL and sealer identity."""
    if digest(key) != KEY_SHA256:
        raise ValueError('APT public key differs from independent pin')
    if digest(sealer) != SEALER_SHA256:
        raise ValueError('producer live-bootstrap sealer differs from immutable source')

    source = sealer.read_text()
    marker = 'python3 - "$OUT_DIR" "$REPOSITORY_URL" "$TRUST_MODE" "$PUBLIC_KEY_FILE" "$TARGET_PREFIX" <<\'PY\'\n'
    if source.count(marker) != 1:
        raise ValueError('pinned sealer entrypoint changed')
    code = source.split(marker, 1)[1].split('\nPY\n', 1)[0]
    subprocess.run(['python3', '-', str(output), REPOSITORY, 'signed-by', str(key.resolve()),
                    '/data/data/com.termux.rafacodephi/files/usr'], input=code, text=True, check=True)
    (output / 'ci-input-custody.json').write_text(json.dumps({
        'schema': 'rafcodephi.ci-real-pkg-inputs/v1', 'parent_artifacts': parents,
        'sealer_sha256': SEALER_SHA256, 'archive_key_sha256': KEY_SHA256,
        'repository_url': REPOSITORY, 'device_runtime': 'TOKEN_VAZIO', 'claim_allowed': False,
    }, indent=2) + '\n')



def prepare_from_producer_artifacts(inputs, output, sealer, key):
    """Consume completed source-build artifacts, never recompile or substitute bridges."""
    from io import BytesIO

    inputs, output, sealer, key = map(Path, (inputs, output, sealer, key))
    merged = {}
    parents = {}
    payloads = {}

    for arch in ('arm', 'aarch64'):
        artifact = inputs / (arch + '.zip')
        if not artifact.is_file() or artifact.stat().st_size == 0:
            raise ValueError('producer artifact absent or empty: ' + arch)
        with zipfile.ZipFile(artifact) as outer:
            names = outer.namelist()
            if len(names) != len(set(names)):
                raise ValueError('duplicate producer artifact members: ' + arch)
            if any(name.startswith('/') or '..' in Path(name).parts or '\\' in name
                   for name in names):
                raise ValueError('unsafe producer artifact member: ' + arch)

            def unique_member(basename):
                hits = [name for name in names if name == basename
                        or name.endswith('/' + basename)]
                if len(hits) != 1:
                    raise ValueError('expected one ' + basename +
                                     ' in producer artifact ' + arch)
                return hits[0]

            payload = outer.read(unique_member('rafcodephi-bootstrap-' + arch + '.zip'))
            if not payload or len(payload) > 512 * 1024 * 1024:
                raise ValueError('invalid real bootstrap byte length: ' + arch)
            manifest_bytes = outer.read(unique_member(
                'RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt'))

        fields = {}
        for line in manifest_bytes.decode('utf-8').splitlines():
            if not line or line.startswith('#'):
                continue
            if '=' not in line:
                raise ValueError('invalid producer manifest record: ' + arch)
            name, value = line.split('=', 1)
            if not name or name in fields:
                raise ValueError('duplicated/empty producer manifest field: ' + arch)
            fields[name] = value
        sha = hashlib.sha256(payload).hexdigest()
        if fields.get('sha256_' + arch) != sha:
            raise ValueError('real bootstrap producer SHA256 mismatch: ' + arch)
        if fields.get('bytes_' + arch) != str(len(payload)):
            raise ValueError('real bootstrap producer byte count mismatch: ' + arch)
        if (fields.get('schema') != 'rafcodephi.real-bootstrap-sourcebuild/v1'
                or fields.get('package_name') != 'com.termux.rafacodephi'
                or fields.get('bootstrap_ninja') != 'EMBEDDED'
                or fields.get('bridge_allowed') != 'false'):
            raise ValueError('source archive is not a real APT+Ninja bootstrap: ' + arch)

        with zipfile.ZipFile(BytesIO(payload)) as installed:
            required = ('bin/apt', 'bin/apt-get', 'bin/dpkg',
                        'bin/bash', 'bin/ninja', 'bin/pkg',
                        'BOOTSTRAP_PROFILE.json', 'var/lib/dpkg/status')
            if any(name not in installed.namelist() for name in required):
                raise ValueError('APT, Bash or Ninja missing inside producer ZIP: ' + arch)
            profile = json.loads(installed.read('BOOTSTRAP_PROFILE.json'))
            if (profile.get('profile') != 'real-pkg'
                    or profile.get('arch') != arch
                    or profile.get('package_name') != 'com.termux.rafacodephi'):
                raise ValueError('wrong producer package, profile or architecture: ' + arch)
            for name in ('bin/apt', 'bin/apt-get', 'bin/dpkg', 'bin/bash', 'bin/ninja'):
                if not installed.read(name).startswith(b'\x7fELF'):
                    raise ValueError('expected native executable, got stub: ' + name)
            status = installed.read('var/lib/dpkg/status').decode('utf-8')
            if not any('Package: ninja' in stanza.splitlines()
                       and 'Status: install ok installed' in stanza.splitlines()
                       for stanza in status.split('\n\n')):
                raise ValueError('Ninja not installed in source package status: ' + arch)

        for name, value in fields.items():
            if name in merged and merged[name] != value:
                raise ValueError('conflicting producer manifest field: ' + name)
            merged[name] = value
        parents[arch] = digest(artifact)
        payloads[arch] = payload

    output.mkdir(parents=True, exist_ok=False)
    for arch, payload in payloads.items():
        (output / ('rafcodephi-bootstrap-' + arch + '.zip')).write_bytes(payload)
    (output / 'RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt').write_text(
        ''.join(name + '=' + value + '\n' for name, value in sorted(merged.items())),
        encoding='utf-8')
    seal_signed_snapshot(output, sealer, key, parents)
    print('REAL_APT_NINJA_PRODUCER_PAIR_IMPORTED=PASS_SCOPED source_build_reused=true')


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    for arg in ('inputs', 'output', 'sealer', 'key'):
        p.add_argument('--' + arg, required=True)
    p.add_argument('--producer-artifacts', action='store_true')
    a = p.parse_args()
    if a.producer_artifacts:
        prepare_from_producer_artifacts(a.inputs, a.output, a.sealer, a.key)
    else:
        prepare(a.inputs, a.output, a.sealer, a.key)
