"""Static regression contracts; not evidence of an Android execution."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "app/src/main/java/com/termux/app/BootstrapWizardSource.java"
ACTIVITY = ROOT / "app/src/main/java/com/termux/app/activities/BetaBootstrapWizardActivity.java"


def test_import_rejects_non_pinned_blake3_before_acceptance() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    check = source.index('String expected = BootstrapIntegrityVerifier.expectedHashForCurrentAbi()')
    digest = source.index('String actual = BootstrapIntegrityVerifier.blake3Hex(tmp, MAX_BOOTSTRAP_BYTES)')
    mismatch = source.index('throw new SecurityException("BOOTSTRAP_BLAKE3_MISMATCH')
    accept = source.index('if (!tmp.renameTo(target))')
    assert check < digest < mismatch < accept
    assert 'tmp.delete();' in source[mismatch - 60:mismatch]
    assert 'if (!expected.equals(actual))' in source[digest:mismatch]


def test_wizard_explains_build_identity_without_bypass() -> None:
    activity = ACTIVITY.read_text(encoding="utf-8")
    assert 'error instanceof SecurityException' in activity
    assert 'failure.startsWith("BOOTSTRAP_BLAKE3_MISMATCH ")' in activity
    assert 'identityMismatch ? "bootstrap.zip identity mismatch" : "bootstrap.zip rejected"' in activity
    assert "Use the bootstrap from the exact same verified APK/CI build" in activity
    assert "Do not rename the file, bypass the hash, or clear app data." in activity
    assert "BootstrapWizardSource.accept(this, uri)" in activity
    assert "BetaRealBootstrapRepair.repair(this, this::updateWizardStep)" in activity
