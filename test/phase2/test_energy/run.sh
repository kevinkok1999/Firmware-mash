#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

out_dir="${TMPDIR:-/tmp}/firmware-mash-phase2"
mkdir -p "$out_dir"

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -Icomponents/mog_energy/include \
  components/mog_energy/mog_energy.c \
  test/phase2/test_energy/test_energy.c \
  -o "$out_dir/test_energy"

"$out_dir/test_energy"
