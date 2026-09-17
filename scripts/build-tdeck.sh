#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
foundation_dir="${MOG_FOUNDATION_DIR:-${repo_root}/build/foundation/bramble}"
expected_idf="v5.4.1"

if [[ "${MOG_FOUNDATION_READY:-0}" != "1" ]]; then
  bash "${repo_root}/scripts/sync-foundation.sh" "$foundation_dir"
fi

[[ -d "${foundation_dir}/.git" ]] || {
  echo "BLOCKED: foundation checkout missing" >&2
  exit 1
}

expected_commit="$(sed -n 's/^foundation_commit=//p' "${repo_root}/docs/BASELINE_APPROVED" | head -n1)"
actual_commit="$(git -C "$foundation_dir" rev-parse HEAD)"
[[ "$actual_commit" == "$expected_commit" ]] || {
  echo "BLOCKED: foundation commit mismatch" >&2
  exit 1
}

actual_idf="$(tr -d '\r\n' < "${foundation_dir}/.esp-idf-version")"
[[ "$actual_idf" == "$expected_idf" ]] || {
  echo "BLOCKED: expected ESP-IDF ${expected_idf}, foundation pins ${actual_idf}" >&2
  exit 1
}

build_native() {
  (
    cd "$foundation_dir"
    bash scripts/flash.sh local tdeck-plus build
  )
}

build_docker() {
  local image="espressif/idf:${expected_idf}"
  docker run --rm \
    -v "${foundation_dir}:/workspace" \
    -w /workspace \
    "$image" \
    bash -lc 'source "$IDF_PATH/export.sh" >/dev/null && bash scripts/flash.sh local tdeck-plus build'
}

if command -v idf.py >/dev/null 2>&1 && [[ -n "${IDF_PATH:-}" ]]; then
  build_native
elif command -v docker >/dev/null 2>&1; then
  build_docker
else
  echo "BLOCKED: install ESP-IDF ${expected_idf} or Docker to perform the T-Deck target build" >&2
  exit 1
fi

bin="${foundation_dir}/build-tdeck-plus/bramble.bin"
[[ -s "$bin" ]] || {
  echo "BLOCKED: T-Deck build completed without ${bin}" >&2
  exit 1
}

bytes="$(wc -c < "$bin" | tr -d ' ')"
sha="$(sha256sum "$bin" | awk '{print $1}')"
echo "T-Deck Plus overlay build: PASS"
echo "firmware_bin=${bin}"
echo "firmware_bin_bytes=${bytes}"
echo "firmware_sha256=${sha}"
