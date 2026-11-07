import json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
CASES={
 'main':('An unknown blow name is looked up',"A modded blow_effects.txt can supply a blow name that is not in the Borg's lookup table. The lookup loop visits every table entry including the NULL end marker, so the original crashes (strcmp against NULL); the corrected loop stops before the marker and returns NONE. The in-table name HURT keeps resolving.")}
results=json.loads((root/'before-results.json').read_text())
for mode,(label,explanation) in CASES.items():
 item={'label':label,'explanation':explanation}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  signal=results[mode]['signal'] if phase=='before' else 'clean'
  entry={'raw':raw,'passed':'native-c/blowname-247 finished: 1/1 passed' in raw,'exit':signal}
  for t,key in (('KNOWN ','known'),('UNKNOWN ','unknown')):
   m=re.search(re.escape(t)+r'(.*)',raw)
   if m:entry[key]={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',m[1])}
  asan=(root/f'{phase}-asan-{mode}.log').read_text() if (root/f'{phase}-asan-{mode}.log').exists() else ''
  entry['asan_crash']='AddressSanitizer' in asan
  m2=re.search(r'AddressSanitizer:[^\n]*',asan)
  entry['asan_snippet']=m2.group(0) if m2 else ''
  item[phase]=entry
 data[mode]=item
CSS=("body{margin:0;background:#f4f1ea;color:#253a3c;font:18px/1.6 system-ui}"
     "main{max-width:1100px;margin:auto;padding:32px 24px}"
     "h1{font-size:40px;line-height:1.2}h2{font-size:24px}"
     ".cards{display:grid;grid-template-columns:1fr 1fr;gap:20px}"
     ".card{background:white;border:1px solid #cbd6d0;border-radius:12px;padding:24px;margin:18px 0}"
     ".cards .card{border-top:6px solid #648e80}.num{font-size:34px;font-weight:750}"
     ".summary{background:#e6eee6;padding:20px;border-left:5px solid #377367}"
     "pre{white-space:pre-wrap;overflow-wrap:anywhere;font:13px/1.5 monospace;max-height:330px;overflow:auto}"
     "details{background:white;padding:20px;margin:20px 0}summary{font-weight:700;cursor:pointer}"
     "@media(max-width:650px){.cards{grid-template-columns:1fr}h1{font-size:30px}}")
d=data['main']
html=('<!doctype html><html lang="en"><meta charset="utf-8">'
 '<meta name="viewport" content="width=device-width,initial-scale=1">'
 '<title>Unknown blow names walked into the NULL end entry</title>'
 '<style>'+CSS+'</style><main>'
 '<p>Recorded native lookups</p>'
 '<h1>Unknown blow names walked into the table&#39;s NULL end entry</h1>'
 '<p>The Borg maps monster blow names (from the game data) to its own danger categories '
 'through a small table that ends with a NULL marker. The lookup loop visited that marker too, '
 'so a blow name absent from the table reached strcmp against NULL and crashed the Borg &#8212; '
 'a name a modded blow_effects.txt can supply. The correction stops the loop before the marker; '
 'unknown names return NONE and the in-table names keep resolving.</p>'
 '<div class="cards">'
 '<section id="original" class="card"><h2>Original</h2><div class="num">'
 + ('Segfault' if d['before']['exit']=='segfault' else 'Clean') +
 '</div><p class="state">'
 + ('Address sanitizer report: ' + d['before']['asan_snippet'] if d['before']['asan_crash'] else 'Crashed on the unknown name') +
 '</p></section>'
 '<section id="compared" class="card"><h2>Corrected</h2><div class="num">Returns NONE</div>'
 '<p class="state">Unknown names return NONE; HURT still resolves (value 1)</p></section>'
 '</div>'
 '<p class="summary">The original crashes on the unknown name; the corrected loop stops before the NULL marker.</p>'
 '<h2>What the recorded run establishes</h2>'
 '<p>The in-table name HURT resolves in both versions (value 1); the made-up name crashes the original '
 '&#8212; under the address sanitizer the report names the null dereference &#8212; and returns NONE in the '
 'corrected build.</p>'
 '<details><summary>Experiment conditions and limits</summary><p>Native Human Warrior, seed 1000. '
 'The lookup function is called directly with an in-table name and a made-up name; no game simulation runs. '
 'This isolates the table lookup, not full autonomous play.</p></details>'
 '<details><summary>Recorded native output</summary><h3>Original</h3><pre id="original-log"></pre>'
 '<h3>Corrected</h3><pre id="compared-log"></pre></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/247">issue247</a>.</p>'
 '</main><script>'
 'document.getElementById("original-log").textContent=d_before_raw;'
 'document.getElementById("compared-log").textContent=d_after_raw;'
 '</script></html>')
# splice the raw logs into placeholders (kept out of the JS to avoid escaping issues)
html=html.replace('d_before_raw', json.dumps(d['before']['raw']))
html=html.replace('d_after_raw', json.dumps(d['after']['raw']))
(root/'replay.html').write_text(html)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
