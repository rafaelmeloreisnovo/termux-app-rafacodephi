#!/usr/bin/env bash
set -u

APK="${1:-}"
REPORT="${2:-}"
HOST_APK="${3:-}"

if [[ -z "${APK}" || -z "${REPORT}" ]]; then
  echo "usage: $0 <loader.apk> <report-file> [host.apk]" >&2
  exit 64
fi

mkdir -p "$(dirname "${REPORT}")"
: > "${REPORT}"
failures=0
record(){ printf '%s\n' "$1" | tee -a "${REPORT}"; }
fail(){ record "FAIL $1"; failures=$((failures + 1)); }
pass(){ record "PASS $1"; }

if [[ ! -f "${APK}" ]]; then
  fail "loader APK not found: ${APK}"
  record "SUMMARY FAIL failures=${failures}"
  exit 1
fi

APKANALYZER="$(bash scripts/find_android_sdk_tool.sh apkanalyzer 2>>"${REPORT}" || true)"
APKSIGNER="$(bash scripts/find_android_sdk_tool.sh apksigner 2>>"${REPORT}" || true)"
AAPT="$(bash scripts/find_android_sdk_tool.sh aapt 2>>"${REPORT}" || true)"

record "state=BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE"
record "claim_allowed=false"
record "apk=${APK}"
record "size_bytes=$(stat -c '%s' "${APK}")"
record "sha256=$(sha256sum "${APK}" | awk '{print $1}')"
record "host_apk=${HOST_APK:-TOKEN_VAZIO}"
record "apkanalyzer=${APKANALYZER:-NOT_FOUND}"
record "apksigner=${APKSIGNER:-NOT_FOUND}"
record "aapt=${AAPT:-NOT_FOUND}"

package_name=""; min_sdk=""; target_sdk=""; manifest_print=""
if [[ -n "${APKANALYZER}" ]]; then
  package_name="$(${APKANALYZER} manifest application-id "${APK}" 2>>"${REPORT}" || true)"
  min_sdk="$(${APKANALYZER} manifest min-sdk "${APK}" 2>>"${REPORT}" || true)"
  target_sdk="$(${APKANALYZER} manifest target-sdk "${APK}" 2>>"${REPORT}" || true)"
  manifest_print="$(${APKANALYZER} manifest print "${APK}" 2>>"${REPORT}" || true)"
  package_name="$(tr -d '\r\n' <<<"${package_name}")"
  min_sdk="$(tr -d '\r\n' <<<"${min_sdk}")"
  target_sdk="$(tr -d '\r\n' <<<"${target_sdk}")"
fi

badging=""; manifest_tree=""
if [[ -n "${AAPT}" ]]; then
  badging="$(${AAPT} dump badging "${APK}" 2>>"${REPORT}" || true)"
  manifest_tree="$(${AAPT} dump xmltree "${APK}" AndroidManifest.xml 2>>"${REPORT}" || true)"
  printf '%s\n' "${badging}" > "$(dirname "${REPORT}")/loader-badging.txt"
  printf '%s\n' "${manifest_tree}" > "$(dirname "${REPORT}")/loader-manifest-tree.txt"
fi

if [[ -z "${package_name}" ]]; then
  package_name="$(sed -n "s/^package: name='\([^']*\)'.*/\1/p" <<<"${badging}" | head -n1)"
  min_sdk="$(sed -n "s/^sdkVersion:'\([^']*\)'.*/\1/p" <<<"${badging}" | head -n1)"
  target_sdk="$(sed -n "s/^targetSdkVersion:'\([^']*\)'.*/\1/p" <<<"${badging}" | head -n1)"
fi
record "package=${package_name:-UNRESOLVED}"
record "min_sdk=${min_sdk:-UNRESOLVED}"
record "target_sdk=${target_sdk:-UNRESOLVED}"
[[ "${package_name}" == "com.termux.rafacodephi.loader" ]] && pass package || fail "package expected=com.termux.rafacodephi.loader actual=${package_name:-UNRESOLVED}"
[[ "${min_sdk}" == "21" ]] && pass min_sdk || fail "min_sdk expected=21 actual=${min_sdk:-UNRESOLVED}"
[[ "${target_sdk}" == "28" ]] && pass target_sdk || fail "target_sdk expected=28 actual=${target_sdk:-UNRESOLVED}"

manifest_all="${manifest_print}"$'\n'"${manifest_tree}"
for required in \
  'BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE' \
  'com.termux.rafacodephi.permission.BOOTSTRAP_HANDOFF' \
  'com.termux.rafacodephi.loader.LoaderActivity' \
  'com.termux.rafacodephi.loader.BootstrapInstallService' \
  'com.termux.rafacodephi.loader.VerifiedBootstrapProvider' \
  'com.termux.rafacodephi.loader.bootstrap'; do
  if grep -Fq -- "${required}" <<<"${manifest_all}"; then pass "manifest:${required}"; else fail "manifest missing ${required}"; fi
