#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

: "${TERMUX_BOOTSTRAP_PACKAGE_NAME:=com.termux.rafacodephi}"
: "${TERMUX_BOOTSTRAP_PAGE_SIZE:=16384}"
: "${RAFCODEPHI_REAL_PKG_BOOTSTRAP:=false}"
: "${RAFCODEPHI_REAL_PKG_ARCH:=all}"
: "${RAFCODEPHI_REAL_PKG_VALIDATE:=true}"
: "${RAFCODEPHI_REAL_PKG_REPO:=https://packages.termux.dev/apt/termux-main}"

builder="${TMPDIR:-/tmp}/bootstrap_zip_builder.$$"
generated_root="${ROOT_DIR}/build/generated/rafaelia-bootstrap/common"
prefix="/data/data/${TERMUX_BOOTSTRAP_PACKAGE_NAME}/files/usr"
trap 'rm -f "$builder"' EXIT

mkdir -p "${generated_root}/bin" "${generated_root}/etc" "${generated_root}/lib" "${generated_root}/tmp" "${generated_root}/var" app/src/main/cpp

emit_wrapper_header() {
    printf '#!/system/bin/sh\nPREFIX="${PREFIX:-%s}"\n' "$prefix"
}

{
    emit_wrapper_header
    cat <<'EOS'
if [ -x "${PREFIX}/bin/bash" ]; then
    exec "${PREFIX}/bin/bash" "$@"
fi
if [ -x /system/bin/sh ]; then
    exec /system/bin/sh "$@"
fi
echo 'no executable shell backend found' >&2
exit 127
EOS
} > "${generated_root}/bin/sh"

{
    emit_wrapper_header
    cat <<'EOS'
cmd="${1:-help}"
is_raf_wrapper() { [ -f "$1" ] && grep -q 'RAFCODEPHI .*wrapper' "$1" 2>/dev/null; }

if [ "$cmd" = "--version" ] || [ "$cmd" = "version" ]; then
    echo 'RAFCODEPHI pkg bridge 1.0'
    if [ -x "${PREFIX}/bin/apt" ] && ! is_raf_wrapper "${PREFIX}/bin/apt"; then
        "${PREFIX}/bin/apt" --version 2>/dev/null || true
    fi
    exit 0
fi

if [ "$cmd" = "help" ] || [ "$cmd" = "-h" ] || [ "$cmd" = "--help" ]; then
    cat <<'HELP'
RAFCODEPHI pkg bridge
Usage: pkg <command> [args]
Commands are delegated to a real apt/apt-get backend after the core packages payload is installed.
Bootstrap-safe commands: help, --version.
HELP
    exit 0
fi

if [ -x "${PREFIX}/bin/apt" ] && ! is_raf_wrapper "${PREFIX}/bin/apt"; then
    exec "${PREFIX}/bin/apt" "$@"
fi
if [ -x "${PREFIX}/bin/apt-get" ] && ! is_raf_wrapper "${PREFIX}/bin/apt-get"; then
    exec "${PREFIX}/bin/apt-get" "$@"
fi
if [ -x "${PREFIX}/bin/apt.real" ]; then
    exec "${PREFIX}/bin/apt.real" "$@"
fi
if [ -x "${PREFIX}/bin/apt-get.real" ]; then
    exec "${PREFIX}/bin/apt-get.real" "$@"
fi

echo 'real apt/apt-get backend is not installed yet' >&2
echo 'build the RAFCODEPHI core packages and install the generated prefix payload.' >&2
exit 127
EOS
} > "${generated_root}/bin/pkg"

{
    emit_wrapper_header
    cat <<'EOS'
if [ -x "${PREFIX}/bin/apt.real" ]; then
    exec "${PREFIX}/bin/apt.real" "$@"
fi
case "${1:-help}" in
    help|-h|--help)
        echo 'RAFCODEPHI apt bridge: real apt backend not installed yet.'
        echo 'Use pkg help or install the RAFCODEPHI core packages payload.'
        exit 0
        ;;
    --version|version)
        echo 'RAFCODEPHI apt bridge 1.0'
        exit 0
        ;;
esac
echo 'real apt backend is not installed yet' >&2
exit 127
EOS
} > "${generated_root}/bin/apt"

{
    emit_wrapper_header
    cat <<'EOS'
if [ -x "${PREFIX}/bin/apt-get.real" ]; then
    exec "${PREFIX}/bin/apt-get.real" "$@"
fi
case "${1:-help}" in
    help|-h|--help)
        echo 'RAFCODEPHI apt-get bridge: real apt-get backend not installed yet.'
        echo 'Use pkg help or install the RAFCODEPHI core packages payload.'
        exit 0
        ;;
    --version|version)
        echo 'RAFCODEPHI apt-get bridge 1.0'
        exit 0
        ;;
