import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
rows={}
MODES=('main', 'clean')
for mode in MODES:
 env=os.environ|{'STAIR403_PROFILE':str(root/'profile'),'STAIR403_MODE':mode}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=300,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':('native-c/staircycle-403 finished: 2/2 passed' in raw) if mode=='main' else ('finished: 1/1 passed' in raw),
      'ailing_failed_as_expected':'native-c/staircycle-403 finished: 1/2 passed' in raw and 'stair_directed=1' in raw}
 for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
  for match in re.findall(re.escape(tag)+r'(.*)',raw):
   row.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
 rows[mode]=row
 print(mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
