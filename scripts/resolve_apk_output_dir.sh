#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 <apk-version-tag> [required-abi ...]" >&2
}

[[ $# -ge 1 ]] || { usage; exit 2; }

apk_version_tag="$1"
shift

workspace="${GITHUB_WORKSPACE:-$PWD}"
search_root="${APK_SEARCH_ROOT:-}"

# Auto-detect gradle APK output directory (gradle 7.x+, 8.x compatibility)
if [[ -z "$search_root" ]]; then
  # Prefer explicit outputs/apk (gradle < 8.x or configured output)
  if [[ -d "$workspace/app/build/outputs/apk" ]]; then
    search_root="$workspace/app/build/outputs/apk"
  # Fallback to intermediates/apk (gradle 8.x+)
  elif [[ -d "$workspace/app/build/intermediates/apk" ]]; then
    search_root="$workspace/app/build/intermediates/apk"
  fi
fi

if [[ -z "$search_root" || ! -d "$search_root" ]]; then
  echo "❌ APK search root does not exist (tried outputs/apk and intermediates/apk)" >&2
  if [[ -d "$workspace/app/build" ]]; then
    echo "Observed app/build directories:" >&2
    find "$workspace/app/build" -maxdepth 5 -type d -print | sort >&2 || true
    echo "Searching for any .apk files:" >&2
    find "$workspace/app/build" -name "*.apk" -type f -print | head -20 >&2 || true
  fi
  exit 3
fi

mapfile -t apks < <(
  find "$search_root" -type f -name "termux-app_${apk_version_tag}_*.apk" -print | sort
)

if (( ${#apks[@]} == 0 )); then
  echo "❌ No APK matched tag '$apk_version_tag' under $search_root" >&2
  echo "Observed APK outputs:" >&2
  find "$search_root" -type f -name '*.apk' -print | sort >&2 || true
  exit 4
fi

mapfile -t output_dirs < <(
  printf '%s\n' "${apks[@]}" | xargs -r -n1 dirname | sort -u
)

if (( ${#output_dirs[@]} != 1 )); then
  echo "❌ Ambiguous APK output directories for tag '$apk_version_tag': ${#output_dirs[@]}" >&2
  printf '  %s\n' "${output_dirs[@]}" >&2
  exit 5
fi

apk_dir_path="${output_dirs[0]}"
metadata="$apk_dir_path/output-metadata.json"

if [[ ! -s "$metadata" ]]; then
  echo "❌ Missing or empty Gradle output metadata: $metadata" >&2
  exit 6
fi

for apk in "${apks[@]}"; do
  if [[ ! -s "$apk" ]]; then
    echo "❌ APK exists but is empty: $apk" >&2
    exit 7
  fi
done

for required_abi in "$@"; do
  required_apk="$apk_dir_path/termux-app_${apk_version_tag}_${required_abi}.apk"
  if [[ ! -s "$required_apk" ]]; then
    echo "❌ Required ABI artifact missing: $required_apk" >&2
    echo "Matched APKs:" >&2
    printf '  %s\n' "${apks[@]}" >&2
    exit 8
  fi
done

echo "Resolved APK output directory: $apk_dir_path" >&2
printf '  %s\n' "${apks[@]}" >&2
printf '%s\n' "$apk_dir_path"