esac
echo 'real apt-get backend is not installed yet' >&2
exit 127
EOS
} > "${generated_root}/bin/apt-get"

cat > "${generated_root}/bin/busybox" <<'EOS'
#!/system/bin/sh
if [ "${1:-}" = "--help" ] || [ "${1:-}" = "help" ]; then
    echo 'RAFCODEPHI busybox bridge; delegates applets to toybox/toolbox until real busybox is installed.'
    exit 0
fi
if [ "${1:-}" = "--version" ]; then
    echo 'RAFCODEPHI busybox bridge 1.0'
    exit 0
fi
if [ $# -eq 0 ]; then
    echo 'busybox bridge requires an applet name' >&2
    exit 1
fi
applet="$1"
shift
if [ -x /system/bin/toybox ]; then
    exec /system/bin/toybox "$applet" "$@"
fi
if [ -x /system/bin/toolbox ]; then
    exec /system/bin/toolbox "$applet" "$@"
fi
if [ -x /system/bin/"$applet" ]; then
    exec /system/bin/"$applet" "$@"
fi
if [ -x /system/xbin/"$applet" ]; then
    exec /system/xbin/"$applet" "$@"
fi
echo "no backend for busybox applet: ${applet}" >&2
exit 127
EOS

runtime_command_wrappers=(
    cat ls clear head tail grep sed awk cut tr wc sort uniq xargs tee
    mkdir rmdir rm cp mv ln chmod chown chgrp uname id pwd env dirname basename
    touch test printf echo sleep date dd du df ps kill which find readlink realpath
    expr yes false true seq tar gzip gunzip zcat stat strings file whoami hostname
    printenv
)

write_busybox_applet_wrapper() {
    local app="$1"
    cat > "${generated_root}/bin/${app}" <<EOS
#!/system/bin/sh
# RAFCODEPHI command wrapper: ${app}
PREFIX="\${PREFIX:-${prefix}}"
if [ -x "\${PREFIX}/bin/busybox" ]; then
    exec "\${PREFIX}/bin/busybox" "${app}" "\$@"
fi
if [ -x /system/bin/toybox ]; then
    exec /system/bin/toybox "${app}" "\$@"
fi
if [ -x /system/bin/toolbox ]; then
    exec /system/bin/toolbox "${app}" "\$@"
fi
echo "RAFCODEPHI: no backend for command wrapper: ${app}" >&2
exit 127
EOS
}

for app in "${runtime_command_wrappers[@]}"; do
    write_busybox_applet_wrapper "$app"
done

{
    emit_wrapper_header
    cat <<'EOS'
for candidate in "${PREFIX}/bin/proot.real" "${PREFIX}/libexec/proot"; do
    if [ -x "$candidate" ]; then
        exec "$candidate" "$@"
    fi
done
if [ "${1:-}" = "--version" ] || [ "${1:-}" = "--help" ] || [ "${1:-}" = "help" ]; then
    echo 'RAFCODEPHI proot bridge: real native proot is not installed yet.'
    exit 0
fi
echo 'real proot native binary is not installed yet' >&2
exit 127
EOS
} > "${generated_root}/bin/proot"

cat > "${generated_root}/bin/apkmanager" <<EOS
#!${prefix}/bin/sh
exec "${prefix}/bin/pkg" "\$@"
EOS

cat > "${generated_root}/bin/shellbash" <<EOS
#!${prefix}/bin/sh
if [ -x "${prefix}/bin/bash" ]; then
    exec "${prefix}/bin/bash" "\$@"
fi
exec "${prefix}/bin/sh" "\$@"
EOS

cat > "${generated_root}/bin/busybox-safe" <<EOS
#!${prefix}/bin/sh
exec "${prefix}/bin/busybox" "\$@"
EOS

cat > "${generated_root}/bin/proot-safe" <<EOS
#!${prefix}/bin/sh
exec "${prefix}/bin/proot" "\$@"
EOS

cat > "${generated_root}/etc/motd" <<'EOS'
RAFCODEPHI bootstrap payload
Bootstrap bridges are active. Install the RAFCODEPHI core package payload for real apt/bash/busybox/proot backends.
EOS
chmod 700 "${generated_root}/bin/sh" "${generated_root}/bin/pkg" "${generated_root}/bin/busybox" "${generated_root}/bin/proot" \
    "${generated_root}/bin/apt" "${generated_root}/bin/apt-get" "${generated_root}/bin/apkmanager" "${generated_root}/bin/shellbash" \
    "${generated_root}/bin/busybox-safe" "${generated_root}/bin/proot-safe"
for app in "${runtime_command_wrappers[@]}"; do
    chmod 700 "${generated_root}/bin/${app}"
done
chmod 600 "${generated_root}/etc/motd"

cc -O2 -std=c11 -Wall -Wextra -Werror scripts/bootstrap_zip_builder.c -o "$builder"
RAF_BOOTSTRAP_SRC_DIR="$generated_root" TERMUX_BOOTSTRAP_PACKAGE_NAME="$TERMUX_BOOTSTRAP_PACKAGE_NAME" TERMUX_BOOTSTRAP_PAGE_SIZE="$TERMUX_BOOTSTRAP_PAGE_SIZE" "$builder" app/src/main/cpp/bootstrap-aarch64.zip aarch64
RAF_BOOTSTRAP_SRC_DIR="$generated_root" TERMUX_BOOTSTRAP_PACKAGE_NAME="$TERMUX_BOOTSTRAP_PACKAGE_NAME" TERMUX_BOOTSTRAP_PAGE_SIZE="$TERMUX_BOOTSTRAP_PAGE_SIZE" "$builder" app/src/main/cpp/bootstrap-arm.zip arm
RAF_BOOTSTRAP_SRC_DIR="$generated_root" TERMUX_BOOTSTRAP_PACKAGE_NAME="$TERMUX_BOOTSTRAP_PACKAGE_NAME" TERMUX_BOOTSTRAP_PAGE_SIZE="$TERMUX_BOOTSTRAP_PAGE_SIZE" "$builder" app/src/main/cpp/bootstrap-i686.zip i686
RAF_BOOTSTRAP_SRC_DIR="$generated_root" TERMUX_BOOTSTRAP_PACKAGE_NAME="$TERMUX_BOOTSTRAP_PACKAGE_NAME" TERMUX_BOOTSTRAP_PAGE_SIZE="$TERMUX_BOOTSTRAP_PAGE_SIZE" "$builder" app/src/main/cpp/bootstrap-x86_64.zip x86_64

cp app/src/main/cpp/bootstrap-aarch64.zip app/src/main/cpp/rewritten-bootstrap-aarch64.zip
cp app/src/main/cpp/bootstrap-arm.zip app/src/main/cpp/rewritten-bootstrap-arm.zip
cp app/src/main/cpp/bootstrap-i686.zip app/src/main/cpp/rewritten-bootstrap-i686.zip
cp app/src/main/cpp/bootstrap-x86_64.zip app/src/main/cpp/rewritten-bootstrap-x86_64.zip

case "${RAFCODEPHI_REAL_PKG_BOOTSTRAP}" in
    true|1|yes|on)
        echo "RAFCODEPHI real pkg bootstrap requested: arch=${RAFCODEPHI_REAL_PKG_ARCH} repo=${RAFCODEPHI_REAL_PKG_REPO}"
        python3 scripts/build_real_arm_bootstrap_core.py \
            --arch "${RAFCODEPHI_REAL_PKG_ARCH}" \
            --repo "${RAFCODEPHI_REAL_PKG_REPO}"
        if [ "${RAFCODEPHI_REAL_PKG_VALIDATE}" = "true" ] || [ "${RAFCODEPHI_REAL_PKG_VALIDATE}" = "1" ] || [ "${RAFCODEPHI_REAL_PKG_VALIDATE}" = "yes" ]; then
            validate_targets=()
            case "${RAFCODEPHI_REAL_PKG_ARCH}" in
                all)
                    validate_targets+=(app/src/main/cpp/rewritten-bootstrap-aarch64.zip app/src/main/cpp/rewritten-bootstrap-arm.zip)
                    ;;
                aarch64)
                    validate_targets+=(app/src/main/cpp/rewritten-bootstrap-aarch64.zip)
                    ;;
                arm)
                    validate_targets+=(app/src/main/cpp/rewritten-bootstrap-arm.zip)
                    ;;
                *)
                    echo "Invalid RAFCODEPHI_REAL_PKG_ARCH='${RAFCODEPHI_REAL_PKG_ARCH}'. Use all, aarch64, or arm." >&2
                    exit 2
                    ;;
            esac
            python3 scripts/validate_real_arm_bootstrap_core.py "${validate_targets[@]}"
        fi
        ;;
    false|0|no|off)
        echo "RAFCODEPHI real pkg bootstrap disabled; generated bridge-only ARM bootstraps. This is not a real pkg build."
        ;;
    *)
        echo "Invalid RAFCODEPHI_REAL_PKG_BOOTSTRAP='${RAFCODEPHI_REAL_PKG_BOOTSTRAP}'. Use true/false." >&2
        exit 2
        ;;
esac

echo "RAFCODEPHI bootstraps generated for package=${TERMUX_BOOTSTRAP_PACKAGE_NAME} page_size=${TERMUX_BOOTSTRAP_PAGE_SIZE} payload=${generated_root} real_pkg=${RAFCODEPHI_REAL_PKG_BOOTSTRAP}"
