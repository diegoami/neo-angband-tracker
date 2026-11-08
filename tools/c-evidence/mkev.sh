#!/bin/sh
TOOLS=$(cd "$(dirname "$0")" && pwd)
WORK=${CEV_WORK:-/tmp/c-evidence-work}
REPO=${CEV_REPO:-$(git -C "$TOOLS" rev-parse --show-toplevel)}
BUILD=${CEV_BUILD:-$WORK/build}
DEVSRC=${CEV_DEVSRC:-$WORK/dev-source}
# usage: mkev.sh slug msgfile   -> creates evidence branch with fixtures, pushes, extracts reproduce.py
set -e
slug=$1; msg=$2; cd $WORK
python3 $TOOLS/gen.py $slug/spec.json $slug/pkg >/dev/null
cp $slug/scenario.c $slug/pkg/ && cp ${CEV_BORG_TXT:-$WORK/borg.txt} $slug/pkg/borg.txt
cd $REPO
git worktree add -q -b task/$slug-evidence .worktrees/ev-$slug origin/master
D=.worktrees/ev-$slug/docs/audit/c-reports/$slug
mkdir -p $D/profile && cp $WORK/$slug/pkg/* $D/ && cp ${CEV_BORG_TXT:-$WORK/borg.txt} $D/profile/
cd .worktrees/ev-$slug && git add docs && git commit -q -F $msg && git push -q -u origin task/$slug-evidence 2>&1 | grep -v "^remote" || true
git rev-parse HEAD
cd $REPO
git fetch -q https://github.com/diegoami/neo-angband-tracker.git task/$slug
git fetch -q https://github.com/diegoami/neo-angband-tracker.git task/$slug-evidence
git show FETCH_HEAD:docs/audit/c-reports/$slug/reproduce.py > $WORK/reproduce-$slug.py
