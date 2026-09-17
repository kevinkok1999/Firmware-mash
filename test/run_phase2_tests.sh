#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

required_suites=(
  test/phase2/test_core
  test/phase2/test_conversation
  test/phase2/test_energy
  test/phase2/test_lora
  test/phase2/test_routing
  test/phase2/test_reliability
  test/phase2/test_custody
  test/phase2/test_espnow
  test/phase2/test_ip_gateway
)

missing=0
for suite in "${required_suites[@]}"; do
  if [[ ! -d "$suite" ]]; then
    echo "BLOCKED: Phase 2 suite missing: $suite" >&2
    missing=1
  fi
done

if [[ "$missing" != "0" ]]; then
  echo "Phase 2 remains intentionally red until the communication-engine suites exist." >&2
  exit 1
fi

# Every suite owns one executable run.sh so a component can choose the right
# host/simulator tooling without teaching this top-level entry point details.
for suite in "${required_suites[@]}"; do
  [[ -s "$suite/run.sh" ]] || {
    echo "BLOCKED: missing $suite/run.sh" >&2
    exit 1
  }
  bash "$suite/run.sh"
done

# Mandatory configuration regression entry point: optional transports may not
# become hidden dependencies of the LoRa-only stable build.
[[ -s test/phase2/run_config_matrix.sh ]] || {
  echo "BLOCKED: missing Phase 2 config-matrix regression runner" >&2
  exit 1
}
bash test/phase2/run_config_matrix.sh

echo "Firmware-mash Phase 2 tests: PASS"
