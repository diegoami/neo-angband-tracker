#!/bin/sh
TOOLS=$(cd "$(dirname "$0")" && pwd)
WORK=${CEV_WORK:-/tmp/c-evidence-work}
REPO=${CEV_REPO:-$(dirname "$(git -C "$TOOLS" rev-parse --path-format=absolute --git-common-dir)")}
BUILD=${CEV_BUILD:-$WORK/build}
DEVSRC=${CEV_DEVSRC:-$WORK/dev-source}
# usage: finish.sh slug fixtures_sha fix_sha  -> README, package, commit (with logs), push; prints receipt sha
set -e
set -o pipefail 2>/dev/null || true
slug=$1; fx=$2; fix=$3
R=$(ls -dt /tmp/angband-$slug-* | head -1)
EV=$REPO/.worktrees/ev-$slug/docs/audit/c-reports/$slug
cd $WORK
python3 $TOOLS/mkreadme.py $slug/spec.json $R $fx $fix $EV/README.md >/dev/null
python3 $TOOLS/package.py $slug/spec.json $R $EV $fx $fix > $WORK/$slug/package.log 2>&1 || { cat $WORK/$slug/package.log; echo "package/render check failed: nothing committed"; exit 1; }
tail -1 $WORK/$slug/package.log
cd $REPO/.worktrees/ev-$slug
git add -f docs/audit/c-reports/$slug
git commit -q -m "Record the $slug verification receipt

Matrix --full-tests green per phase at fixtures ${fx%${fx#?????????}} and fix ${fix%${fix#?????????}}.
README, receipt with sha256 of every file, all phase logs (scenario,
configure, build and full-gate), replay page and data; replay
render-checked headless with no console errors."
git push -q origin task/$slug-evidence 2>&1 | grep -v "^remote" || true
git rev-parse HEAD
