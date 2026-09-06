"""Safety falsifiers and real native CTI integration; no mocked inference."""
import copy
import json
import os
from pathlib import Path
import sys
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from atlas_contract_io import canonical, decode_json, load_json, sha256, validate_shape
from atlas_novo_context_adapter import ROUTE_ID, run_adapter
from validate_atlas_llm_navigation_contract import DEFAULT_FIXTURE, validate_fixture


class EnvelopeGate(unittest.TestCase):
    def check(self, data):
        errors = []
        validate_fixture(data, errors)
        return errors

    def test_baseline_remains_valid(self):
        self.assertEqual(self.check(load_json(DEFAULT_FIXTURE)), [])

    def test_missing_required_fields_rejected(self):
        data = load_json(DEFAULT_FIXTURE)
        for field in ("query", "envelope_id", "source_refs", "token_vazio"):
            broken = copy.deepcopy(data); broken.pop(field)
            with self.subTest(field=field):
                self.assertTrue(self.check(broken))

    def test_non_object_and_bad_nested_type_rejected(self):
        for data in ([], None, {"claim_allowed": False}):
            self.assertTrue(self.check(data))
        data = load_json(DEFAULT_FIXTURE); data["model_backend"] = []
        self.assertTrue(self.check(data))

    def test_public_raw_body_is_not_an_envelope_field(self):
        data = load_json(DEFAULT_FIXTURE); data["private_body"] = "unexpected"
        self.assertTrue(self.check(data))

    def test_false_not_zero_and_invalid_digest_rejected(self):
        data = load_json(DEFAULT_FIXTURE); data["claim_allowed"] = 0
        self.assertTrue(self.check(data))
        data = load_json(DEFAULT_FIXTURE); data["query"]["text_sha256"] = "not-a-hash"
        self.assertTrue(self.check(data))

    def test_dangling_hit_and_duplicate_identity_rejected(self):
        data = load_json(DEFAULT_FIXTURE)
        data["cti_hits"] = [{"source_ref": "missing", "status": "HIT"}]
        self.assertTrue(self.check(data))
        data["cti_hits"] = []; data["source_refs"] *= 2
        self.assertTrue(self.check(data))

    def test_duplicate_keys_and_nonfinite_json_rejected(self):
        for raw in ('{"a":1,"a":2}', '{"a":NaN}', '{"a":Infinity}'):
            with self.assertRaises(ValueError):
                decode_json(raw)

    def test_unknown_assertion_fails_closed(self):
        errors = []
        validate_shape("a", {"type": "string", "unimplementedGate": True}, errors)
        self.assertTrue(errors)


@unittest.skipUnless(os.getenv("ATLAS_CTI_NATIVE") and os.getenv("ATLAS_CTI_SOURCE_ROOT") and
                     os.getenv("ATLAS_MAPA_ROOT"), "native producer and Mapa inputs not supplied")
