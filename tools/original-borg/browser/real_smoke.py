#!/usr/bin/env python3
"""Optional read-only real-evidence inspection, separate from fixture tests."""
import argparse
import json
from server import Store
p=argparse.ArgumentParser();p.add_argument('--cache',required=True);a=p.parse_args()
s=Store([a.cache]);receipts=[]
for seed,needle in [('1261438670','shop-exit'),('1460096338','cast')]:
    rows=s.catalogue({'source_profile':'neo-original','seed':seed})['rows']
    if not rows:
        receipts.append({'seed':seed,'status':'not downloaded/pending'});continue
    r=rows[0];life=s.life(r['id']);commands=s.page(r['id'],{'file':'run/commands.jsonl','tail':'1','limit':'100'})
    assert any(v['value'].get('intent',{}).get('command',{}).get('code')==needle for v in commands['rows'])
    assert life['record']['outcome']=='stalled'
    if needle=='shop-exit':assert any(v['value'].get('accepted') is False for v in commands['rows'])
    assert s.page(r['id'],{'file':'run/messages.jsonl','tail':'1'})['rows']
    assert s.page(r['id'],{'file':'run/states.jsonl','tail':'1'})['rows']
    receipts.append({'seed':seed,'outcome':'stalled','recorded_command':needle,'raw_artifact':life['raw_artifact'],'status':'pass'})
rows=s.catalogue({'source_profile':'better-borg-c','outcome':'death','limit':'1'})['rows']
assert rows
r=rows[0];life=s.life(r['id']);states=s.page(r['id'],{'file':'trace.txt','mode':'states','tail':'1'})
assert any(int(v['value'].get('hp','0'))<0 for v in states['rows'])
assert any(f['name'].endswith('.map') for f in life['files'])
receipts.append({'seed':r['seed'],'source':'native C','outcome':'death','state_rows':len(states['rows']),'status':'pass'})
print(json.dumps({'schema':'borg-record-browser-real-smoke@1','cached_lives':s.catalogue({})['cached_total'],'observations':receipts},indent=2))
