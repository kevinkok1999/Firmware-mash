#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
foundation="${1:-${repo_root}/build/foundation/bramble}"
marker="${repo_root}/docs/BASELINE_APPROVED"

test -s "$marker" || { echo "BLOCKED: missing $marker" >&2; exit 1; }
test -d "$foundation/.git" || { echo "BLOCKED: foundation checkout missing: $foundation" >&2; exit 1; }

expected_commit="$(sed -n 's/^foundation_commit=//p' "$marker" | head -n1)"
actual_commit="$(git -C "$foundation" rev-parse HEAD)"
if [[ ! "$expected_commit" =~ ^[0-9a-f]{40}$ || "$actual_commit" != "$expected_commit" ]]; then
  echo "BLOCKED: overlay only applies to pinned foundation $expected_commit; got $actual_commit" >&2
  exit 1
fi

# Exact upstream blobs at the approved pin. If upstream integration files ever
# change, baseline/overlay review must be repeated instead of applying blindly.
declare -A expected_blob=(
  ["components/msg_store/CMakeLists.txt"]="26a0ae7c5dc9b2f0fbfd88c60cfdb7ae851ef02a"
  ["components/msg_store/msg_store_spiffs.c"]="dfdabad14de56d49e55d525d2e23e27eb3b1c7e5"
)

for path in "${!expected_blob[@]}"; do
  overlay_path="${repo_root}/overlays/bramble/${path}"
  target_path="${foundation}/${path}"
  test -s "$overlay_path" || { echo "BLOCKED: missing overlay $overlay_path" >&2; exit 1; }
  test -s "$target_path" || { echo "BLOCKED: missing foundation file $target_path" >&2; exit 1; }

  if cmp -s "$overlay_path" "$target_path"; then
    continue
  fi

  current_blob="$(git -C "$foundation" hash-object "$path")"
  if [[ "$current_blob" != "${expected_blob[$path]}" ]]; then
    echo "BLOCKED: unexpected upstream blob for $path" >&2
    echo "expected ${expected_blob[$path]} got $current_blob" >&2
    exit 1
  fi

done

rm -rf "${foundation}/components/mog_message_store"
cp -R "${repo_root}/components/mog_message_store" "${foundation}/components/mog_message_store"

for path in "${!expected_blob[@]}"; do
  cp "${repo_root}/overlays/bramble/${path}" "${foundation}/${path}"
done

# Fail on whitespace/conflict-marker damage in the assembled workspace.
git -C "$foundation" diff --check

# Prove that both expected overlay files are now byte-identical to source-of-truth.
for path in "${!expected_blob[@]}"; do
  cmp -s "${repo_root}/overlays/bramble/${path}" "${foundation}/${path}" || {
    echo "BLOCKED: overlay copy verification failed for $path" >&2
    exit 1
  }
done

echo "Firmware-mash overlay applied to Bramble $actual_commit"
