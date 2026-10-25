"""HOSTED_TEST_ONLY: synthetic ZIP tests of S16 gate, not real APK signatures."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from zipfile import ZipFile, ZIP_DEFLATED

script=Path(__file__).resolve().parents[1]/"scripts"/"s16_inspect_apk_canary.py"
spec=importlib.util.spec_from_file_location("s16", script)
gate=importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)

class TestS16Gate(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir=Path(self.tmp.name)
        self.apk=self.dir/"synthetic.apk"
        self.contents={
          "classes.dex":b"dex\nfake-not-a-real-dex",
          "lib/armeabi-v7a/libtermux_rafaelia_middleware_s15.so":b"synthetic-arm",
          "lib/arm64-v8a/libtermux_rafaelia_middleware_s15.so":b"synthetic-arm64",
          "lib/armeabi-v7a/liblibtermux-bootstrap.so":b"synthetic-bootstrap",
        }
    def write(self):
        with ZipFile(self.apk,"w",ZIP_DEFLATED) as z:
            for k,v in self.contents.items(): z.writestr(k,v)
    def check(self, mock, expected=None):
        self.write()
        with patch.object(gate,"command",side_effect=mock):
            if expected:
                with self.assertRaisesRegex(RuntimeError,expected):
                    gate.validate_one(self.apk,"aapt","apksigner","readelf",self.dir)
            else:
                return gate.validate_one(self.apk,"aapt","apksigner","readelf",self.dir)
    @staticmethod
    def helper(cmd):
        a=cmd[0]
        if a=="aapt":return "package: name='com.termux.rafacodephi' versionCode='118'\n"
        if a=="apksigner":return "Verifies\n"
        if a=="readelf":
            if cmd[1]=="-h":
                return "Machine: AArch64" if "arm64-v8a" in cmd[2] else "Machine: ARM"
            return "0x00000001 (NEEDED) Shared library: [libc.so]"
        raise AssertionError(cmd)
    def test_synthetic_positive_only(self):
        x=self.check(self.helper)
        self.assertEqual({v["abi"] for v in x["middleware_elf"]},{"arm64-v8a","armeabi-v7a"})
        self.assertTrue(x["debug_signature_verified_by_apksigner"])
    def test_no_native(self):
        self.contents={k:v for k,v in self.contents.items() if "middleware_s15.so" not in k}
        self.check(self.helper,"PINNED_MIDDLEWARE_NOT_PACKAGED")
    def test_no_bootstrap(self):
        self.contents={k:v for k,v in self.contents.items() if "bootstrap" not in k}
        self.check(self.helper,"BOOTSTRAP_ELF_NOT_PACKAGED")
    def test_no_dex(self):
        self.contents.pop("classes.dex")
        self.check(self.helper,"DEX_MISSING")
    def test_wrong_package(self):
        def wrong(cmd):
            if cmd[0]=="aapt": return "package: name='com.termux'\n"
            return self.helper(cmd)
        self.check(wrong,"PACKAGE_IDENTITY_MISMATCH")
    def test_wrong_machine(self):
        def wrong(cmd):
            if cmd[0]=="readelf" and cmd[1]=="-h":return "Machine: AMD x86-64"
            return self.helper(cmd)
        self.check(wrong,"ELF_MACHINE_MISMATCH")
    def test_bad_external_dependency(self):
        def wrong(cmd):
            if cmd[0]=="readelf" and cmd[1]=="-d":return "0x1 (NEEDED) Shared library: [libbad_external.so]"
            return self.helper(cmd)
        self.check(wrong,"UNREVIEWED_ANDROID_DYNAMIC_DEPS")
    def test_unsigned(self):
        def wrong(cmd):
            if cmd[0]=="apksigner":return ""
            return self.helper(cmd)
        self.check(wrong,"SIGNATURE_RECEIPT_EMPTY")
    def test_bad_zip(self):
        self.apk.write_bytes(b"not a zip")
        with patch.object(gate,"command",side_effect=self.helper):
            with self.assertRaisesRegex(RuntimeError,"BAD_APK_ZIP"):
                gate.validate_one(self.apk,"aapt","apksigner","readelf",self.dir)

if __name__=="__main__":unittest.main()
