#!/usr/bin/env python3
"""Build README.md for an evidence package from its spec and the verified run.
usage: mkreadme.py spec.json run_root fixtures_sha fix_sha out_README"""
import json, re, sys
from pathlib import Path
spec = json.loads(Path(sys.argv[1]).read_text())
root, fixtures, fix, out = Path(sys.argv[2]), sys.argv[3], sys.argv[4], Path(sys.argv[5])
res = json.loads((root / 'result.json').read_text())
slug, issue = spec['slug'], spec['issue']
cases = res['cases']
gates = res['full_tests']

def verdict(phase, mode):
    r = cases[phase][mode]
    return ('passed' if r['passed'] else 'failed') + ', ' + spec['card_key'] + '=' + str(
        r.get('outcome', {}).get(spec['card_key'], r.get('state', {}).get(spec['card_key'], 'n/a')))

rows = []
for m in spec['modes']:
    if m.get('control'):
        rows.append(f"| {m['label']} | {verdict('before', m['name'])} | byte-identical log |")
    else:
        rows.append(f"| {m['label']} | {verdict('before', m['name'])} | {verdict('after', m['name'])} |")

cmds = f"""```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/{slug}
git fetch https://github.com/diegoami/neo-angband-tracker.git task/{slug}-evidence
git show {fixtures}:docs/audit/c-reports/{slug}/reproduce.py > /tmp/reproduce-{slug}.py
python3 /tmp/reproduce-{slug}.py --full-tests --fixtures-ref {fixtures} --fix-ref {fix}
```"""

text = f"""# {spec['title']}

This is **{slug} / issue{issue}**, a C Borg defect
(diegoami/neo-angband-tracker#{issue}).

{spec['intro']}

## What to watch

{spec['card_label'][0].upper() + spec['card_label'][1:]}; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `{res['base'][:9]}`) | Corrected (fix `{fix[:9]}`) |
| --- | --- | --- |
""" + "\n".join(rows) + f"""

""" + "\n\n".join(f"- **{m['label']}.** {m['explanation']}" for m in spec['modes']) + f"""

## Conditions

{spec['conditions']}

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

{cmds}

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `{', '.join(sorted(spec['changes']))}`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `{res['base']}`, fix `{fix}` (branch `task/{slug}`), fixtures
  `{fixtures}` (branch `task/{slug}-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: {', '.join(res['identical_controls']) or 'none'}
- full native gates: before {gates['before']['units']} unit checks and {gates['before']['frontends']} frontend tests;
  after {gates['after']['units']} and {gates['after']['frontends']}
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

{spec['claim']}

{spec.get('limits', '')}

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{{before,after}}-build.log`,
`run-{{before,after}}-scenarios.log`, `run-{{before,after}}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
"""
out.write_text(text)
print('README written', out)
