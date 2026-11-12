"""Regression tests for fail-closed APK output metadata discovery."""
import json
import tempfile
import unittest
from pathlib import Path

from scripts.verify_apk_bootstrap_payload import discover_packaged_apk_directory as resolve


class ApkDiscoveryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.build = Path(self.tmp.name) / 'app' / 'build'
        self.requested = self.build / 'outputs' / 'apk' / 'debug'

    def make(self, folder, files, variant='debug'):
        folder.mkdir(parents=True, exist_ok=True)
        (folder / 'output-metadata.json').write_text(json.dumps({
            'variantName': variant,
            'elements': [{'outputFile': name} for name in files],
        }), encoding='utf-8')
        for name in files:
            (folder / name).write_bytes(b'fake-apk')

    def test_nested_debug(self):
        expected = self.build / 'outputs' / 'apk' / 'apt-android-7' / 'debug'
        self.make(expected, ['termux-app_v1_arm64-v8a.apk'])
        self.assertEqual(resolve(self.requested, 'v1'), expected)

    def test_intermediates_fallback(self):
        expected = self.build / 'intermediates' / 'apk' / 'apt-android-7' / 'debug'
        self.make(expected, ['termux-app_v1_arm64-v8a.apk'])
        self.assertEqual(resolve(self.requested, 'v1'), expected)

    def test_ambiguous_directories_fail_closed(self):
        for item in ('a', 'b'):
            self.make(self.build / 'outputs' / 'apk' / item / 'debug',
                      ['termux-app_v1_arm64-v8a.apk'])
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            resolve(self.requested, 'v1')

    def test_stale_tag_not_accepted(self):
        self.make(self.build / 'outputs' / 'apk' / 'apt-android-7' / 'debug',
                  ['termux-app_old_arm64-v8a.apk'])
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            resolve(self.requested, 'v1')

    def test_release_not_accepted_as_debug(self):
        self.make(self.build / 'outputs' / 'apk' / 'apt-android-7' / 'release',
                  ['termux-app_v1_universal.apk'], variant='release')
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            resolve(self.requested, 'v1')

    def test_missing_apk_not_accepted(self):
        folder = self.build / 'outputs' / 'apk' / 'apt-android-7' / 'debug'
        name = 'termux-app_v1_arm64-v8a.apk'
        self.make(folder, [name])
        (folder / name).unlink()
        with self.assertRaisesRegex(ValueError, 'missing/empty'):
            resolve(self.requested, 'v1')

    def test_unsafe_metadata_path_not_accepted(self):
        folder = self.build / 'outputs' / 'apk' / 'apt-android-7' / 'debug'
        folder.mkdir(parents=True)
        (folder / 'output-metadata.json').write_text(
            json.dumps({'elements': [{'outputFile': '../oops.apk'}]}),
            encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'unsafe'):
            resolve(self.requested, 'v1')

    def test_unexpected_path_not_accepted(self):
        with self.assertRaisesRegex(ValueError, 'unsupported'):
            resolve(self.build / 'outputs' / 'release', 'v1')


if __name__ == '__main__':
    unittest.main()
