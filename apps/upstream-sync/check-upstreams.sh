#!/usr/bin/env bash
#
# Gate for the weekly upstream sync (.github/workflows/upstream-sync.yml):
# fetch each synced repo's upstream, count the new commits, and decide
# whether the sync should run — not when there is nothing new, nor when a
# previous cycle's upstream-sync PRs are still open. Outputs (run/core/pb/ah)
# go to $GITHUB_OUTPUT; when run locally they print to stdout instead.
#
# Requires full (non-shallow) clones and an authenticated gh (GH_TOKEN).

set -euo pipefail
cd "$(dirname "$0")/../.."

# Prints the number of upstream commits not yet merged into origin/main.
# Idempotent about the remote so the script also works in a local checkout
# where an upstream remote is already configured.
fetch_upstream() { # <dir> <url> <branch>
  git -C "$1" remote add upstream "$2" 2>/dev/null \
    || git -C "$1" remote set-url upstream "$2"
  git -C "$1" fetch --quiet upstream "$3"
  git -C "$1" rev-list --count "origin/main..upstream/$3"
}

core=$(fetch_upstream . https://github.com/mod-playerbots/azerothcore-wotlk.git Playerbot)
pb=$(fetch_upstream modules/mod-playerbots https://github.com/mod-playerbots/mod-playerbots.git master)
ah=$(fetch_upstream modules/mod-ah-bot-plus https://github.com/NathanHandley/mod-ah-bot-plus.git master)

echo "New upstream commits: core=$core mod-playerbots=$pb mod-ah-bot-plus=$ah"

# Don't stack sync cycles: skip while any previous upstream-sync PR is open.
open=0
for repo in azerothcore mod-playerbots mod-ah-bot-plus configs; do
  n=$(gh pr list --repo "felworld/$repo" --state open --json headRefName \
    --jq '[.[] | select(.headRefName | startswith("upstream-sync/"))] | length')
  open=$((open + n))
done

run=false
if [ "$open" -gt 0 ]; then
  echo "::notice::Skipping: $open upstream-sync PR(s) from a previous cycle are still open."
elif [ $((core + pb + ah)) -eq 0 ]; then
  echo "::notice::All upstreams already merged; nothing to do."
else
  run=true
fi

{
  echo "run=$run"
  echo "core=$core"
  echo "pb=$pb"
  echo "ah=$ah"
} >> "${GITHUB_OUTPUT:-/dev/stdout}"
