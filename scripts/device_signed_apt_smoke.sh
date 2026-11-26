#!/system/bin/sh
# Read-only by default; opt-in local apt update/install from inside RAFCODEPHI.
# This probes actual Android execution; CI does NOT invoke it on a device.
set -eu
P=/data/data/com.termux.rafacodephi/files/usr
SOURCE="$P/etc/apt/sources.list.d/termux.sources"
KEY="$P/etc/apt/keyrings/rafcodephi-archive-key.gpg"
EXPECTED="${RAF_APT_TRUSTED_KEY_SHA256:-}"
MODE="${1:---check}"
case "$MODE" in --check|--update|--install-nano|--ninja-smoke|--install-ninja) ;; *)
  echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=invalid_mode' >&2
  exit 2
esac
[ "${PREFIX:-}" = "$P" ] || {
  echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=wrong_app_prefix' >&2
  exit 3
}
[ -x "$P/bin/apt-get" ] && [ -x "$P/bin/dpkg" ] &&
  [ -r "$SOURCE" ] && [ -s "$KEY" ] && [ -r "$P/etc/apt/apt.conf.d/00rafcodephi-repository-block" ] || {
  echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=missing_real_payload' >&2
  exit 4
}
case "$EXPECTED" in
  *[!0-9a-f]*|'') echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=unbound_key_hash' >&2; exit 5 ;;
esac
[ "${#EXPECTED}" -eq 64 ] || {
  echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=invalid_hash_size' >&2
  exit 5
}
printf '%s  %s\n' "$EXPECTED" "$KEY" | "$P/bin/sha256sum" -c - >/dev/null || {
  echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=archive_key_mismatch' >&2
  exit 6
}
grep -Fxq 'Enabled: yes' "$SOURCE" &&
  grep -Fxq 'Types: deb' "$SOURCE" &&
  grep -Fxq 'Architectures: arm aarch64' "$SOURCE" &&
  grep -Fxq "Signed-By: $KEY" "$SOURCE" || {
  echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=unsigned_or_disabled_source' >&2
  exit 7
}
! grep -Fq 'Trusted: yes' "$SOURCE" || {
  echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=trust_bypass' >&2
  exit 7
}
[ -z "$(grep -l '^[[:space:]]*APT::Update::Pre-Invoke' "$P"/etc/apt/apt.conf.d/00rafcodephi-repository-block 2>/dev/null)" ] || {
  echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=preinvoke_still_active' >&2
  exit 8
}
"$P/bin/apt-get" --version >/dev/null &&
  "$P/bin/dpkg" --version >/dev/null || {
  echo 'RAFCODEPHI_DEVICE_APT=BLOCKED reason=package_manager_exec_failed' >&2
  exit 9
}
case "$MODE" in
  --check)
    echo 'RAFCODEPHI_DEVICE_APT=PASS_SCOPED local_binaries_and_key=true remote_repo=NOT_TESTED install=NOT_RUN'
    ;;
  --update)
    "$P/bin/apt-get" -o APT::Update::Error-Mode=any update
    echo 'RAFCODEPHI_DEVICE_APT=PASS_SCOPED apt_update=true install=NOT_RUN'
    ;;
  --install-nano)
    "$P/bin/apt-get" -o APT::Update::Error-Mode=any update
    "$P/bin/apt-get" install -y --no-install-recommends nano
    echo 'RAFCODEPHI_DEVICE_APT=PASS_SCOPED apt_update=true nano_install=true'
    ;;
  --install-ninja)
    "$P/bin/apt-get" -o APT::Update::Error-Mode=any update
    "$P/bin/apt-get" install -y --no-install-recommends ninja
    ;;
esac

case "$MODE" in --ninja-smoke|--install-ninja)
  [ -x "$P/bin/ninja" ] || {
    echo 'RAFCODEPHI_DEVICE_NINJA=BLOCKED reason=missing_ninja' >&2
    exit 10
  }
  ninja_status="$("$P/bin/dpkg" -s ninja)" || {
    echo 'RAFCODEPHI_DEVICE_NINJA=BLOCKED reason=ninja_status_failed' >&2
    exit 11
  }
  printf '%s\n' "$ninja_status" | grep -Fxq 'Status: install ok installed' || {
    echo 'RAFCODEPHI_DEVICE_NINJA=BLOCKED reason=ninja_not_installed' >&2
    exit 11
  }
  "$P/bin/ninja" --version
  probe="$(mktemp -d "$P/tmp/rafcodephi-ninja.XXXXXX")"
  trap 'rm -rf "$probe"' EXIT
  trap 'exit 130' INT
  trap 'exit 143' TERM
  printf '%s\n' 'rule emit' \
    '  command = printf rafcodephi_ninja_contract_v1 > $out' \
    'build proof.txt: emit' > "$probe/build.ninja"
  "$P/bin/ninja" -C "$probe" -f build.ninja || {
    echo 'RAFCODEPHI_DEVICE_NINJA=BLOCKED reason=build_failed' >&2
    exit 12
  }
  [ "$(cat "$probe/proof.txt")" = rafcodephi_ninja_contract_v1 ] || {
    echo 'RAFCODEPHI_DEVICE_NINJA=BLOCKED reason=output_mismatch' >&2
    exit 13
  }
  echo 'schema=rafcodephi.apt-bootstrap-ninja-device/v1'
  "$P/bin/sha256sum" "$P/bin/apt-get" "$P/bin/dpkg" "$P/bin/ninja" "$probe/proof.txt"
  printf 'archive_key_sha256=%s\n' "$EXPECTED"
  echo 'RAFCODEPHI_DEVICE_NINJA=PASS_SCOPED graph_executed=true claim_allowed_release=false'
  ;;
esac
