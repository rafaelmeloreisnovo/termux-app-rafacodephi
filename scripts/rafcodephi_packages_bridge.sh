#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGES_REPO_URL="${RAFCODEPHI_PACKAGES_REPO_URL:-https://github.com/rafaelmeloreisnovo/termux-packages.git}"
PACKAGES_SELECTOR="${RAFCODEPHI_PACKAGES_REF:-${RAFCODEPHI_PACKAGES_CHANNEL:-canonical}}"
PACKAGES_REF="$(python3 "${ROOT_DIR}/scripts/resolve_termux_packages_pin.py" "$PACKAGES_SELECTOR")"
PACKAGES_DIR="${RAFCODEPHI_PACKAGES_DIR:-${ROOT_DIR}/out/termux-packages}"
OUT_DIR="${ROOT_DIR}/out/rafcodephi-packages-bridge"
REQUIRED_PACKAGES=(apt bash busybox proot dpkg ca-certificates coreutils termux-tools termux-api)
REQUIRED_ARCHES=(aarch64 arm)

info() { printf '[rafcodephi-packages-bridge] %s\n' "$*"; }
fail() { printf '[rafcodephi-packages-bridge][ERROR] %s\n' "$*" >&2; exit 1; }

validate_source_identity() {
  case "$PACKAGES_REPO_URL" in
    https://github.com/rafaelmeloreisnovo/termux-packages|https://github.com/rafaelmeloreisnovo/termux-packages.git)
      ;;
    *)
      fail "RAFCODEPHI_PACKAGES_REPO_URL must point to rafaelmeloreisnovo/termux-packages, got: ${PACKAGES_REPO_URL}"
      ;;
  esac
  [[ "$PACKAGES_REF" =~ ^[0-9a-f]{40}$ ]] \
    || fail "resolved packages ref must be a pinned 40-char commit, selector=${PACKAGES_SELECTOR} resolved=${PACKAGES_REF}"
}

checkout_pinned_ref() {
  git -C "$PACKAGES_DIR" fetch --depth 1 origin "$PACKAGES_REF" >/dev/null
  git -C "$PACKAGES_DIR" checkout --detach -q FETCH_HEAD
  local actual
  actual="$(git -C "$PACKAGES_DIR" rev-parse HEAD)"
  [[ "$actual" == "$PACKAGES_REF" ]] \
    || fail "resolved packages commit ${actual} does not match required ${PACKAGES_REF}"
}

ensure_repo() {
  validate_source_identity
  mkdir -p "$(dirname "$PACKAGES_DIR")" "$OUT_DIR"
  if [[ -d "${PACKAGES_DIR}/.git" ]]; then
    info "using existing packages repo: ${PACKAGES_DIR}"
    local existing_url
    existing_url="$(git -C "$PACKAGES_DIR" remote get-url origin 2>/dev/null || true)"
    case "$existing_url" in
      https://github.com/rafaelmeloreisnovo/termux-packages|https://github.com/rafaelmeloreisnovo/termux-packages.git)
        ;;
      *)
        fail "existing packages repo has unexpected origin: ${existing_url:-missing}"
        ;;
    esac
  else
    [[ ! -e "$PACKAGES_DIR" ]] || fail "packages directory exists but is not a git repository: ${PACKAGES_DIR}"
    info "cloning packages repo: ${PACKAGES_REPO_URL}"
    git clone --depth 1 "$PACKAGES_REPO_URL" "$PACKAGES_DIR" >/dev/null
  fi
  checkout_pinned_ref
}

validate_recipe() {
  local pkg="$1"
  local recipe="${PACKAGES_DIR}/packages/${pkg}/build.sh"
  [[ -f "$recipe" ]] || fail "missing package recipe: packages/${pkg}/build.sh"
  grep -q 'TERMUX_PKG_VERSION' "$recipe" || fail "recipe missing TERMUX_PKG_VERSION: ${pkg}"
  info "recipe ok: ${pkg}"
}

validate_contract() {
  ensure_repo
  for pkg in "${REQUIRED_PACKAGES[@]}"; do
    validate_recipe "$pkg"
  done
  [[ -f "${PACKAGES_DIR}/.github/workflows/packages.yml" ]] || fail "missing packages workflow"
  for arch in "${REQUIRED_ARCHES[@]}"; do
    grep -q "$arch" "${PACKAGES_DIR}/.github/workflows/packages.yml" || fail "workflow missing arch: ${arch}"
  done
  printf '%s\n' "${REQUIRED_PACKAGES[@]}" > "${OUT_DIR}/required-packages.txt"
  printf '%s\n' "${REQUIRED_ARCHES[@]}" > "${OUT_DIR}/required-arches.txt"
  git -C "$PACKAGES_DIR" rev-parse HEAD > "${OUT_DIR}/packages-repo-head.txt"
  printf '%s\n' "$PACKAGES_SELECTOR" > "${OUT_DIR}/packages-repo-selector.txt"
  printf '%s\n' "$PACKAGES_REF" > "${OUT_DIR}/packages-repo-required-ref.txt"
  info "selector=${PACKAGES_SELECTOR}"
  info "contract=PASS"
}

emit_dispatch_plan() {
  validate_contract
  cat > "${OUT_DIR}/workflow-dispatch-packages.txt" <<EOF
packages: ${REQUIRED_PACKAGES[*]}
free-space: false
workflow: Packages
repository: ${PACKAGES_REPO_URL}
selector: ${PACKAGES_SELECTOR}
ref: ${PACKAGES_REF}
EOF
  info "dispatch plan: ${OUT_DIR}/workflow-dispatch-packages.txt"
}

case "${1:-validate}" in
  validate) validate_contract ;;
  plan|dispatch-plan) emit_dispatch_plan ;;
  *) fail "usage: $0 [validate|plan]" ;;
esac
