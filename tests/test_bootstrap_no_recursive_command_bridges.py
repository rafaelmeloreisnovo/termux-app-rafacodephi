from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "scripts/build_rafaelia_bootstraps.sh"


def test_pkg_apt_bridge_does_not_recurse():
    source = GENERATOR.read_text(encoding="utf-8")
    assert "is_raf_wrapper" in source
    assert "RAFCODEPHI apt bridge" in source
    assert "RAFCODEPHI apt-get bridge" in source
    assert "RAFCODEPHI developer bootstrap pkg" not in source
    assert "RAFCODEPHI bootstrap busybox stub" not in source
    assert "RAFCODEPHI bootstrap proot stub" not in source

    # pkg is emitted from a grouped shell block redirected to bin/pkg. Extract
    # that current structure instead of depending on the obsolete direct-cat form.
    pkg_end_marker = '} > "${generated_root}/bin/pkg"'
    pkg_end = source.index(pkg_end_marker)
    pkg_start = source.rfind("\n{\n", 0, pkg_end)
    assert pkg_start >= 0
    pkg_block = source[pkg_start:pkg_end + len(pkg_end_marker)]

    # pkg must delegate to distinct apt/apt-get backends and never exec itself.
    assert 'exec "${PREFIX}/bin/pkg"' not in pkg_block
    assert 'exec "${PREFIX}/bin/apt" "$@"' in pkg_block
    assert 'exec "${PREFIX}/bin/apt-get" "$@"' in pkg_block

    # apkmanager legitimately bridges to the distinct pkg binary; that is not
    # self-recursion and must keep working.
    apkmanager_start = source.index('cat > "${generated_root}/bin/apkmanager" <<')
    apkmanager_end = source.index("\nEOS\n", apkmanager_start)
    apkmanager_block = source[apkmanager_start:apkmanager_end]
    assert 'exec "${prefix}/bin/pkg" "\\$@"' in apkmanager_block
