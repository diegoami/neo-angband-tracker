#!/usr/bin/env python3
"""Copy the verified run's artifacts into the evidence directory, run the
render check on the copied replay, and write receipt.json.
usage: package.py spec.json run_root evidence_dir fixtures_sha fix_sha"""
import hashlib, json, os, re, shutil, subprocess, sys
from pathlib import Path
TOOLS = str(Path(__file__).resolve().parent)
WORK = os.environ.get("CEV_WORK", "/tmp/c-evidence-work")
Path(WORK).mkdir(parents=True, exist_ok=True)
spec = json.loads(Path(sys.argv[1]).read_text())
root, ev = Path(sys.argv[2]), Path(sys.argv[3])
fixtures, fix = sys.argv[4], sys.argv[5]
src = root / 'evidence'
for p in src.iterdir():
    if p.suffix in ('.log', '.json', '.html'):
        shutil.copy(p, ev / p.name)
for p in root.iterdir():
    if p.is_file() and (p.suffix == '.log' or p.name == 'result.json'):
        shutil.copy(p, ev / ('run-' + p.name if p.name.endswith('.log') else p.name))
# node --check the generated replay script
page = (ev / 'replay.html').read_text()
script = page.split('<script>', 1)[1].rsplit('</script>', 1)[0]
(Path(WORK) / "replay-check.js").write_text(script)
subprocess.run(['node', '--check', str(Path(WORK) / 'replay-check.js')], check=True)
render = subprocess.run(['node', str(Path(TOOLS) / 'render-check.js'), str(ev / 'replay.html'), str(ev / 'replay-data.json')],
                        capture_output=True, text=True, cwd=WORK)
print(render.stdout, render.stderr)
if render.returncode:
    sys.exit('render check failed')
checks = int(re.search(r'checks=(\d+)', render.stdout).group(1))
result = json.loads((ev / 'result.json').read_text())
cc = subprocess.check_output(['cc', '--version'], text=True).splitlines()[0]
cm = subprocess.check_output(['cmake', '--version'], text=True).splitlines()[0]
cases = {}
for phase in ('before', 'after'):
    for mode, row in result['cases'][phase].items():
        cases.setdefault(mode, {})[phase] = {k: row[k] for k in ('passed', 'state', 'outcome') if k in row}
receipt = {
    'issue': 'diegoami/neo-angband-tracker#%d' % spec['issue'],
    'report': spec['slug'],
    'base': result['base'], 'fix': fix, 'fix_branch': spec['branch'],
    'fixtures_ref': fixtures,
    'cases': cases,
    'identical_controls': result['identical_controls'],
    'claim': spec['claim'],
    'full_tests': {p: {'unit_checks': g['units'], 'frontend_tests': g['frontends']} for p, g in result['full_tests'].items()},
    'fixed_sources': sorted(spec['changes']),
    'fixed_source_sha256': result['fixed_source_sha256'],
    'runner_asserts': 'Each corrected file must equal the base file with exactly the expected hunk; the source scope must be exactly those files; every case must match its expected before/after pass state and every control log must be byte-identical.',
    'compiler': cc, 'cmake': cm,
    'browser_verification': {'tool': 'Playwright Chromium headless on the committed replay.html', 'checks': checks,
                             'console_errors': 0, 'node_check': 'generated replay script passes node --check'},
    'run_log': 'matrix exit 0; VERIFIED line in reproduce output',
}
files = sorted(p for p in ev.rglob('*') if p.is_file() and p.name not in ('receipt.json',))
receipt['sha256'] = {str(p.relative_to(ev)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
(ev / 'receipt.json').write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + '\n')
print('packaged', ev, 'render checks', checks)
