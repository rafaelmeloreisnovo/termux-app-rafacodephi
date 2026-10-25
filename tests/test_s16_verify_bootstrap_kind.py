"""HOSTED_TEST_ONLY: synthetic diagnostic bootstrap custody negative fixtures."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from zipfile import ZipFile

script=Path(__file__).resolve().parents[1]/"scripts"/"s16_verify_bootstrap_kind.py"
spec=importlib.util.spec_from_file_location("s16bootstrap",script)
gate=importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)

class TestS16Bootstrap(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.d=Path(self.temp.name)
        self.good={
            "BOOTSTRAP_INFO":"TERMUX_PACKAGE_NAME=com.termux.rafacodephi\nTERMUX_PAGE_SIZE=16384\nRAFCODEPHI_BOOTSTRAP=local-ci\n",
            "bin/pkg":"#!/system/bin/sh\n",
            "bin/proot":"#!/system/bin/sh\n",
            "bin/apt":"#!/system/bin/sh\necho 'real apt backend is not installed yet'\n"
        }
    def make(self,arch,alter=None):
        values=dict(self.good)
        if alter:alter(values)
        for prefix in ("bootstrap","rewritten-bootstrap"):
            p=self.d/f"{prefix}-{arch}.zip"
            with ZipFile(p,"w") as z:
                for k,v in values.items():z.writestr(k,v)
    def make_all(self,alter=None,which=None):
        for arch in gate.ARCHES:
            self.make(arch,alter if arch==which else None)
    def test_bridge_only(self):
        self.make_all()
        d=gate.check(self.d)
        self.assertEqual(d["status"],"PASS_BRIDGE_ONLY_DIAGNOSTIC")
        self.assertFalse(d["source_built_package_runtime"])
        self.assertEqual(len(d["receipts"]),4)
    def test_missing_source(self):
        self.make_all()
        (self.d/"bootstrap-arm.zip").unlink()
        with self.assertRaisesRegex(RuntimeError,"BOOTSTRAP_PAIR_MISSING"):gate.check(self.d)
    def test_unauthorized_package(self):
        self.make_all(lambda d:d.update(BOOTSTRAP_INFO=d["BOOTSTRAP_INFO"].replace("com.termux.rafacodephi","com.termux")),which="arm")
        with self.assertRaisesRegex(RuntimeError,"PACKAGE_METADATA_MISMATCH"):gate.check(self.d)
    def test_unrecognized_apt(self):
        self.make_all(lambda d:d.update({"bin/apt":"echo false"}),which="arm")
        with self.assertRaisesRegex(RuntimeError,"UNRECOGNIZED_APT_BRIDGE"):gate.check(self.d)
    def test_wrong_embedded(self):
        self.make_all()
        with ZipFile(self.d/"rewritten-bootstrap-arm.zip","a") as z:z.writestr("altered","new")
        with self.assertRaisesRegex(RuntimeError,"EMBEDDED_SOURCE_BYTES_MISMATCH"):gate.check(self.d)
    def test_reject_non_local_ci(self):
        self.make_all(lambda d:d.update(BOOTSTRAP_INFO=d["BOOTSTRAP_INFO"].replace("local-ci","source-built-real")),which="arm")
        with self.assertRaisesRegex(RuntimeError,"PROVENANCE_NOT_DIAGNOSTIC"):gate.check(self.d)

if __name__=="__main__":unittest.main()
