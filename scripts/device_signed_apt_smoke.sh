#!/system/bin/sh
# Read-only by default; opt-in local apt update/install from inside RAFCODEPHI.
# This probes actual Android execution; CI does NOT invoke it on a device.
set -eu
P=/data/data/com.termux.rafacodephi/files/usr
SOURCE="$P/etc/apt/sources.list.d/termux.sources"
KEY="$P/etc/apt/keyrings/rafcodephi-archive-key.gpg"
EXPECTED="${RAF_APT_TRUSTED_KEY_SHA256:-}"
MODE="${1:---check}"
case "$MODE" in --check|--update|--install-nano) ;; *)
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
    "$P/bin/apt-get" update
    echo 'RAFCODEPHI_DEVICE_APT=PASS_SCOPED apt_update=true install=NOT_RUN'
    ;;
  --install-nano)
    "$P/bin/apt-get" update
    "$P/bin/apt-get" install -y --no-install-recommends nano
    echo 'RAFCODEPHI_DEVICE_APT=PASS_SCOPED apt_update=true nano_install=true'
    ;;
esac
