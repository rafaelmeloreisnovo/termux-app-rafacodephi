#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build-time only gate. Python is not a pure-runtime dependency."""
import pathlib
import re
import subprocess
import sys
import tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
SRCS = (ROOT / "src/raf_fold.c", ROOT / "src/raf_decimal.c")
MARKER = re.compile(rb"/\* RAF_OPT:PROFILE=(stable|size|speed);UNROLL=(1|4) \*/\n")
OPT = {b"stable": "-O2", b"size": "-Os", b"speed": "-O3"}
BASE = ["-std=c11", "-ffreestanding", "-nostdinc", "-nostdlib", "-fno-builtin",
        "-fno-stack-protector", "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables", "-ffunction-sections",
        "-fdata-sections", "-Wall", "-Wextra", "-Werror", "-pedantic"]
def flags(path):
    raw = pathlib.Path(path).read_bytes()
    if not raw or len(raw) > 65536 or b"\0" in raw or re.search(rb"\?\?[=/'()!<>-]", raw) is not None:
        raise ValueError("unsafe or unbounded source")
    if re.search(rb"\\\r?\n", raw):
        raise ValueError("source line splice is forbidden")
    first = raw.partition(b"\n")
    if not first[1]:
        raise ValueError("no leading profile comment")
    match = MARKER.fullmatch(first[0] + first[1])
    if match is None or b"RAF_OPT:" in first[2]:
        raise ValueError("missing, malformed or duplicate profile marker")
    return [OPT[match.group(1)], "-DRAF_UNROLL=" + match.group(2).decode("ascii")]
def falsifiers():
    with tempfile.TemporaryDirectory() as tmp:
        p = pathlib.Path(tmp) / "probe.c"
        valid = b"/* RAF_OPT:PROFILE=stable;UNROLL=1 */\nint x;\n"
        p.write_bytes(valid)
        assert flags(p) == ["-O2", "-DRAF_UNROLL=1"]
        for bad in (
            b" int x;\n" + valid,
            valid + valid,
            b"/* RAF_OPT:PROFILE=evil;UNROLL=4 */\nint x;\n",
            b"/* RAF_OPT:PROFILE=speed;UNROLL=99 */\nint x;\n",
            b"/* RAF_OPT:PROFILE=stable;UNROLL=1 */\n\\\nint x;\n",
            b"/* RAF_OPT:PROFILE=stable;UNROLL=1 */\n??/\nint x;\n",
            b"/* RAF_OPT:PROFILE=stable;UNROLL=1 */\n??=\nint x;\n",
            valid + b"\0",
        ):
            p.write_bytes(bad)
            try:
                flags(p)
            except ValueError:
                continue
            raise AssertionError("unsafe profile accepted")
    print("PASS_SCOPED_HOST: profile adversarial tests")
def cmd(argv):
    subprocess.run(argv, check=True)
def gate(cc, cross=False):
    falsifiers()
    targets = ("armv7a-linux-androideabi21", "aarch64-linux-android21") if cross else (None,)
    with tempfile.TemporaryDirectory() as tmp:
        for target in targets:
            objects = []
            for index, src in enumerate(SRCS):
                obj = str(pathlib.Path(tmp) / (str(index) + ("-cross" if target else "") + ".o"))
                prefix = [cc] + (["-target", target] if target else [])
                cmd(prefix + BASE + flags(src) + ["-c", str(src), "-o", obj])
                if cross:
                    # readelf is target-agnostic: host nm may not understand ARM objects.
                    symbols = subprocess.run(["readelf", "--wide", "--syms", obj],
                                             check=True, capture_output=True, text=True).stdout
                    unresolved = []
                    for line in symbols.splitlines():
                        fields = line.split()
                        if (len(fields) >= 7 and fields[0].endswith(":")
                                and fields[4] in ("GLOBAL", "WEAK")
                                and fields[6] == "UND"):
                            unresolved.append(fields[7] if len(fields) > 7 else "<unnamed>")
                    if unresolved:
                        raise AssertionError("undefined ARM core symbol: " + ", ".join(unresolved))
                else:
                    undef = subprocess.run(["nm", "-u", obj], check=True,
                                           capture_output=True, text=True).stdout.strip()
                    if undef:
                        raise AssertionError("undefined core symbol: " + undef)
                objects.append(obj)
            if not cross:
                binary = str(pathlib.Path(tmp) / "host-test")
                cmd([cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                     str(ROOT / "tests/test_core.c")] + objects + ["-o", binary])
                cmd([binary])
    print("PASS_SCOPED_" + ("ARM_OBJECTS" if cross else "HOST_OBJECTS") + ": " + cc)
if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "flags":
        print(" ".join(flags(sys.argv[2])))
    elif len(sys.argv) == 3 and sys.argv[1] == "gate":
        gate(sys.argv[2])
    elif len(sys.argv) == 3 and sys.argv[1] == "cross":
        gate(sys.argv[2], True)
    else:
        raise SystemExit("Usage: raf_gate.py flags <source> | gate <cc> | cross <clang>")
