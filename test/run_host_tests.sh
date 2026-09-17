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

"${build_dir}/test_mog_store_snapshot"
"${build_dir}/test_mog_store_journal"
"${build_dir}/test_mog_store_state"

echo "Firmware-mash host tests: PASS"
