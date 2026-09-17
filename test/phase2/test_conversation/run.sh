#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

out_dir="${TMPDIR:-/tmp}/firmware-mash-phase2"
mkdir -p "$out_dir"

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -Icomponents/mog_core/include \
  -Icomponents/mog_conversation/include \
  components/mog_core/mog_core.c \
  components/mog_conversation/mog_conversation.c \
  test/phase2/test_conversation/test_conversation.c \
  -o "$out_dir/test_conversation"

"$out_dir/test_conversation"
