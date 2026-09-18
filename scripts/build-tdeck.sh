#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
foundation_dir="${MOG_FOUNDATION_DIR:-${repo_root}/build/foundation/bramble}"
out_dir="${MOG_BUILD_ARTIFACT_DIR:-${repo_root}/build-artifacts/firmware-mash/tdeck-plus}"
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

actual_idf_pin="$(tr -d '\r\n' < "${foundation_dir}/.esp-idf-version")"
[[ "$actual_idf_pin" == "$expected_idf" ]] || {
  echo "BLOCKED: expected ESP-IDF ${expected_idf}, foundation pins ${actual_idf_pin}" >&2
  exit 1
}

# This is the decisive step: a target build is not Phase-1 evidence unless it
# contains the reviewed Firmware-mash overlay on the exact approved foundation.
bash "${repo_root}/scripts/apply-overlay.sh" "$foundation_dir"

build_native() {
  local version
  version="$(idf.py --version 2>/dev/null || true)"
  [[ "$version" == *"${expected_idf}"* ]] || {
    echo "BLOCKED: native idf.py is not ${expected_idf}: ${version:-unknown}" >&2
    exit 1
  }
  (
    cd "$foundation_dir"
    export BRAMBLE_OTA_ALLOW_GENERATED_KEY=1
    bash scripts/flash.sh local tdeck-plus build
    bash scripts/ci/check-firmware-size.sh tdeck-plus
  )
}

build_docker() {
  local image="espressif/idf:${expected_idf}"
  docker run --rm \
    -e BRAMBLE_OTA_ALLOW_GENERATED_KEY=1 \
    -v "${foundation_dir}:/workspace" \
    -w /workspace \
    "$image" \
    bash -lc 'source "$IDF_PATH/export.sh" >/dev/null && bash scripts/flash.sh local tdeck-plus build && bash scripts/ci/check-firmware-size.sh tdeck-plus'
}

if command -v idf.py >/dev/null 2>&1 && [[ -n "${IDF_PATH:-}" ]]; then
  build_native
elif command -v docker >/dev/null 2>&1; then
  build_docker
else
  echo "BLOCKED: install ESP-IDF ${expected_idf} or Docker to perform the T-Deck target build" >&2
  exit 1
fi

build_dir="${foundation_dir}/build-tdeck-plus"
required=(
  bramble.bin
  bramble.elf
  bramble.map
  bootloader/bootloader.bin
  partition_table/partition-table.bin
  ota_data_initial.bin
  flasher_args.json
)
for rel in "${required[@]}"; do
  [[ -s "${build_dir}/${rel}" ]] || {
    echo "BLOCKED: target build did not produce ${rel}" >&2
    exit 1
  }
done

rm -rf "$out_dir"
mkdir -p "$out_dir"
cp "${build_dir}/bramble.bin" "$out_dir/firmware.bin"
cp "${build_dir}/bramble.elf" "$out_dir/firmware.elf"
cp "${build_dir}/bramble.map" "$out_dir/firmware.map"
cp "${build_dir}/bootloader/bootloader.bin" "$out_dir/bootloader.bin"
cp "${build_dir}/partition_table/partition-table.bin" "$out_dir/partition-table.bin"
cp "${build_dir}/ota_data_initial.bin" "$out_dir/ota_data_initial.bin"
cp "${build_dir}/flasher_args.json" "$out_dir/flasher_args.json"

(
  cd "$out_dir"
  sha256sum \
    firmware.bin firmware.elf firmware.map bootloader.bin \
    partition-table.bin ota_data_initial.bin flasher_args.json \
    > SHA256SUMS.txt
)

bytes="$(wc -c < "$out_dir/firmware.bin" | tr -d ' ')"
sha="$(sha256sum "$out_dir/firmware.bin" | awk '{print $1}')"
project_sha="$(git -C "$repo_root" rev-parse HEAD 2>/dev/null || printf 'unknown')"
cat > "$out_dir/BUILD_METADATA.txt" <<EOF
board=tdeck-plus
foundation_sha=${actual_commit}
firmware_mash_sha=${project_sha}
esp_idf=${expected_idf}
release_tier=DEVELOPMENT
flashable_claim=NO
EOF

echo "T-Deck Plus Firmware-mash overlay build: PASS"
echo "firmware_bin=${out_dir}/firmware.bin"
echo "firmware_bin_bytes=${bytes}"
echo "firmware_sha256=${sha}"
echo "NOTE: development target build only; Phase 3 + hardware evidence are required for a user-facing flasher release."
