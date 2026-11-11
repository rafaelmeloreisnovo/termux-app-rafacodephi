#!/usr/bin/env python3
"""Verify exact ZIP bytes exported by the APK's native bootstrap JNI library.

Artifact payload gate only; APK signing and Android execution are separate gates.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

SPECS = {'armeabi-v7a': (1, 40, 'arm'), 'arm64-v8a': (2, 183, 'aarch64')}


def embedded_zip(elf, abi):
    elf_class, machine, _ = SPECS[abi]
    if elf[:4] != b'\x7fELF' or elf[4:6] != bytes((elf_class, 1)):
        raise ValueError('native bootstrap ELF class/endian mismatch')
    def unpack(fmt, offset):
        try:
            return struct.unpack_from('<' + fmt, elf, offset)
        except struct.error as error:
            raise ValueError('truncated ELF structure') from error
    if unpack('H', 18)[0] != machine:
        raise ValueError('native bootstrap ELF machine mismatch')
    if elf_class == 2:
        phoff, shoff = unpack('QQ', 32)
        phent, phnum, shent, shnum = unpack('HHHH', 54)
        shformat, symformat = 'IIQQQQIIQQ', 'IBBHQQ'
    else:
        phoff, shoff = unpack('II', 28)
        phent, phnum, shent, shnum = unpack('HHHH', 42)
        shformat, symformat = 'IIIIIIIIII', 'IIIBBH'
    if phnum > 4096 or shnum > 65535:
        raise ValueError('ELF table exceeds bound')
    sections = [unpack(shformat, shoff + i * shent) for i in range(shnum)]
    def section_data(section):
        offset, size = section[4:6]
        if offset + size > len(elf):
            raise ValueError('ELF section exceeds file')
        return elf[offset:offset + size]
    symbols = {}
    for section in sections:
        if section[1] != 11:  # SHT_DYNSYM
            continue
        if section[6] >= len(sections):
            raise ValueError('ELF string table link invalid')
        strings = section_data(sections[section[6]])
        table = section_data(section)
        entry_size = section[9]
        if entry_size < struct.calcsize('<' + symformat) or len(table) % entry_size:
            raise ValueError('invalid ELF symbol table')
        for offset in range(0, len(table), entry_size):
            sym = struct.unpack_from('<' + symformat, table, offset)
            name_offset = sym[0]
            if name_offset >= len(strings):
                raise ValueError('ELF symbol name exceeds string table')
            name = strings[name_offset:].split(b'\0', 1)[0]
            if name in (b'blob', b'blob_size'):
                if name in symbols:
                    raise ValueError('duplicate bootstrap symbol')
                symbols[name] = sym[4] if elf_class == 2 else sym[1]
    if set(symbols) != {b'blob', b'blob_size'}:
        raise ValueError('native bootstrap symbols missing')
    def slice_at(address, size):
        for i in range(phnum):
            p = phoff + i * phent
            if unpack('I', p)[0] != 1:
                continue
            if elf_class == 2:
                offset, virtual = unpack('QQ', p + 8)
                file_size = unpack('Q', p + 32)[0]
            else:
                offset, virtual = unpack('II', p + 4)
                file_size = unpack('I', p + 16)[0]
            relative = address - virtual
            if relative >= 0 and relative + size <= file_size and offset + relative + size <= len(elf):
                return elf[offset + relative:offset + relative + size]
        raise ValueError('bootstrap symbol exceeds loadable ELF bytes')
    size = struct.unpack('<I', slice_at(symbols[b'blob_size'], 4))[0]
    if size == 0 or size > 512 * 1024 * 1024:
        raise ValueError('invalid native bootstrap size')
    return slice_at(symbols[b'blob'], size)


def verify(apk, bootstraps, abis):
    results = []
    with zipfile.ZipFile(apk) as package:
        for abi in abis:
            expected = Path(bootstraps[SPECS[abi][2]]).read_bytes()
            member = f'lib/{abi}/libtermux-bootstrap.so'
            actual = embedded_zip(package.read(member), abi)
            if actual != expected:
                raise ValueError('APK_EMBEDDED_BOOTSTRAP_MISMATCH: ' + abi)
            with zipfile.ZipFile(Path(bootstraps[SPECS[abi][2]])) as source:
                profile = json.loads(source.read('BOOTSTRAP_PROFILE.json'))
                results.append({'abi': abi, 'library': member, 'bytes': len(actual),
                                'sha256': hashlib.sha256(actual).hexdigest(),
                                'profile': profile['profile'],
                                'apt_repository_state': profile.get('package_repo_runtime_state')})
    return {'schema': 'rafcodephi.apk-bootstrap-payload/v1', 'status': 'PASS',
            'artifact_sha256': hashlib.sha256(Path(apk).read_bytes()).hexdigest(),
            'payloads': results, 'device_runtime': 'TOKEN_VAZIO', 'claim_allowed': False}


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('apk', type=Path)
    p.add_argument('--bootstrap-arm', required=True)
    p.add_argument('--bootstrap-aarch64', required=True)
    p.add_argument('--abi', action='append', choices=tuple(SPECS))
    p.add_argument('--out', type=Path)
    a = p.parse_args()
    try:
        bootstraps = {'arm': a.bootstrap_arm, 'aarch64': a.bootstrap_aarch64}
        if a.apk.is_dir():
            metadata = json.loads((a.apk / 'output-metadata.json').read_text())
            artifacts = []
            for entry in metadata['elements']:
                name = entry['outputFile']
                if Path(name).name != name:
                    raise ValueError('unsafe APK metadata output path')
                filters = [f['value'] for f in entry.get('filters', []) if f['filterType'] == 'ABI']
                abis = [abi for abi in filters if abi in SPECS] if filters else list(SPECS)
                if abis:
                    artifacts.append(verify(a.apk / name, bootstraps, abis))
            if not artifacts:
                raise ValueError('no canonical ARM APK outputs found')
            result = {'schema': 'rafcodephi.apk-bootstrap-matrix/v1', 'status': 'PASS',
                      'artifacts': artifacts, 'device_runtime': 'TOKEN_VAZIO', 'claim_allowed': False}
        else:
            result = verify(a.apk, bootstraps, a.abi or list(SPECS))
    except (ValueError, KeyError, OSError, zipfile.BadZipFile) as error:
        raise SystemExit(str(error))
    text = json.dumps(result, indent=2) + '\n'
    if a.out:
        a.out.parent.mkdir(parents=True, exist_ok=True)
        a.out.write_text(text)
    else:
        print(text)
