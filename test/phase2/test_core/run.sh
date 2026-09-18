#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
out_dir="${repo_root}/build-host-tests/phase2-core"
mkdir -p "$out_dir"

cc_bin="${CC:-cc}"
"$cc_bin" -std=c11 -Wall -Wextra -Werror \
  -I"${repo_root}/components/mog_core/include" \
  "${repo_root}/components/mog_core/mog_core.c" \
  "${repo_root}/test/phase2/test_core/test_core.c" \
  -o "${out_dir}/test_core"

"${out_dir}/test_core"
