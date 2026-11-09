import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
SUITE='native-c/panelrows-348'
MODES=['light-tall', 'light-wide', 'light-normal', 'light-fit', 'vault-tall', 'vault-wide', 'vault-normal', 'dig-tall', 'dig-wide', 'dig-normal']
rows={}
for mode in MODES:
 env=os.environ|{'PR348'+'_PROFILE':str(root/'profile'),'PR348'+'_MODE':mode}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=300,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':SUITE+' finished: 1/1 passed' in raw}
 for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
  for match in re.findall(re.escape(tag)+r'(.*)',raw):
   row.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
 row['asan']='AddressSanitizer' in raw
 site=re.search(r'ERROR: AddressSanitizer: ([\w-]+) on address.*\n(READ|WRITE) of size \d+.*\n\s*#0 0x[0-9a-f]+ in (\w+) \S*?/([\w-]+\.c:\d+)',raw)
 row['asan_site']=f'{site.group(1)} {site.group(2)} in {site.group(3)} {site.group(4)}' if site else None
 rows[mode]=row
 print(mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
