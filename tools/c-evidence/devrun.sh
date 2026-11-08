#!/bin/sh
TOOLS=$(cd "$(dirname "$0")" && pwd)
WORK=${CEV_WORK:-/tmp/c-evidence-work}
REPO=${CEV_REPO:-$(git -C "$TOOLS" rev-parse --show-toplevel)}
BUILD=${CEV_BUILD:-$WORK/build}
DEVSRC=${CEV_DEVSRC:-$WORK/dev-source}
# usage: devrun.sh slug Observer PREFIX mode
slug=$1; obs=$2; pre=$3; mode=$4
prof=$WORK/$slug/profile; mkdir -p $prof; cp ${CEV_BORG_TXT:-$WORK/borg.txt} $prof/borg.txt
cd $BUILD/game && env ${pre}_PROFILE=$prof ${pre}_MODE=$mode ./$obs -v 2>&1 | sed 's/\x1b\[[0-9;]*m//g'
