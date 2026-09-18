#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

# Never package around a red lower-level gate.
bash scripts/phase-gate.sh 3

source_dir="${MOG_RELEASE_SOURCE_DIR:-build-artifacts/firmware-mash/tdeck-plus}"
out_dir="${MOG_RELEASE_OUT_DIR:-release/firmware-mash-tdeck-plus}"

required=(
  firmware.bin
  bootloader.bin
  partition-table.bin
  ota_data_initial.bin
  flasher_args.json
  SHA256SUMS.txt
  BUILD_METADATA.txt
)
for rel in "${required[@]}"; do
  [[ -s "$source_dir/$rel" ]] || {
    echo "BLOCKED: release source missing $source_dir/$rel" >&2
    exit 1
  }
done

# Phase 3 must eventually generate these from the actual target build and
# validated product metadata. They are deliberately required instead of being
# guessed here.
required_generated=(
  build-artifacts/release/release-manifest.json
  build-artifacts/release/flasher-manifest.json
  build-artifacts/release/recovery.txt
)
for path in "${required_generated[@]}"; do
  [[ -s "$path" ]] || {
    echo "BLOCKED: generated release artifact missing: $path" >&2
    exit 1
  }
done

rm -rf "$out_dir"
mkdir -p "$out_dir"
cp "$source_dir"/firmware.bin "$out_dir/"
cp "$source_dir"/bootloader.bin "$out_dir/"
cp "$source_dir"/partition-table.bin "$out_dir/"
cp "$source_dir"/ota_data_initial.bin "$out_dir/"
cp "$source_dir"/flasher_args.json "$out_dir/"
cp "$source_dir"/BUILD_METADATA.txt "$out_dir/"
cp build-artifacts/release/release-manifest.json "$out_dir/"
cp build-artifacts/release/flasher-manifest.json "$out_dir/"
cp build-artifacts/release/recovery.txt "$out_dir/"

(
  cd "$out_dir"
  sha256sum \
    firmware.bin bootloader.bin partition-table.bin ota_data_initial.bin \
    flasher_args.json BUILD_METADATA.txt release-manifest.json \
    flasher-manifest.json recovery.txt > SHA256SUMS.txt
)

echo "Firmware-mash release package: PASS"
echo "package_dir=$out_dir"
