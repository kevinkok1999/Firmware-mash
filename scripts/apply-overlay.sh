#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
foundation_dir="${1:-${repo_root}/build/foundation/bramble}"
marker="${repo_root}/docs/BASELINE_APPROVED"

[[ -d "${foundation_dir}/.git" ]] || {
  echo "BLOCKED: foundation checkout missing at ${foundation_dir}" >&2
  exit 1
}
[[ -s "$marker" ]] || {
  echo "BLOCKED: baseline marker missing" >&2
  exit 1
}

expected_commit="$(sed -n 's/^foundation_commit=//p' "$marker" | head -n1)"
actual_commit="$(git -C "$foundation_dir" rev-parse HEAD)"
[[ "$actual_commit" == "$expected_commit" ]] || {
  echo "BLOCKED: refusing overlay on unpinned foundation: $actual_commit" >&2
  exit 1
}

# Verify the persistence backend before replacing it. Accept only either the
# exact approved upstream blob or the exact current Firmware-mash overlay blob;
# this makes the operation idempotent without becoming permissive.
upstream_store="${foundation_dir}/components/msg_store/msg_store_spiffs.c"
overlay_store="${repo_root}/overlay/bramble/components/msg_store/msg_store_spiffs.c"
expected_store_blob="dfdabad14de56d49e55d525d2e23e27eb3b1c7e5"
actual_store_blob="$(git -C "$foundation_dir" hash-object components/msg_store/msg_store_spiffs.c)"
overlay_store_blob="$(git hash-object "$overlay_store")"

if [[ "$actual_store_blob" != "$expected_store_blob" && \
      "$actual_store_blob" != "$overlay_store_blob" ]]; then
  echo "BLOCKED: msg_store_spiffs.c is neither approved upstream nor approved overlay: ${actual_store_blob}" >&2
  exit 1
fi

# Overlay only Firmware-mash-owned components. The upstream checkout itself
# remains detached at the pinned commit so provenance is always recoverable.
rm -rf "${foundation_dir}/components/mog_message_store"
cp -R "${repo_root}/components/mog_message_store" \
      "${foundation_dir}/components/mog_message_store"

# Replace the persistence adapter while preserving Bramble's public
# msg_store_spiffs_* API for the rest of the firmware.
if [[ "$actual_store_blob" != "$overlay_store_blob" ]]; then
  cp "$overlay_store" "$upstream_store"
fi

# Force ESP-IDF to compile/link the Firmware-mash durability component as part
# of Bramble's real msg_store dependency graph. This is intentionally
# idempotent and fails closed if the pinned upstream CMake shape changes.
cmake_file="${foundation_dir}/components/msg_store/CMakeLists.txt"
python3 - "$cmake_file" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
patched = 'PRIV_REQUIRES esp_timer spiffs mog_message_store'
if patched in text:
    pass
elif 'PRIV_REQUIRES esp_timer spiffs' in text:
    text = text.replace('PRIV_REQUIRES esp_timer spiffs', patched, 1)
    path.write_text(text)
else:
    raise SystemExit('BLOCKED: unexpected upstream msg_store CMakeLists.txt shape')
PY

overlay_commit="$(git -C "$repo_root" rev-parse HEAD 2>/dev/null || echo unknown)"
cat > "${foundation_dir}/.firmware-mash-overlay" <<EOF
foundation_commit=${expected_commit}
firmware_mash_commit=${overlay_commit}
message_store_adapter=transactional-journal-snapshot
EOF

echo "Firmware-mash overlay applied to ${foundation_dir}"
