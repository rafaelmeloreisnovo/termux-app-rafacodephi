"""Host fixtures execute shell control flow. They do not prove Android runtime."""
from pathlib import Path
import os
import hashlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class DeviceProbeControlTests(unittest.TestCase):
    def probe(self, mode="--ninja-smoke", *, fail_stage="", ninja_mode="good"):
        with tempfile.TemporaryDirectory(prefix="raf-device-fixture-") as temp:
            p = Path(temp)
            for name in ("bin", "tmp", "etc/apt/sources.list.d", "etc/apt/keyrings", "etc/apt/apt.conf.d"):
                (p / name).mkdir(parents=True, exist_ok=True)
            key = p / "etc/apt/keyrings/rafcodephi-archive-key.gpg"
            key.write_bytes(b"synthetic-public-key")
            (p / "etc/apt/sources.list.d/termux.sources").write_text(
                "Enabled: yes\nTypes: deb\nArchitectures: arm aarch64\n" + f"Signed-By: {key}\n")
            (p / "etc/apt/apt.conf.d/00rafcodephi-repository-block").write_text("// inactive\n")
            stubs = {
                "apt-get": 'echo "apt:$*" >> "$TRACE"\n'
                    'case " $* " in *" update "*) [ "$FAIL_STAGE" != update ] || exit 41;;'
                    ' *" install "*) [ "$FAIL_STAGE" != install ] || exit 42;; esac\n',
                "dpkg": 'echo "dpkg:$*" >> "$TRACE"\n'
                    'if [ "$1" = -s ]; then\n'
                    '  echo "Status: install ok installed"\n'
                    '  [ "$FAIL_STAGE" != status ] || exit 43\nfi\n',
                "ninja": 'echo "ninja:$*" >> "$TRACE"\n'
                    'if [ "$1" = --version ]; then echo 1.13.2-fixture; exit 0; fi\n'
                    '[ "$NINJA_MODE" != fail ] || exit 44\n'
                    'if [ "$NINJA_MODE" != version_only ]; then\n'
                    '  if [ "$NINJA_MODE" = wrong_output ]; then echo wrong > "$2/proof.txt";\n'
                    '  else printf rafcodephi_ninja_contract_v1 > "$2/proof.txt"; fi\nfi\n',
            }
            for name, body in stubs.items():
                f = p / "bin" / name
                f.write_text("#!/bin/sh\nset -eu\n" + body + "\nexit 0\n")
                f.chmod(0o755)
            (p / "bin/sha256sum").symlink_to(shutil.which("sha256sum"))
            source = (ROOT / "scripts/device_signed_apt_smoke.sh").read_text()
            # Fixture-only relocation; the production probe has no prefix override.
            original = "P=/data/data/com.termux.rafacodephi/files/usr"
            self.assertEqual(source.count(original), 1)
            rendered = source.replace(original, "P=" + str(p), 1)
            env = dict(os.environ, PREFIX=str(p), FAIL_STAGE=fail_stage, NINJA_MODE=ninja_mode,
                       TRACE=str(p / "trace"), RAF_APT_TRUSTED_KEY_SHA256=hashlib.sha256(key.read_bytes()).hexdigest())
            proc = subprocess.run(["sh", "-s", "--", mode], input=rendered,
                                  env=env, text=True, capture_output=True)
            trace = (p / "trace").read_text() if (p / "trace").exists() else ""
            self.assertEqual(list((p / "tmp").iterdir()), [], "temporary build must be cleaned")
            return proc, trace

    def test_check_is_local_only(self):
        proc, trace = self.probe("--check")
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("remote_repo=NOT_TESTED", proc.stdout)
        self.assertNotIn("ninja:", trace)

    def test_fixture_graph_output_required(self):
        proc, trace = self.probe()
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("graph_executed=true", proc.stdout)
        self.assertNotIn("apt:install", trace)

    def test_failed_graph_cannot_pass(self):
        for mode in ("fail", "version_only", "wrong_output"):
            with self.subTest(mode=mode):
                proc, _ = self.probe(ninja_mode=mode)
                self.assertNotEqual(proc.returncode, 0)
                self.assertNotIn("DEVICE_NINJA=PASS", proc.stdout)

    def test_install_stops_after_each_failed_prerequisite(self):
        for stage in ("update", "install", "status"):
            with self.subTest(stage=stage):
                proc, trace = self.probe("--install-ninja", fail_stage=stage)
                self.assertNotEqual(proc.returncode, 0)
                self.assertNotIn("DEVICE_NINJA=PASS", proc.stdout)
                self.assertNotIn("ninja:", trace)
                if stage == "update":
                    self.assertNotIn("apt:install", trace)


if __name__ == "__main__":
    unittest.main()
