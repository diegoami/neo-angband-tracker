#!/usr/bin/env python3
"""Run an engine live (no replay/record channels) with the D0008 profile: seed 1668846659, character Measure, cap from the record.
usage: run_live.py <engine> <new output dir> [cap] [timeout_s]"""
import os, shutil, subprocess, sys
from pathlib import Path
O = Path('/home/diego/projects/angband/better-borg-c/artifacts/death-workflow/collections/20261107T230736.548607Z/game-001-seed-1668846659')
engine, out = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
cap = sys.argv[3] if len(sys.argv) > 3 else '200000'
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 300
out.mkdir(parents=True)
shutil.copytree(O / 'initial-user', out / 'user')
for d in ('save', 'scores', 'panic', 'archive'): (out / d).mkdir()
env = {k: v for k, v in os.environ.items() if not k.startswith(('BORG_MEASURE_', 'BORG_RECORD_', 'BORG_REPLAY_'))}
env.update(LANG='C.UTF-8', LC_ALL='C.UTF-8', BORG_MEASURE_SEED=os.environ.get('D0008_SEED','1668846659'), BORG_MEASURE_CAP=cap,
           BORG_MEASURE_DIR=str(out), BORG_MEASURE_FINAL_SAVE=str(out / 'final.save'))
cmd = [str(engine), '-mtest'] + [f'-d{k}={out / k}' for k in ('user', 'save', 'scores', 'panic', 'archive')]
cmd += [f'-d{k}={O / "data" / n}' for k, n in (('gamedata', 'gamedata'), ('pref', 'customize'), ('screens', 'screens'), ('help', 'help'))]
cmd += ['-uMeasure']
shutil.copy2(O / 'input.txt', out / 'input.txt')
with open('/tmp/angband-validation.lock', 'a') as g, (out / 'terminal.log').open('w') as log, (out / 'input.txt').open() as inp:
    import fcntl; fcntl.flock(g, fcntl.LOCK_EX)
    try:
        p = subprocess.run(cmd, cwd=out, env=env, stdin=inp, stdout=log, stderr=subprocess.STDOUT, timeout=timeout)
        print('exit', p.returncode)
    except subprocess.TimeoutExpired:
        print('timeout after', timeout, 's')
