import importlib.util
import struct
import sys
from pathlib import Path

MODULE_PATH = Path(__file__).resolve().parents[1] / "scripts" / "verify_apk_evidence.py"
spec = importlib.util.spec_from_file_location("verify_apk_evidence", MODULE_PATH)
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)


def _utf8_len(value):
    if value < 0x80:
        return bytes([value])
    return bytes([0x80 | (value >> 8), value & 0xFF])


def _string_pool(strings):
    payload = b""
    offsets = []
    for value in strings:
        raw = value.encode("utf-8")
        utf16_chars = len(value.encode("utf-16le")) // 2
        offsets.append(len(payload))
        payload += _utf8_len(utf16_chars) + _utf8_len(len(raw)) + raw + b"\x00"
    header_size = 28
    offset_table = b"".join(struct.pack("<I", offset) for offset in offsets)
    strings_start = header_size + len(offset_table)
    body = (
        struct.pack(
            "<IIIII",
            len(strings),
            0,
            module.UTF8_FLAG,
            strings_start,
            0,
        )
        + offset_table
        + payload
    )
    return struct.pack(
        "<HHI", module.RES_STRING_POOL_TYPE, header_size, 8 + len(body)
    ) + body


def _manifest_start(strings):
    index = {value: i for i, value in enumerate(strings)}
    attrs = []

    def add_attr(name, raw=None, data_type=module.TYPE_STRING, data=0):
        raw_index = module.NO_INDEX if raw is None else index[raw]
        if raw is not None and data_type == module.TYPE_STRING:
            data = index[raw]
        attrs.append(
            struct.pack(
                "<IIIHBBI",
                module.NO_INDEX,
                index[name],
                raw_index,
                8,
                0,
                data_type,
                data,
            )
        )

    add_attr("package", "com.example.app")
    add_attr("versionName", "1.2.3")
    add_attr("versionCode", None, 0x10, 42)
    attr_ext = struct.pack(
        "<IIHHHHHH",
        module.NO_INDEX,
        index["manifest"],
        20,
        20,
        len(attrs),
        0,
        0,
        0,
    )
    node = struct.pack("<II", 1, module.NO_INDEX)
    body = node + attr_ext + b"".join(attrs)
    return struct.pack(
        "<HHI", module.RES_XML_START_ELEMENT_TYPE, 16, 8 + len(body)
    ) + body


def _axml():
    strings = [
        "manifest",
        "package",
        "com.example.app",
        "versionName",
        "1.2.3",
        "versionCode",
    ]
    string_pool = _string_pool(strings)
    start = _manifest_start(strings)
    size = 8 + len(string_pool) + len(start)
    return struct.pack("<HHI", 0x0003, 8, size) + string_pool + start


def test_parse_manifest_axml_extracts_identity():
    result = module.parse_manifest_axml(_axml())
    assert result["package"] == "com.example.app"
    assert result["version_name"] == "1.2.3"
    assert result["version_code"] == 42


def test_parse_manifest_axml_rejects_invalid_root():
    try:
        module.parse_manifest_axml(b"12345678")
    except Exception:
        return
    raise AssertionError("invalid root must fail closed")
