import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
SUITE='native-c/backaway-155'
MODES=['monster', 'trap', 'healthy']
rows={}
for mode in MODES:
 env=os.environ|{'BA155'+'_PROFILE':str(root/'profile'),'BA155'+'_MODE':mode}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=300,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':SUITE+' finished: 1/1 passed' in raw}
 for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
  for match in re.findall(re.escape(tag)+r'(.*)',raw):
   row.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
 rows[mode]=row
 print(mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
