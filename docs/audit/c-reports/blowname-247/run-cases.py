import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,asan,directory=sys.argv[1:];root=Path(directory)
def run_case(binary_path, tag, mode=None):
 env=os.environ|{'BW247_PROFILE':str(root/'profile')}
 if mode:env=env|{'BW247_MODE':mode}
 result=subprocess.run([str(Path(binary_path).resolve()),"-v"],env=env,cwd=Path(binary_path).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout)
 case=mode or 'main'
 name=f'{phase}-{tag}-{case}.log' if tag else f'{phase}-{case}.log'
 (root/name).write_text(raw)
 row={'exit':result.returncode,'passed':'native-c/blowname-247 finished: 1/1 passed' in raw,
      'signal':'segfault' if ('segmentation fault' in (root/name).read_text().lower() or result.returncode in (-11,139)) else 'clean',
      'asan_report':'AddressSanitizer' in raw}
 for t,key in (('KNOWN ','known'),('UNKNOWN ','unknown'),('LAST ','last')):
  m=re.search(re.escape(t)+r'(.*)',raw)
  if m:row[key]={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',m[1])}
 return row,raw
rows={}
row,raw=run_case(binary,None);rows['main']=row;print('main',row)
row,raw=run_case(asan,'asan');rows['asan']=row
m=re.search(r'AddressSanitizer:SB?[^\\n]*',raw)
rows['asan']['summary']=('sanitizer crash: '+m[0][:80]) if m else ('no sanitizer report' if row['passed'] else 'failed without sanitizer report')
print('asan',rows['asan'])
# Control: in-table names only (HURT and the last real entry BLACK_BREATH).
row,raw=run_case(binary,None,'known');rows['known']=row;print('known',row)
row,raw=run_case(asan,'asan','known');rows['asan-known']=row;print('asan-known',row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
