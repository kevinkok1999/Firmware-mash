#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_root}/build-host-tests"
mkdir -p "$build_dir"

cc_bin="${CC:-cc}"
common_flags=(-std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L)
include_flags=(-I"${repo_root}/components/mog_message_store/include")

"$cc_bin" "${common_flags[@]}" "${include_flags[@]}" \
  "${repo_root}/components/mog_message_store/mog_store_snapshot.c" \
  "${repo_root}/test/host/test_mog_store_snapshot.c" \
  -o "${build_dir}/test_mog_store_snapshot"

"$cc_bin" "${common_flags[@]}" "${include_flags[@]}" \
  "${repo_root}/components/mog_message_store/mog_store_snapshot.c" \
  "${repo_root}/components/mog_message_store/mog_store_journal.c" \
  "${repo_root}/test/host/test_mog_store_journal.c" \
  -o "${build_dir}/test_mog_store_journal"

"$cc_bin" "${common_flags[@]}" "${include_flags[@]}" \
  "${repo_root}/components/mog_message_store/mog_store_snapshot.c" \
  "${repo_root}/components/mog_message_store/mog_store_journal.c" \
  "${repo_root}/components/mog_message_store/mog_store_state.c" \
  "${repo_root}/test/host/test_mog_store_state.c" \
  -o "${build_dir}/test_mog_store_state"

# Compile the durable stored_msg_t layout contract independently. Any upstream
# field-order/width/padding drift must fail the build and force an explicit
# storage-schema migration review instead of silently reinterpreting bytes.
"$cc_bin" "${common_flags[@]}" \
  -I"${repo_root}/test/host/bramble_shim" \
  -c "${repo_root}/overlay/bramble/components/msg_store/mog_msg_store_layout_guard.c" \
  -o "${build_dir}/mog_msg_store_layout_guard.o"

# Compile the actual Bramble adapter's ESP_PLATFORM branch against lightweight
# host shims. Only the mount path strings are redirected from /spiffs to /tmp;
# the adapter logic itself is the production overlay source.
adapter_src="${build_dir}/msg_store_spiffs_host.c"
sed 's#"/spiffs/#"/tmp/mog-spiffs/#g' \
  "${repo_root}/overlay/bramble/components/msg_store/msg_store_spiffs.c" \
  > "$adapter_src"

"$cc_bin" "${common_flags[@]}" -DESP_PLATFORM \
  -I"${repo_root}/test/host/bramble_shim" \
  "${include_flags[@]}" \
  "${repo_root}/components/mog_message_store/mog_store_snapshot.c" \
  "${repo_root}/components/mog_message_store/mog_store_journal.c" \
  "${repo_root}/components/mog_message_store/mog_store_state.c" \
  "$adapter_src" \
  "${repo_root}/test/host/test_mog_bramble_adapter.c" \
  -o "${build_dir}/test_mog_bramble_adapter"

"${build_dir}/test_mog_store_snapshot"
"${build_dir}/test_mog_store_journal"
"${build_dir}/test_mog_store_state"

rm -rf /tmp/mog-spiffs
mkdir -p /tmp/mog-spiffs
adapter_test="${build_dir}/test_mog_bramble_adapter"

"$adapter_test" write-timestamp
"$adapter_test" read-timestamp
"$adapter_test" fill
"$adapter_test" read-fill
"$adapter_test" torn-setup

python3 - <<'PY'
from pathlib import Path
path = Path('/tmp/mog-spiffs/mog-msg-journal.bin')
data = path.read_bytes()
assert len(data) > 10
path.write_bytes(data[:-10])
PY

"$adapter_test" torn-read-append
"$adapter_test" torn-verify
"$adapter_test" clean

echo "Firmware-mash host tests: PASS"
