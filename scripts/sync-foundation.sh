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

if [[ "$foundation_repo" =~ ^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$ ]]; then
  foundation_url="https://github.com/${foundation_repo}.git"
elif [[ "$foundation_repo" =~ ^https://github.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+(\.git)?$ ]]; then
  foundation_url="$foundation_repo"
else
  echo "BLOCKED: foundation_repo must be owner/repo or an explicit public GitHub HTTPS repository" >&2
  exit 1
fi

mkdir -p "$(dirname "$dest")"

if [[ ! -d "$dest/.git" ]]; then
  rm -rf "$dest"
  git clone --filter=blob:none --no-checkout "$foundation_url" "$dest"
fi

git -C "$dest" remote set-url origin "$foundation_url"
git -C "$dest" fetch --force --no-tags origin "$foundation_commit"
git -C "$dest" checkout --detach --force "$foundation_commit"
git -C "$dest" clean -ffdqx

actual="$(git -C "$dest" rev-parse HEAD)"
if [[ "$actual" != "$foundation_commit" ]]; then
  echo "BLOCKED: foundation SHA mismatch: expected $foundation_commit got $actual" >&2
  exit 1
fi

bash "${repo_root}/scripts/apply-overlay.sh" "$dest"

echo "Foundation + Firmware-mash overlay ready: $foundation_repo@$actual"
