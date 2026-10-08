from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/sign-release.yml"


class ReleasePublicationContract(unittest.TestCase):
    def test_signing_cannot_automatically_publish(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        top, signing, publishing = workflow.split("  sign-release:", 1)[0], workflow.split("  sign-release:", 1)[1].split("  prepare-release-draft:", 1)[0], workflow.split("  prepare-release-draft:", 1)[1]
        self.assertIn("contents: read", top)
        self.assertNotIn("contents: write", signing)
        self.assertIn("if: github.actor == 'rafaelmeloreisnovo'", signing)
        self.assertIn("needs: sign-release", publishing)
        self.assertIn("contents: write", publishing)
        self.assertIn("github.event_name == 'workflow_dispatch'", publishing)
        self.assertIn("inputs.create_release == true", publishing)
        self.assertIn("github.actor == 'rafaelmeloreisnovo'", publishing)
        self.assertIn("startsWith(github.ref, 'refs/tags/v')", publishing)
        self.assertIn("name: termux-rafacodephi-signed-", publishing)
        self.assertIn("draft: true", publishing)
        self.assertNotIn("draft: false", publishing)
        self.assertIn("release_publication=BLOCKED_DRAFT_ONLY", publishing)


if __name__ == "__main__":
    unittest.main()
