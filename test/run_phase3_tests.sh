#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

required_suites=(
  test/phase3/test_ui_contract
  test/phase3/test_update_recovery
  test/phase3/test_release_manifest
  test/phase3/test_flasher_manifest
)

missing=0
for suite in "${required_suites[@]}"; do
  if [[ ! -d "$suite" ]]; then
    echo "BLOCKED: Phase 3 suite missing: $suite" >&2
    missing=1
  fi
done

if [[ "$missing" != "0" ]]; then
  echo "Phase 3 remains intentionally red until product/release suites exist." >&2
  exit 1
fi

for suite in "${required_suites[@]}"; do
  [[ -s "$suite/run.sh" ]] || {
    echo "BLOCKED: missing $suite/run.sh" >&2
    exit 1
  }
  bash "$suite/run.sh"
done

# STABLE is impossible without explicit physical evidence. BETA/DEVELOPMENT
# packaging may use a different release path later, but the user-facing stable
# flasher gate must never silently infer these results.
required_evidence=(
  evidence/hardware/tdeck-plus/no-sd-boot.PASS
  evidence/hardware/tdeck-plus/reboot-persistence.PASS
  evidence/hardware/tdeck-plus/power-cut-recovery.PASS
  evidence/hardware/tdeck-plus/lora-send-receive.PASS
  evidence/hardware/tdeck-plus/update-rollback.PASS
)
for marker in "${required_evidence[@]}"; do
  [[ -s "$marker" ]] || {
    echo "BLOCKED: missing STABLE hardware evidence: $marker" >&2
    exit 1
  }
done

echo "Firmware-mash Phase 3 tests: PASS"
