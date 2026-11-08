#!/bin/bash
cd "$(dirname "$0")"; rm -rf sweep; mkdir sweep
for seed in 1668846659 101 202 303 404 505 606 707 808 909; do
  for v in ctrl fix; do
    D0008_SEED=$seed timeout 150 python3 -I run_live.py $PWD/build-$v/game/angband $PWD/sweep/$v-$seed 30000 120 > sweep/$v-$seed.out 2>&1
    t=sweep/$v-$seed/trace.txt
    cause=$(grep -m1 '^CAUSE' $t 2>/dev/null | cut -c7-); st=$(grep '^STATE' $t 2>/dev/null | tail -1 | grep -oE 'turn=[0-9]+|level=[0-9]+|depth=[0-9]+' | tr '\n' ' ')
    echo "$seed $v | ${cause:-NO-CAUSE} | $st | $(cat sweep/$v-$seed.out | tr '\n' ' ')" >> sweep/summary.txt
  done
done
echo DONE >> sweep/summary.txt
