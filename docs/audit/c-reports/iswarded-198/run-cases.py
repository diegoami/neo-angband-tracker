import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
SUITE='native-c/iswarded-198'
MODES=['rune-town', 'hit-town', 'rune-inside', 'hit-inside', 'hit-noglyph']
rows={}
for mode in MODES:
 env=os.environ|{'ISW198'+'_PROFILE':str(root/'profile'),'ISW198'+'_MODE':mode}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=300,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':SUITE+' finished: 1/1 passed' in raw}
 for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
  for match in re.findall(re.escape(tag)+r'(.*)',raw):
   row.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
 row['assert_abort']="Assertion `square_in_bounds(c, grid)' failed" in raw
 row['asan']='AddressSanitizer' in raw
 row['asan_read']=re.search(r'ERROR: AddressSanitizer: heap-buffer-overflow.*\nREAD of size',raw) is not None
 site=re.search(r'#0 0x[0-9a-f]+ in (\w+) .*\n.*\n.*\n.*#3 0x[0-9a-f]+ in borg_parse_aux \S*?(borg-messages\.c:\d+)',raw)
 row['asan_site']=(site.group(1)+' <- borg_parse_aux '+site.group(2)) if site else None
 rows[mode]=row
 print(mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
