"""Hosted-only negative mutation tests for canonical browser materialization."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "materialize_browser_fail_closed", ROOT/"scripts/materialize_browser_fail_closed.py"
)
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


class MaterializerFailClosedFalsifiers(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.predecessor = MOD.DEFAULT_SOURCE.read_text(encoding="utf-8")
        if cls.predecessor.count(MOD.OLD_BLOCK) != 1:
            raise AssertionError("predecessor downgrade block missing; cannot claim baseline")

    def _run(self, text):
        with tempfile.TemporaryDirectory(prefix="browser_generator_p0_") as d:
            source, output = Path(d)/"Browser.sh", Path(d)/"result.sh"
            source.write_text(text, encoding="utf-8")
            try:
                report = MOD.materialize(source, output)
            except ValueError:
                self.assertFalse(output.exists())
                return False
            self.assertTrue(output.exists())
            materialized = output.read_text(encoding="utf-8")
            self.assertNotIn("[FALLBACK] Usando HTTP para demo", materialized)
            self.assertNotIn("crypto não implementado — usando HTTP para demo", materialized)
            self.assertEqual(report["https_policy"], "FAIL_CLOSED")
            self.assertFalse(report["tls_implemented"])
            # Negative regression guard: #500 removed #503 entropy and bounds.
            self.assertIn('cat > "$D/br_entropy.h"', materialized)
            self.assertIn('BR_RANDOM_FILL(t->random', materialized)
            self.assertIn('if(TLS_INIT(&_TLS)!=0)', materialized)
            self.assertIn('TLS entropy unavailable: aborting before ClientHello', materialized)
            self.assertIn('if(!t||!host||!buf)return 0u;', materialized)
            self.assertIn('cap<92u+hlen', materialized)
            self.assertIn('c<=0x20u||c>=0x7Fu', materialized)
            self.assertIn('if(!chlen)', materialized)
            self.assertIn('TLS ClientHello length invalid', materialized)
            self.assertNotIn('0xDEADBEEF', materialized)
            self.assertNotIn('AI u32 PRNG', materialized)
            self.assertNotIn('LFSR + PHI64', materialized)
            return True

    def test_baseline_materializes_without_downgrade(self):
        self.assertTrue(self._run(self.predecessor))

    def test_missing_known_downgrade_pattern_fails_closed(self):
        mutant = self.predecessor.replace(MOD.OLD_BLOCK, "unknown upstream rewrite", 1)
        self.assertFalse(self._run(mutant))

    def test_missing_outer_marker_fails_closed(self):
        mutant = self.predecessor.replace(MOD.START_MARKER, "UNRECOGNIZED_MARKER", 1)
        self.assertFalse(self._run(mutant))

    def test_bad_http_literal_fails_closed(self):
        needle = r'",47u);p+=47;'
        self.assertIn(needle, self.predecessor)
        mutant = self.predecessor.replace(needle, r'",46u);p+=46;', 1)
        self.assertFalse(self._run(mutant))


    def test_security_source_drift_fails_closed(self):
        # Every source-bound replacement must be unique; avoid silent partial hardening.
        source_markers = (
            "/* Gerador pseudo-aleatório determinístico para random[]",
            "static u32 TLS_BUILD_CLIENT_HELLO(TLSCtx*t,const char*host,u8*buf,u32 cap){",
            "        TLS_INIT(&_TLS);",
            "        u32 chlen=TLS_BUILD_CLIENT_HELLO(&_TLS,ctx->host,_NB,NET_BUF);",
            "#define TLS_HT_FINISHED       0x14u",
            '    u32 hlen=SL(host);',
            'typedef unsigned int       usize;',
        )
        for marker in source_markers:
            with self.subTest(marker=marker):
                self.assertEqual(self.predecessor.count(marker), 1)
                mutant = self.predecessor.replace(marker, "P0_INVALID_SOURCE_ANCHOR", 1)
                self.assertFalse(self._run(mutant))

if __name__ == "__main__":
    unittest.main()
