from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GRADLE = ROOT / "termux-shared" / "build.gradle"


def test_release_publication_materializes_the_android_aar_component() -> None:
    text = GRADLE.read_text(encoding="utf-8")

    assert 'singleVariant("release")' in text
    assert "from components.release" in text
    assert "components.findByName('release')" not in text
    assert "artifactId = 'termux-shared'" in text
    assert "version = '0.118.0'" in text
