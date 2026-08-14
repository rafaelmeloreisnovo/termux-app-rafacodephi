#!/usr/bin/env python3
from __future__ import annotations
import argparse
import hashlib
import json
import shutil
import struct
import subprocess
import sys
import zipfile
from pathlib import Path

NO_INDEX = 0xFFFFFFFF
RES_STRING_POOL_TYPE = 0x0001
RES_XML_START_ELEMENT_TYPE = 0x0102
UTF8_FLAG = 0x00000100
TYPE_STRING = 0x03


class AxmlError(ValueError):
    pass


def u16(buf, off):
    return struct.unpack_from("<H", buf, off)[0]


def u32(buf, off):
    return struct.unpack_from("<I", buf, off)[0]


def chunk_header(buf, off):
    if off + 8 > len(buf):
        raise AxmlError("truncated chunk header")
    chunk_type, header_size, size = struct.unpack_from("<HHI", buf, off)
    if header_size < 8 or size < header_size or off + size > len(buf):
        raise AxmlError(
            f"invalid chunk at {off}: type={chunk_type:#x} "
            f"header={header_size} size={size}"
        )
    return chunk_type, header_size, size


def read_utf8_len(buf, pos):
    value = buf[pos]
    pos += 1
    if value & 0x80:
        value = ((value & 0x7F) << 8) | buf[pos]
        pos += 1
    return value, pos


def read_utf16_len(buf, pos):
    value = u16(buf, pos)
    pos += 2
    if value & 0x8000:
        value = ((value & 0x7FFF) << 16) | u16(buf, pos)
        pos += 2
    return value, pos


def parse_string_pool(buf, off):
    chunk_type, header_size, size = chunk_header(buf, off)
    if chunk_type != RES_STRING_POOL_TYPE or header_size < 28:
        raise AxmlError("expected Android string pool")
    count, _style_count, flags, strings_start, _styles_start = struct.unpack_from(
        "<IIIII", buf, off + 8
    )
    offsets_base = off + header_size
    if offsets_base + 4 * count > off + size:
        raise AxmlError("string offset table out of bounds")
    offsets = [u32(buf, offsets_base + 4 * i) for i in range(count)]
    strings_base = off + strings_start
    strings = []
    for relative in offsets:
        pos = strings_base + relative
        if pos >= off + size:
            raise AxmlError("string out of bounds")
        if flags & UTF8_FLAG:
            _utf16_chars, pos = read_utf8_len(buf, pos)
            byte_len, pos = read_utf8_len(buf, pos)
            raw = buf[pos : pos + byte_len]
            strings.append(raw.decode("utf-8", errors="strict"))
        else:
            char_len, pos = read_utf16_len(buf, pos)
            raw = buf[pos : pos + char_len * 2]
            strings.append(raw.decode("utf-16le", errors="strict"))
    return strings, size


def string_at(strings, index):
    return None if index == NO_INDEX else strings[index]


def parse_manifest_axml(buf):
    root_type, root_header_size, root_size = chunk_header(buf, 0)
    if root_type != 0x0003:
        raise AxmlError(f"not Android binary XML: root={root_type:#x}")

    pos = root_header_size
    strings = None
    while pos < root_size:
        chunk_type, header_size, size = chunk_header(buf, pos)
        if chunk_type == RES_STRING_POOL_TYPE and strings is None:
            strings, _ = parse_string_pool(buf, pos)
        elif chunk_type == RES_XML_START_ELEMENT_TYPE:
            if strings is None:
                raise AxmlError("start element before string pool")
            if header_size < 16 or size < 36:
                raise AxmlError("start element too small")

            name_index = u32(buf, pos + 20)
            element_name = string_at(strings, name_index)
            attribute_start = u16(buf, pos + 24)
            attribute_size = u16(buf, pos + 26)
            attribute_count = u16(buf, pos + 28)
            if element_name == "manifest":
                if attribute_size < 20:
                    raise AxmlError("attribute size too small")
                attrs = {}
                attribute_base = pos + 16 + attribute_start
                for i in range(attribute_count):
                    attr = attribute_base + i * attribute_size
                    if attr + 20 > pos + size:
                        raise AxmlError("attribute out of bounds")
                    ns_index, attr_name_index, raw_index = struct.unpack_from(
                        "<III", buf, attr
                    )
                    data_type = buf[attr + 15]
                    data = u32(buf, attr + 16)
                    attr_name = string_at(strings, attr_name_index)
                    raw_value = string_at(strings, raw_index)
                    if raw_value is not None:
                        value = raw_value
                    elif data_type == TYPE_STRING:
                        value = string_at(strings, data)
                    else:
                        value = data
                    attrs[attr_name] = {
                        "value": value,
                        "data_type": data_type,
                        "namespace": string_at(strings, ns_index),
                    }

                def value(name):
                    return attrs.get(name, {}).get("value")

                return {
                    "package": value("package"),
                    "version_name": value("versionName"),
                    "version_code": value("versionCode"),
                    "compile_sdk_version": value("compileSdkVersion"),
                    "attributes": attrs,
                }
        pos += size
    raise AxmlError("manifest start element not found")


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command):
    completed = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    return completed.returncode, completed.stdout


def jarsigner_check(apk):
    executable = shutil.which("jarsigner")
    if not executable:
        return {"status": "TOKEN_VAZIO_TOOL_MISSING", "tool": None}
    returncode, output = run([executable, "-verify", "-certs", str(apk)])
    return {
        "status": "VERIFIED" if returncode == 0 else "FAILED",
        "tool": executable,
        "returncode": returncode,
        "summary": output[-4000:],
    }


