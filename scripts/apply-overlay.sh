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

# Overlay only Firmware-mash-owned components. The upstream checkout itself
# remains detached at the pinned commit so provenance is always recoverable.
rm -rf "${foundation_dir}/components/mog_message_store"
cp -R "${repo_root}/components/mog_message_store" \
      "${foundation_dir}/components/mog_message_store"

# Force ESP-IDF to compile the Firmware-mash storage component as part of the
# real Bramble msg_store dependency graph. This is intentionally idempotent and
# fails closed if the pinned upstream CMake shape unexpectedly changes.
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
EOF

echo "Firmware-mash overlay applied to ${foundation_dir}"
