import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,asan_binary,directory=sys.argv[1:];root=Path(directory)
cases=('multibyte','control','wide')
def run(path,mode,extra):
 env=os.environ|{'T287_MODE':mode,'T287_PROFILE':str(root/'profile')}|extra
 result=subprocess.run([str(Path(path).resolve()),"-v"],env=env,cwd=Path(path).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 return result.returncode,re.sub(r'\x1b\[[0-9;]*m','',result.stdout)
def fields(row,raw):
 for tag in ('GEOMETRY ','RATING ','OUTCOME '):
  match=re.search(re.escape(tag)+r'(.*)',raw)
  if match:row[tag.split()[0].lower()]=match[1].strip()
 return row
rows={}
for mode in cases:
 code,raw=run(binary,mode,{});(root/f'{phase}-{mode}.log').write_text(raw)
 rows[mode]=fields({'exit':code,'passed':'native-c/text-287 finished: 1/1 passed' in raw},raw)
 print(mode,rows[mode])
# The same cases under AddressSanitizer (exit code tolerated; a report aborts the run).
rows['asan']={}
for mode in cases:
 code,raw=run(asan_binary,mode,{'ASAN_OPTIONS':'detect_leaks=1'});(root/f'{phase}-asan-{mode}.log').write_text(raw)
 report=re.search(r'ERROR: AddressSanitizer: (\S+)',raw)
 access=re.search(r'(READ|WRITE) of size (\d+)',raw)
 row=fields({'exit':code,'passed':'native-c/text-287 finished: 1/1 passed' in raw,
  'sanitizer_report':report[1] if report else None,
  'access':f'{access[1]} of size {access[2]}' if access else None,
  'frames':[f for f in ('wcstombs','borg_what_text','wide_case') if re.search(r'#\d+ 0x[0-9a-f]+ in \S*'+f+r'\b',raw)]},raw)
 rows['asan'][mode]=row
 print('asan',mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
