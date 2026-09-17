#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
phase="${1:-}"

if [[ ! "$phase" =~ ^[123]$ ]]; then
  echo "usage: bash scripts/phase-gate.sh <1|2|3>" >&2
  exit 2
fi

cd "$repo_root"

require_file() {
  [[ -s "$1" ]] || { echo "BLOCKED: missing or empty $1" >&2; exit 1; }
}

require_dir() {
  [[ -d "$1" ]] || { echo "BLOCKED: missing directory $1" >&2; exit 1; }
}

require_marker_field() {
  local pattern="$1"
  grep -Eq "$pattern" docs/BASELINE_APPROVED || {
    echo "BLOCKED: baseline marker does not satisfy $pattern" >&2
    exit 1
  }
}

phase1() {
  echo "== Phase 1: foundation + durable core =="
  require_file docs/BASELINE_APPROVED
  require_marker_field '^foundation_repo=.+$'
  require_marker_field '^foundation_commit=[0-9a-f]{40}$'
  require_marker_field '^tdeck_build=PASS$'
  require_marker_field '^baseline_tests=PASS$'
  require_marker_field '^simulator_tests=PASS$'
  require_marker_field '^license_review=PASS$'
  require_marker_field '^no_sd_dependency_review=PASS$'

  require_file docs/THREE_PHASE_IMPLEMENTATION_PLAN.md
  require_file docs/REPOSITORY_LAYOUT.md
  require_file docs/BASELINE_EVIDENCE_2026-09-17.md
  require_file docs/BASELINE_PROVENANCE_2026-09-17.md
  require_file docs/PHASE1_STORAGE_DURABILITY_DESIGN.md

  require_file components/mog_message_store/CMakeLists.txt
  require_file components/mog_message_store/include/mog_store_snapshot.h
  require_file components/mog_message_store/mog_store_snapshot.c
  require_file components/mog_message_store/include/mog_store_journal.h
  require_file components/mog_message_store/mog_store_journal.c
  require_file components/mog_message_store/include/mog_store_state.h
  require_file components/mog_message_store/mog_store_state.c
  require_file overlay/bramble/components/msg_store/msg_store_spiffs.c

  require_file test/host/test_mog_store_snapshot.c
  require_file test/host/test_mog_store_journal.c
  require_file test/host/test_mog_store_state.c
  require_file test/host/test_mog_bramble_adapter.c
  require_file test/host/bramble_shim/msg_store.h
  require_file test/host/bramble_shim/msg_store_spiffs.h
  require_file test/host/bramble_shim/sdkconfig.h
  require_file test/host/bramble_shim/esp_heap_caps.h
  require_file test/host/bramble_shim/esp_spiffs.h
  require_file test/host/bramble_shim/esp_log.h
  require_file test/run_host_tests.sh

  require_file scripts/sync-foundation.sh
  require_file scripts/apply-overlay.sh
  require_file scripts/build-tdeck.sh

  bash test/run_host_tests.sh

  if [[ "${MOG_SKIP_TARGET_BUILD:-0}" != "1" ]]; then
    bash scripts/build-tdeck.sh
  elif [[ "${MOG_SKIP_FOUNDATION_SYNC:-0}" != "1" ]]; then
    echo "NOTE: target build skipped; foundation + overlay sync still verified"
    bash scripts/sync-foundation.sh
    bash scripts/apply-overlay.sh
  else
    echo "NOTE: target build and foundation sync explicitly skipped"
  fi

  echo "PHASE 1 PASS"
}

phase2() {
  phase1
  echo "== Phase 2: communication engine =="

  local required_components=(
    mog_core
    mog_packet
    mog_events
    mog_conversation
    mog_messaging
    mog_energy
    mog_airtime
    mog_transport_lora
    mog_neighbor
    mog_routing
    mog_reliability
    mog_dedup
    mog_custody
    mog_radio_scheduler
    mog_transport_espnow
    mog_transport_ip
    mog_gateway
    mog_health
    mog_metrics
  )

  for component in "${required_components[@]}"; do
    require_dir "components/${component}"
    require_file "components/${component}/CMakeLists.txt"
  done

  require_file docs/PACKET_DELIVERY_CONTRACT.md
  require_file docs/STORE_CARRY_CUSTODY_CONTRACT.md
  require_file docs/IP_GATEWAY_FEDERATION_CONTRACT.md
  require_file docs/ENERGY_MANAGEMENT_CONTRACT.md
  require_file docs/BUILD_CONFIG_MATRIX.md
  require_file docs/TEST_TRACEABILITY.md

  require_file test/run_phase2_tests.sh
  bash test/run_phase2_tests.sh

  echo "PHASE 2 PASS"
}

phase3() {
  phase2
  echo "== Phase 3: product + release/flasher =="

  require_file docs/UI_UX_CONTRACT.md
  require_file docs/UI_IMPLEMENTATION_MAP.md
  require_file docs/FLASHER_RELEASE_CONTRACT.md
  require_file docs/USER_RELEASE_CONTRACT.md
  require_file scripts/release-package.sh
  require_file test/run_phase3_tests.sh

  bash test/run_phase3_tests.sh

  echo "PHASE 3 PASS"
}

case "$phase" in
  1) phase1 ;;
  2) phase2 ;;
  3) phase3 ;;
esac
