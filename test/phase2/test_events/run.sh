#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
out_dir="${repo_root}/build-host-tests/phase2-events"
mkdir -p "$out_dir"

cc_bin="${CC:-cc}"
"$cc_bin" -std=c11 -Wall -Wextra -Werror \
  -I"${repo_root}/components/mog_core/include" \
  -I"${repo_root}/components/mog_events/include" \
  "${repo_root}/components/mog_core/mog_core.c" \
  "${repo_root}/components/mog_events/mog_events.c" \
  "${repo_root}/test/phase2/test_events/test_events.c" \
  -o "${out_dir}/test_events"

"${out_dir}/test_events"