class NativeCTIAdapter(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.data = self.root / "data"; self.data.mkdir()
        self.work = self.root / "work"; self.work.mkdir()
        self.raw = {"id": "fixture-conversation-001", "title": "synthetic retrieval control", "mapping": {
            "node-one": {"message": {"id": "message-one", "author": {"role": "user"},
                        "content": {"parts": ["rarebeacon color code AZURE-STONE-731."]}}},
            "node-two": {"message": {"id": "message-two", "author": {"role": "assistant"},
                        "content": {"parts": ["Public control without that keyword."]}}}}}
        self.source = self.data / "conversation.json"; self.source.write_bytes(canonical(self.raw))
        self.lineage = self.data / "lineage.json"
        self.lineage.write_bytes(canonical({"predecessor_ids": ["L:ATLAS-NOVO-RMRCTI-LLM-NAV-20260906"]}))
        route = Path(os.environ["ATLAS_MAPA_ROOT"]) / "indices/deltas/ATLAS_X_NOVO_RMRCTI_LLM_NAV_20260906.md"
        self.manifest = {"schema": "rafaelia.atlas_novo_source.v1", "claim_allowed": False,
            "disclosure": "LOCAL_PRIVATE_CONTEXT",
            "route": {"route_id": ROUTE_ID, "path": str(route), "content_sha256": sha256(route.read_bytes()),
                      "commit": subprocess.check_output(["git", "-C", os.environ["ATLAS_MAPA_ROOT"],
                                                          "rev-parse", "HEAD"]).decode().strip()},
            "source": {"source_id": "fixture-source", "authority": "Synthetic/Fixture",
                       "path": str(self.source), "content_sha256": sha256(self.source.read_bytes())},
            "longitudinal": {"path": str(self.lineage), "content_sha256": sha256(self.lineage.read_bytes())}}
        self.manifest_path = self.data / "manifest.json"
        self.save_manifest()

    def tearDown(self):
        self.temp.cleanup()

    def save_manifest(self):
        self.manifest_path.write_bytes(canonical(self.manifest))

    def run_control(self, query="rarebeacon", enabled=True, name="run", top_k=5):
        self.out = self.root / "runs" / name
        return run_adapter(self.manifest_path, query, Path(os.environ["ATLAS_CTI_SOURCE_ROOT"]),
                           Path(os.environ["ATLAS_CTI_NATIVE"]), self.out, self.work, enabled, top_k,
                           assembled_at="2026-09-06T00:00:00Z")

    def test_rare_fact_requires_enabled_retrieval(self):
        before = self.source.read_bytes()
        result = self.run_control()
        self.assertEqual(result["hit_count"], 1)
        self.assertIn("AZURE-STONE-731", (self.out / "chunks.json").read_text())
        off = self.run_control(enabled=False, name="off")
        self.assertEqual(off["status"], "disabled")
        self.assertFalse((self.out / "context_bundle.json").exists())
        self.assertNotIn("AZURE-STONE-731", (self.out / "chunks.json").read_text())
        self.assertEqual(self.source.read_bytes(), before)
        self.assertFalse(result["model_executed"])
        self.assertFalse(result["weights_modified"])

    def test_no_hit_does_not_fabricate_bundle(self):
        result = self.run_control(query="unfindablezzqxmarker")
        self.assertEqual(result["status"], "no_hits")
        self.assertEqual(result["hit_count"], 0)
        self.assertFalse((self.out / "context_bundle.json").exists())

    def test_source_hash_tamper_rejected(self):
        self.source.write_bytes(self.source.read_bytes() + b" ")
        with self.assertRaisesRegex(ValueError, "source_hash_mismatch"):
            self.run_control()
        self.assertFalse(self.out.exists())

    def test_route_and_disclosure_fail_closed(self):
        self.manifest["disclosure"] = "PUBLIC"; self.save_manifest()
        with self.assertRaisesRegex(ValueError, "disclosure_not_authorized"):
            self.run_control()
        self.assertFalse(self.out.exists())

    def test_append_only_and_deterministic_selection(self):
        first = self.run_control(name="first")
        first_envelope = (self.out / "envelope.json").read_bytes()
        second = self.run_control(name="second")
        self.assertEqual(first_envelope, (self.out / "envelope.json").read_bytes())
        self.assertEqual(first["run_id"], second["run_id"])
        with self.assertRaisesRegex(ValueError, "output_exists"):
            self.run_control(name="first")

    def test_query_limit_before_native_execution(self):
        with self.assertRaisesRegex(ValueError, "query_byte_limit"):
            self.run_control(query="x" * 4097)
        self.assertFalse(self.out.exists())

    def test_multibyte_context_respects_total_budget(self):
        self.raw["mapping"] = {f"node-{i}": {"message": {"id": f"message-{i}",
            "author": {"role": "user"}, "content": {"parts": [f"rarebeacon segment{i:02d} " + "界" * 500]}}}
            for i in range(20)}
        self.source.write_bytes(canonical(self.raw))
        self.manifest["source"]["content_sha256"] = sha256(self.source.read_bytes()); self.save_manifest()
        result = self.run_control(top_k=20)
        self.assertGreater(result["hit_count"], 5)
        chunks = load_json(self.out / "chunks.json")
        self.assertLessEqual(sum(len(c["content"].encode("utf-8")) for c in chunks), 2500)
        self.assertEqual(result["context_bytes"], sum(len(c["content"].encode("utf-8")) for c in chunks))

    def test_restricted_query_is_blocked(self):
        result = self.run_control(query="-----BEGIN PRIVATE KEY----- synthetic-test-only")
        self.assertEqual(result["status"], "privacy_blocked")
        self.assertFalse(result["bundle_emitted"])

    def test_pii_redacted_before_context(self):
        self.raw["mapping"]["node-one"]["message"]["content"]["parts"] = ["rarebeacon contact tester@example.org"]
        self.source.write_bytes(canonical(self.raw))
        self.manifest["source"]["content_sha256"] = sha256(self.source.read_bytes()); self.save_manifest()
        result = self.run_control()
        self.assertEqual(result["privacy_redacted_hits"], 1)
        self.assertNotIn("tester@example.org", (self.out / "chunks.json").read_text())

    def test_ambiguous_redacted_source_rejected(self):
        self.raw["mapping"]["node-two"]["message"]["author"]["role"] = "user"
        self.raw["mapping"]["node-two"]["message"]["content"] = copy.deepcopy(self.raw["mapping"]["node-one"]["message"]["content"])
        self.source.write_bytes(canonical(self.raw))
        self.manifest["source"]["content_sha256"] = sha256(self.source.read_bytes()); self.save_manifest()
        with self.assertRaisesRegex(ValueError, "hit_source_ambiguous"):
            self.run_control()


if __name__ == "__main__":
    unittest.main()