done
if grep -Eq 'usesCleartextTraffic[^\n]*(false|0x0)' <<<"${manifest_all}"; then
  pass uses_cleartext_false
else
  fail "manifest must disable cleartext traffic"
fi

mapfile -t dex_files < <(unzip -Z1 "${APK}" | grep -E '^classes([0-9]*)?\.dex$' || true)
dex_count="${#dex_files[@]}"
record "dex_count=${dex_count}"
# API 21+ supports native multidex. Restrict the loader to its observed one-
# or two-DEX layout and validate every archive entry, not just classes.dex.
# Extra DEX files, gaps, duplicate names or missing entrypoint classes fail.
if (( dex_count < 1 || dex_count > 2 )); then
  fail "functional loader requires one or two DEX files, actual=${dex_count}"
else
  descriptors=""
  for (( dex_index=0; dex_index<dex_count; dex_index++ )); do
    expected_name="classes.dex"
    if (( dex_index > 0 )); then expected_name="classes$((dex_index + 1)).dex"; fi
    dex_name="${dex_files[dex_index]}"
    if [[ "${dex_name}" != "${expected_name}" ]]; then
      fail "noncanonical DEX sequence expected=${expected_name} actual=${dex_name}"
      continue
    fi
    if ! unzip -tqq "${APK}" "${dex_name}" >/dev/null 2>&1; then
      fail "unreadable DEX archive entry: ${dex_name}"
      continue
    fi
    dex_sha="$(unzip -p "${APK}" "${dex_name}" | sha256sum | awk '{print $1}')"
    record "dex_sha256[${dex_name}]=${dex_sha}"
    this_descriptors="$(unzip -p "${APK}" "${dex_name}" | strings -a | grep -oE 'L[A-Za-z0-9_/$.-]+;' || true)"
    descriptors+="${this_descriptors}"$'\n'
  done
  descriptors="$(printf '%s' "${descriptors}" | sed '/^$/d' | sort -u)"
  printf '\n[dex_class_descriptors]\n%s\n' "${descriptors}" | tee -a "${REPORT}"
  for descriptor in \
    'Lcom/termux/rafacodephi/loader/LoaderActivity;' \
    'Lcom/termux/rafacodephi/loader/BootstrapInstallService;' \
    'Lcom/termux/rafacodephi/loader/BootstrapSourcePolicy;' \
    'Lcom/termux/rafacodephi/loader/VerifiedBootstrapProvider;'; do
    if grep -Fxq -- "${descriptor}" <<<"${descriptors}"; then pass "dex:${descriptor}"; else fail "DEX missing ${descriptor}"; fi
  done
fi
payloads="$(unzip -Z1 "${APK}" | grep -Ei '(^|/)(bootstrap|payload).*(\.zip|\.tar|\.gz)$' || true)"
if [[ -z "${payloads}" ]]; then
  pass no_embedded_bootstrap_payload
else
  fail "loader must acquire, not embed payloads: ${payloads//$'\n'/,}"
fi

certificate_digest(){
  local apk="$1"
  "${APKSIGNER}" verify --print-certs "${apk}" 2>>"${REPORT}" \
    | sed -n 's/^Signer #1 certificate SHA-256 digest: //p' | head -n1 | tr 'A-F' 'a-f'
}
if [[ -z "${APKSIGNER}" ]]; then
  fail "apksigner unavailable"
else
  signature_output="$(${APKSIGNER} verify --verbose "${APK}" 2>&1)"; signature_status=$?
  printf '\n[apksigner-loader]\n%s\n' "${signature_output}" | tee -a "${REPORT}"
  if [[ ${signature_status} -eq 0 ]]; then pass loader_signature; else fail "loader apksigner exit=${signature_status}"; fi
  loader_cert="$(certificate_digest "${APK}")"
  record "loader_cert_sha256=${loader_cert:-UNRESOLVED}"
  if [[ -n "${HOST_APK}" ]]; then
    if [[ ! -f "${HOST_APK}" ]]; then
      fail "host APK not found: ${HOST_APK}"
    else
      host_signature_output="$(${APKSIGNER} verify --verbose "${HOST_APK}" 2>&1)"; host_status=$?
      printf '\n[apksigner-host]\n%s\n' "${host_signature_output}" | tee -a "${REPORT}"
      [[ ${host_status} -eq 0 ]] && pass host_signature || fail "host apksigner exit=${host_status}"
      host_cert="$(certificate_digest "${HOST_APK}")"
      record "host_cert_sha256=${host_cert:-UNRESOLVED}"
      if [[ -n "${loader_cert}" && "${loader_cert}" == "${host_cert}" ]]; then
        pass matching_host_loader_certificate
      else
        fail "host and loader certificates differ"
      fi
    fi
  else
    record "matching_host_loader_certificate=TOKEN_VAZIO"
    fail "host APK required for signature-bound handoff verification"
  fi
fi

if (( failures > 0 )); then
  record "SUMMARY FAIL failures=${failures}"
  exit 1
fi
record "SUMMARY PASS failures=0"
