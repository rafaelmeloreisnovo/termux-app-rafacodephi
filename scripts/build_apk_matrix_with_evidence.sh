#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/dist/apk-matrix"
EVIDENCE_DIR="${OUT_DIR}/evidence"
TEMPLATE="${ROOT_DIR}/data/contracts/apk_rafcodephi_release.v1.json"
ALLOW_DIRTY_BUILD="${ALLOW_DIRTY_BUILD:-0}"

info() { printf '\n[build_apk_matrix_with_evidence] %s\n' "$*"; }
fail() { printf '\n[build_apk_matrix_with_evidence] ERROR: %s\n' "$*" >&2; exit 1; }

cd "${ROOT_DIR}"
[[ -f "${TEMPLATE}" ]] || fail "missing evidence contract template: ${TEMPLATE}"
[[ -f scripts/verify_apk_evidence.py ]] || fail "missing scripts/verify_apk_evidence.py"

SOURCE_COMMIT="$(git rev-parse HEAD)"
PRE_BUILD_DIRTY="$(git status --porcelain --untracked-files=no)"
if [[ -n "${PRE_BUILD_DIRTY}" && "${ALLOW_DIRTY_BUILD}" != "1" ]]; then
  fail "tracked source tree is dirty before build; set ALLOW_DIRTY_BUILD=1 only for explicit non-canonical experiments"
fi

info "Pinned source commit: ${SOURCE_COMMIT}"
./scripts/build_apk_matrix.sh

POST_BUILD_DIRTY="$(git status --porcelain --untracked-files=no)"
if [[ -n "${POST_BUILD_DIRTY}" && "${ALLOW_DIRTY_BUILD}" != "1" ]]; then
  fail "tracked source tree changed during build; provenance cannot be attributed to a single commit"
fi

BUILD_TOOLS_VERSION="$(awk -F= '/^buildToolsVersion=/{gsub(/[[:space:]]/, "", $2); print $2; exit}' gradle.properties || true)"
if [[ -z "${BUILD_TOOLS_VERSION}" ]]; then
  BUILD_TOOLS_VERSION="$(awk -F= '/^compileSdkVersion=/{gsub(/[[:space:]]/, "", $2); print $2; exit}' gradle.properties).0.0"
fi
SDK_DIR="$(grep -E '^sdk.dir=' local.properties | cut -d= -f2-)"
SDK_DIR="${SDK_DIR//\\/}"
APKSIGNER="${SDK_DIR}/build-tools/${BUILD_TOOLS_VERSION}/apksigner"
[[ -x "${APKSIGNER}" ]] || fail "apksigner not found at ${APKSIGNER}"

mkdir -p "${EVIDENCE_DIR}/contracts" "${EVIDENCE_DIR}/receipts"

BUILD_RECEIPT="${EVIDENCE_DIR}/BUILD_PROVENANCE.json"
python3 - "${OUT_DIR}" "${SOURCE_COMMIT}" "${BUILD_RECEIPT}" <<'PY'
import hashlib
import json
import os
import sys
from pathlib import Path

out_dir = Path(sys.argv[1])
source_commit = sys.argv[2]
out_path = Path(sys.argv[3])
artifacts = []
for apk in sorted((out_dir / "signed").glob("*.apk")):
    h = hashlib.sha256()
    with apk.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    artifacts.append({
        "path": str(apk.relative_to(out_dir)),
        "bytes": apk.stat().st_size,
        "sha256": h.hexdigest(),
    })
if not artifacts:
    raise SystemExit("no signed APK artifacts found")
receipt = {
    "schema": "rafaelia.apk-build-provenance.v1",
    "source_commit": source_commit,
    "release_track": os.environ.get("RELEASE_TRACK", "internal"),
    "source_tree_clean_before_build": os.environ.get("ALLOW_DIRTY_BUILD", "0") != "1",
    "artifacts": artifacts,
    "claim_allowed": False,
    "invariant": "SOURCE_COMMIT + BUILD_PROCESS + ARTIFACT_HASH != PHYSICAL_RUNTIME_EVIDENCE",
}
out_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
BUILD_RECEIPT_SHA256="$(sha256sum "${BUILD_RECEIPT}" | awk '{print $1}')"
printf '%s  %s\n' "${BUILD_RECEIPT_SHA256}" "$(basename "${BUILD_RECEIPT}")" > "${EVIDENCE_DIR}/BUILD_PROVENANCE.sha256"

RUN_CONTRACT="${EVIDENCE_DIR}/contracts/RAFCODEPHI_RELEASE_RUN.json"
python3 - "${TEMPLATE}" "${RUN_CONTRACT}" "${SOURCE_COMMIT}" <<'PY'
import json
import sys
from pathlib import Path

template = Path(sys.argv[1])
out = Path(sys.argv[2])
source_commit = sys.argv[3]
data = json.loads(template.read_text(encoding="utf-8"))
data["status"] = "RUN_CONTRACT"
data["build_provenance"]["source_commit"] = source_commit
# The receipt is materialized and hashed, but V1 deliberately leaves the
# contract receipt pin empty so provenance is not promoted by declaration.
data["build_provenance"]["build_receipt_sha256"] = None
out.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

info "Verifying signed APK identities, CRC and signature schemes"
while IFS= read -r -d '' apk; do
  base="$(basename "${apk%.apk}")"
  receipt="${EVIDENCE_DIR}/receipts/${base}.evidence.json"
  python3 scripts/verify_apk_evidence.py \
    "${apk}" \
    --contract "${RUN_CONTRACT}" \
    --apksigner "${APKSIGNER}" \
    --out "${receipt}"
done < <(find "${OUT_DIR}/signed" -maxdepth 1 -type f -name '*.apk' -print0)

(
  cd "${EVIDENCE_DIR}"
  find receipts -type f -name '*.json' -print0 | sort -z | xargs -0 sha256sum > RECEIPT_SHA256SUMS.txt
)

python3 - "${EVIDENCE_DIR}" "${SOURCE_COMMIT}" "${BUILD_RECEIPT_SHA256}" <<'PY'
import json
import sys
from pathlib import Path

evidence_dir = Path(sys.argv[1])
source_commit = sys.argv[2]
build_receipt_sha256 = sys.argv[3]
receipts = []
for path in sorted((evidence_dir / "receipts").glob("*.json")):
    data = json.loads(path.read_text(encoding="utf-8"))
    receipts.append({
        "receipt": str(path.relative_to(evidence_dir)),
        "artifact_sha256": data["artifact"]["sha256"],
        "status": data["status"],
        "provenance_claim_allowed": data["provenance_claim_allowed"],
    })
index = {
    "schema": "rafaelia.apk-evidence-index.v1",
    "source_commit": source_commit,
    "build_provenance_sha256": build_receipt_sha256,
    "receipts": receipts,
    "physical_runtime": "TOKEN_VAZIO",
    "claim_allowed": False,
}
(evidence_dir / "APK_EVIDENCE_INDEX.json").write_text(
    json.dumps(index, indent=2, sort_keys=True) + "\n", encoding="utf-8"
)
PY

info "Evidence gate complete"
cat "${EVIDENCE_DIR}/BUILD_PROVENANCE.json"
cat "${EVIDENCE_DIR}/APK_EVIDENCE_INDEX.json"
cat "${EVIDENCE_DIR}/RECEIPT_SHA256SUMS.txt"
