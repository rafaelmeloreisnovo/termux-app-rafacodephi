"""Static-only contract guards; not Android execution evidence."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

class DualEngineContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.schema = json.loads((ROOT / 'contracts/rafcodephi-dual-engine-benchmark.v1.schema.json').read_text())

    def test_closed_contract(self):
        s = self.schema
        self.assertFalse(s['additionalProperties'])
        self.assertEqual(s['properties']['claim_allowed']['const'], False)
        self.assertEqual(s['properties']['engine']['enum'], ['JAVA', 'NATIVE'])
        self.assertEqual(s['properties']['run_state']['enum'], ['NOT_RUN', 'OBSERVED', 'FAILED'])
        self.assertEqual(set(s['required']), set(s['properties']))

    def test_evidence_and_compatibility_fields(self):
        p = self.schema['properties']
        for name in ('engine', 'workload_id', 'workload_version', 'algorithm_id', 'input_seed', 'operations', 'checksum_sha256', 'elapsed_ns', 'artifact_sha256', 'android_api', 'abi', 'evidence_ref'):
            self.assertIn(name, p)
        self.assertEqual(p['android_api']['minimum'], 29)
        self.assertEqual(p['operations']['minimum'], 1)
        self.assertIn('null', p['elapsed_ns']['type'])
        self.assertIn('null', p['checksum_sha256']['type'])

    def test_observed_not_equivalent_to_not_run(self):
        gates = self.schema['allOf']
        self.assertEqual(gates[0]['if']['properties']['run_state']['const'], 'OBSERVED')
        self.assertEqual(gates[0]['then']['properties']['elapsed_ns']['type'], 'integer')
        self.assertEqual(gates[1]['if']['properties']['run_state']['const'], 'NOT_RUN')
        self.assertEqual(gates[1]['then']['properties']['elapsed_ns']['type'], 'null')

    def test_existing_native_path_reused(self):
        for path in ('app/src/main/java/com/termux/app/benchmark/PaBenchmarkRunner.java', 'app/src/main/java/com/termux/app/benchmark/BenchmarkEnvironmentSnapshot.java', 'app/src/main/cpp/freestanding/raf_pa_core.c'):
            self.assertTrue((ROOT / path).is_file(), path)
        doc = (ROOT / 'contracts/RAFCODEPHI_DUAL_ENGINE_BENCHMARK_V1.md').read_text()
        self.assertIn('NOT_IMPLEMENTED', doc)
        self.assertIn('claim_allowed=false', doc)
        self.assertIn('DRAFT/HOLD', doc)

if __name__ == '__main__':
    unittest.main()
