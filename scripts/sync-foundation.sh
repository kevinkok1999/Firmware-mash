#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
marker="${repo_root}/docs/BASELINE_APPROVED"
dest="${1:-${repo_root}/build/foundation/bramble}"

if [[ ! -s "$marker" ]]; then
  echo "BLOCKED: missing $marker" >&2
  exit 1
fi

foundation_repo="$(sed -n 's/^foundation_repo=//p' "$marker" | head -n1)"
foundation_commit="$(sed -n 's/^foundation_commit=//p' "$marker" | head -n1)"

if [[ -z "$foundation_repo" || ! "$foundation_commit" =~ ^[0-9a-f]{40}$ ]]; then
  echo "BLOCKED: invalid foundation pin in $marker" >&2
  exit 1
fi

case "$foundation_repo" in
  https://github.com/*/*.git|https://github.com/*/*) ;;
  *)
    echo "BLOCKED: foundation_repo must be an explicit public GitHub HTTPS repository" >&2
    exit 1
    ;;
esac

mkdir -p "$(dirname "$dest")"

if [[ ! -d "$dest/.git" ]]; then
  rm -rf "$dest"
  git clone --filter=blob:none --no-checkout "$foundation_repo" "$dest"
fi

git -C "$dest" remote set-url origin "$foundation_repo"
git -C "$dest" fetch --force --no-tags origin "$foundation_commit"
git -C "$dest" checkout --detach --force "$foundation_commit"
git -C "$dest" clean -ffdqx

actual="$(git -C "$dest" rev-parse HEAD)"
if [[ "$actual" != "$foundation_commit" ]]; then
  echo "BLOCKED: foundation SHA mismatch: expected $foundation_commit got $actual" >&2
  exit 1
fi

echo "Foundation ready: $foundation_repo@$actual"
