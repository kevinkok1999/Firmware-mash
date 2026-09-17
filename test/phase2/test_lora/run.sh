#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

out_dir="${TMPDIR:-/tmp}/firmware-mash-phase2"
mkdir -p "$out_dir"

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -Icomponents/mog_airtime/include \
  components/mog_airtime/mog_airtime.c \
  test/phase2/test_lora/test_lora.c \
  -o "$out_dir/test_lora"

"$out_dir/test_lora"
