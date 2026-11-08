#!/usr/bin/env python3
"""Replay the recorded D0008 bundle with an alternate engine (diagnostic only).
usage: run_d0008.py <engine> <new output dir>"""
import importlib.util, json, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
ORIGINAL = Path('/home/diego/projects/angband/better-borg-c/artifacts/death-workflow/collections/20261107T230736.548607Z/game-001-seed-1668846659')
spec = importlib.util.spec_from_file_location('replay_borg', HERE / 'replay-borg.py')
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
engine, out = Path(sys.argv[1]).resolve(), Path(sys.argv[2])
r = m.verify(ORIGINAL, out, 600, engine=engine)
print(json.dumps({k: r[k] for k in ('verified','complete','exit_code','interrupted','error','first_difference','engine_sha256','original_engine_sha256')}, indent=1, default=str))