def apksigner_check(apk, explicit=None):
    executable = explicit or shutil.which("apksigner")
    if not executable:
        return {
            "status": "TOKEN_VAZIO_TOOL_MISSING",
            "tool": None,
            "schemes": {"v1": None, "v2": None, "v3": None, "v3_1": None, "v4": None},
        }
    returncode, output = run(
        [executable, "verify", "--verbose", "--print-certs", str(apk)]
    )
    labels = {
        "Verified using v1 scheme (JAR signing):": "v1",
        "Verified using v2 scheme (APK Signature Scheme v2):": "v2",
        "Verified using v3 scheme (APK Signature Scheme v3):": "v3",
        "Verified using v3.1 scheme (APK Signature Scheme v3.1):": "v3_1",
        "Verified using v4 scheme (APK Signature Scheme v4):": "v4",
    }
    schemes = {}
    for line in output.splitlines():
        stripped = line.strip()
        for prefix, key in labels.items():
            if stripped.startswith(prefix):
                schemes[key] = stripped[len(prefix) :].strip().lower() == "true"
    for key in ("v1", "v2", "v3", "v3_1", "v4"):
        schemes.setdefault(key, None)
    return {
        "status": "VERIFIED" if returncode == 0 else "FAILED",
        "tool": executable,
        "returncode": returncode,
        "schemes": schemes,
        "summary": output[-6000:],
    }


def load_contract(path):
    if not path:
        return {}
    with path.open(encoding="utf-8") as stream:
        return json.load(stream)


def main():
    parser = argparse.ArgumentParser(description="Fail-closed APK artifact evidence gate")
    parser.add_argument("apk", type=Path)
    parser.add_argument("--contract", type=Path)
    parser.add_argument("--apksigner")
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()

    if not args.apk.is_file():
        raise SystemExit(f"APK not found: {args.apk}")

    contract = load_contract(args.contract)
    expected = contract.get("expected", {})
    requirements = contract.get("requirements", {})
    checks = []

    def check(name, observed, expected_value, required=True):
        if expected_value is None:
            checks.append(
                {
                    "name": name,
                    "status": "TOKEN_VAZIO_EXPECTATION",
                    "observed": observed,
                    "expected": None,
                    "required": required,
                }
            )
            return not required
        passed = observed == expected_value
        checks.append(
            {
                "name": name,
                "status": "PASS" if passed else "FAIL",
                "observed": observed,
                "expected": expected_value,
                "required": required,
            }
        )
        return passed

    digest = sha256_file(args.apk)
    zip_crc = False
    manifest = None
    zip_error = None
    try:
        with zipfile.ZipFile(args.apk) as archive:
            bad_entry = archive.testzip()
            zip_crc = bad_entry is None
            if bad_entry is not None:
                zip_error = f"CRC failure: {bad_entry}"
            manifest = parse_manifest_axml(archive.read("AndroidManifest.xml"))
    except Exception as exc:
        zip_error = f"{type(exc).__name__}: {exc}"

    jar_signature = jarsigner_check(args.apk)
    apk_signature = apksigner_check(args.apk, args.apksigner)

    required_all = True
    required_all &= check("zip_crc", zip_crc, True, True)
    required_all &= check(
        "package",
        manifest.get("package") if manifest else None,
        expected.get("package"),
        expected.get("package") is not None,
    )
    required_all &= check(
        "version_name",
        manifest.get("version_name") if manifest else None,
        expected.get("version_name"),
        expected.get("version_name") is not None,
    )
    if expected.get("sha256") is not None:
        required_all &= check("sha256", digest, expected["sha256"], True)
    else:
        check("sha256", digest, None, False)

    if requirements.get("jar_v1"):
        required_all &= check("jar_v1_signature", jar_signature["status"], "VERIFIED", True)
    for scheme in ("v1", "v2", "v3", "v3_1", "v4"):
        if requirements.get(f"apk_signature_{scheme}"):
            required_all &= check(
                f"apk_signature_{scheme}",
                apk_signature["schemes"].get(scheme),
                True,
                True,
            )

    build_provenance = contract.get("build_provenance", {})
    source_commit = build_provenance.get("source_commit")
    build_receipt_sha256 = build_provenance.get("build_receipt_sha256")
    provenance_claim_allowed = bool(
        required_all and source_commit and build_receipt_sha256
    )
    status = "PASS" if required_all else "FAIL"
    if required_all and not provenance_claim_allowed:
        status = "VERIFIED_LIMITED"

    receipt = {
        "schema": "rafaelia.apk-evidence-receipt.v1",
        "artifact": {
            "path": str(args.apk),
            "bytes": args.apk.stat().st_size,
            "sha256": digest,
        },
        "zip": {"crc_full_pass": zip_crc, "error": zip_error},
        "manifest": manifest,
        "jar_signature": jar_signature,
        "apk_signature": apk_signature,
        "contract": str(args.contract) if args.contract else None,
        "checks": checks,
        "status": status,
        "provenance_claim_allowed": provenance_claim_allowed,
        "claim_allowed": False,
        "invariant": "ARTIFACT_IDENTITY != BUILD_PROVENANCE != RUNTIME_EVIDENCE != CLAIM",
        "token_vazio": [],
    }
    if apk_signature["status"] == "TOKEN_VAZIO_TOOL_MISSING":
        receipt["token_vazio"].append("TV-APK-SIGNATURE-V2V3V4-TOOL")
    if not source_commit:
        receipt["token_vazio"].append("TV-SOURCE-COMMIT")
    if not build_receipt_sha256:
        receipt["token_vazio"].append("TV-BUILD-RECEIPT-SHA256")

    text = json.dumps(receipt, ensure_ascii=False, indent=2, sort_keys=True)
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(text + "\n", encoding="utf-8")
    else:
        print(text)
    return 0 if required_all else 2


if __name__ == "__main__":
    sys.exit(main())
