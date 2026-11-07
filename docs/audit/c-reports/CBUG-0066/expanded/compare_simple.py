"""Compare the tested selection conditions on the same native test cases.

Called by reproduce.py --compare-simple; only its disposable source is edited.
The verified corrected source and binary are restored before this exits.

The disposable tree arrives at the corrected (sentinel) source. All comparison
variants are derived from the original base source by inverting the accepted
fix, so every variant is measured in this one experiment:

- original comparison  (the shipped condition; the before-phase baseline)
- simple reversal      (b_p2 <= b_p1, no guards; the historical 7/9 result)
- guarded fix          (availability guards plus lower danger; 9/9)
- sentinel fix         (9999 sentinels, no early return, plain pick; 9/9)
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root=Path(sys.argv[1]).resolve()
source,build,evidence=root/'source',root/'build',root/'evidence'
path=source/'src/borg/borg-item-wear.c'
corrected=path.read_bytes()
sentinel_sha='3a9219e11f4de32a070652a326ffc67def6100d094d7eff78939879922c2f25b'
original_sha='a7f1427419a3f2b3c7810aa7761d9c31d13e632633ccdb6f86998c1d4ad02bd9'
guarded_sha='96dd28c786e73ddf211a94f1d73b288225c182caa5ca68cdcd746c33d96dd40e'
if hashlib.sha256(corrected).hexdigest()!=sentinel_sha:
    raise SystemExit('The disposable source is not the verified corrected version')


def run(label,args):
    log=root/(label+'.log')
    print(f'{label}: waiting for shared validation lock; log {log}',flush=True)
    with log.open('w') as out:
        r=subprocess.run(['flock','/tmp/angband-validation.lock',*args],cwd=build/'game',stdout=out,stderr=subprocess.STDOUT)
    if r.returncode: raise RuntimeError(f'{label} failed; see {log}')


def cases_for(label):
    run(label+'-cases',[sys.executable,str(evidence/'run-expanded.py'),label,str(build/'game/CBUG0066ExpandedObserver')])
    return json.loads((evidence/f'{label}-expanded.json').read_text())['cases']


def passes(cases): return {name for name,case in cases.items() if case['passed']}

# Invert the accepted fix to recover the original base source, pinned by digest.
original=corrected.decode()
for old,new in (
    ('    int32_t b_p1 = 9999L;\n    int32_t b_p2 = 9999L;','    int32_t b_p1 = 0L;\n    int32_t b_p2 = 0L;'),
    ('\n\n    if (armour_swap) {',
     '\n    /* make sure we have an appropriate swap */\n    if (!armour_swap && !weapon_swap)\n        return false;\n\n    if (armour_swap) {'),
    ('if (b_p2 <= b_p1) {','if (b_p1 <= b_p2) {'),
):
    assert original.count(old)==1,old
    original=original.replace(old,new,1)
if hashlib.sha256(original.encode()).hexdigest()!=original_sha:
    raise RuntimeError('Recovered original source does not match the pinned base digest')

guarded=original.replace('if (b_p1 <= b_p2) {','if (weapon_swap && (!armour_swap || b_p2 <= b_p1)) {',1)
if hashlib.sha256(guarded.encode()).hexdigest()!=guarded_sha:
    raise RuntimeError('Derived guarded source does not match the previously validated digest')

try:
    # Variant A: the plain reversal without availability guards. The missing
    # item's uncomputed score stays 0, so each single-backup case selects it.
    path.write_text(original.replace('if (b_p1 <= b_p2) {','if (b_p2 <= b_p1) {',1))
    run('simple-reversal-build',['cmake','--build',str(build),'--target','CBUG0066ExpandedObserver','--parallel','2'])
    cases=cases_for('simple-reversal')
    failed={name for name,case in cases.items() if not case['passed']}
    if failed!={'armour-only','weapon-only'}:
        raise RuntimeError(f'Unexpected simple-reversal failures: {sorted(failed)}')
    if not all(case['completed'] and case['exit']==0 for case in cases.values()):
        raise RuntimeError('A comparison case crashed or did not finish')
    for name in failed:
        output=(evidence/f'simple-reversal-{name}.log').read_text()
        if 'pack_index=-1' not in output:
            raise RuntimeError(f'Expected absent-item selection missing: {name}')
    result={'comparison':'if (b_p2 <= b_p1)', 'passes':7,'failures':2,
            'failed_cases':sorted(failed),'failure_reason':'Both single-backup cases select pack_index=-1, an absent item.',
            'cases':{name:case['passed'] for name,case in cases.items()}}
    (root/'simple-reversal-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('VERIFIED: simple reversal passes 7/9; armour-only and weapon-only fail with pack_index=-1.',flush=True)

    # Variant B: the guarded fix, previously the accepted proposal shape.
    path.write_text(guarded)
    run('guarded-build',['cmake','--build',str(build),'--target','CBUG0066ExpandedObserver','--parallel','2'])
    cases=cases_for('guarded')
    if len(passes(cases))!=9:
        raise RuntimeError(f'Guarded fix must pass all nine cases; failed: {sorted(set(cases)-passes(cases))}')
    result={'comparison':'if (weapon_swap && (!armour_swap || b_p2 <= b_p1)) {','passes':9,'failures':0,
            'failed_cases':[],'source_sha256':guarded_sha,
            'cases':{name:case['passed'] for name,case in cases.items()}}
    (root/'guarded-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('VERIFIED: guarded fix passes 9/9.',flush=True)

    # Variant C: the sentinel fix, the accepted proposal shape (the corrected
    # source itself, rebuilt and rerun here so every variant has fresh logs).
    path.write_bytes(corrected)
    run('sentinel-shape-build',['cmake','--build',str(build),'--target','CBUG0066ExpandedObserver','--parallel','2'])
    cases=cases_for('sentinel-shape')
    if len(passes(cases))!=9:
        raise RuntimeError(f'Sentinel fix must pass all nine cases; failed: {sorted(set(cases)-passes(cases))}')
    if not all(case['completed'] and case['exit']==0 for case in cases.values()):
        raise RuntimeError('A sentinel case crashed or did not finish')
    identical=[name for name in cases
               if (evidence/f'sentinel-shape-{name}.log').read_bytes()==(evidence/f'guarded-{name}.log').read_bytes()]
    if len(identical)!=9:
        raise RuntimeError(f'Sentinel output differs from the guarded run in: {sorted(set(cases)-set(identical))}')
    result={'comparison':'if (b_p2 <= b_p1) with b_p1/b_p2 initialized to 9999 and no early return',
            'passes':9,'failures':0,'failed_cases':[],'source_sha256':sentinel_sha,
            'byte_identical_to_guarded':sorted(identical),
            'cases':{name:case['passed'] for name,case in cases.items()}}
    (root/'sentinel-shape-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('VERIFIED: sentinel fix passes 9/9 and every native log is byte-identical to the guarded run.',flush=True)
finally:
    path.write_bytes(corrected)
    run('restore-corrected-build',['cmake','--build',str(build),'--target','CBUG0066ExpandedObserver','--parallel','2'])
