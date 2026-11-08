import json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
CASES={
 'main':('The clock-overflow abort fired before the graceful reset could',"The borg clock sits exactly on the abort threshold (BORG_TIME_MAX - 3000). The original deactivates the borg there (borg_oops, clock overflow), about 2900 turns before the graceful reset threshold in borg_think can ever fire, so the borg stops and the clock stays frozen. The correction resets the clock in place and keeps playing; the borg_think overflow check remains as a failsafe."),
 'below-threshold':('Control: one turn below the threshold','The clock at BORG_TIME_MAX - 3001. Neither the original abort nor the corrected reset applies; the Borg keeps its clock and stays active in both versions.')}
for mode,(label,explanation) in CASES.items():
 item={'label':label,'explanation':explanation}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':('native-c/clockreset-246 finished: 1/1 passed' in raw) if mode=='main' else ('finished: 1/1 passed' in raw)}
  for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
   for match in re.findall(re.escape(tag)+r'(.*)',raw):
    entry.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
  item[phase]=entry
 data[mode]=item
def html_escape(s):
 return str(s).replace('&','&amp;').replace('<','&lt;').replace('>','&gt;')
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
 '<title>The clock-overflow abort fired before the graceful reset</title>'
 '<style>'+CSS+'</style><main>'
 '<p>Recorded native borg state at the overflow threshold</p>'
 '<h1>The clock-overflow abort fired before the graceful reset</h1>'
 '<p>The borg clock sits exactly on the abort threshold (BORG_TIME_MAX &#8722; 3000). The original deactivates the borg there &#8212; about 2900 turns before the graceful reset in borg_think can ever fire &#8212; so the borg stops for good and the clock stays frozen. The correction resets the clock in place and keeps playing; the overflow check in borg_think remains as a failsafe.</p>'
 '<div class="cards">'
 '<section id="original" class="card"><h2>Original</h2><div class="num">'
 + str(d['before']['state'].get('active')) +
 '</div><p class="state">borg still active after the threshold pass (0 = aborted, clock frozen)</p></section>'
 '<section id="compared" class="card"><h2>Corrected</h2><div class="num">'
 + str(d['after']['state'].get('active')) +
 '</div><p class="state">borg still active after the threshold pass (clock reset to 1)</p></section>'
 '</div>'
 '<p class="summary">Original: clock frozen at the threshold, borg aborted. Corrected: clock reset to 1, borg keeps playing.</p>'
 '<details><summary>Experiment conditions and limits</summary><p>Native Human Warrior, seed 246, depth 9, a glowing arena with no monsters. The borg clock is set to the exact abort threshold and one production think_dungeon pass runs; the clock value, the active flag and the borg notes are read back directly. This isolates the overflow handling; it does not measure autonomous play.</p></details>'
 '<h2>Other recorded cases</h2>'
 + ''.join('<section class="card" id="case-'+m+'"><h3>'+html_escape(data[m]['label'])+'</h3><p>'+html_escape(data[m]['explanation'])+'</p>'+'<p class="summary">'+('Control: the original and corrected logs are byte-identical.' if data[m]['before']['raw']==data[m]['after']['raw'] else 'Diagnostic: the logs differ (recorded, not asserted).')+'</p>'+'<details><summary>Original log</summary><pre>'+html_escape(data[m]['before']['raw'])+'</pre></details>'+'<details><summary>Corrected log</summary><pre>'+html_escape(data[m]['after']['raw'])+'</pre></details></section>' for m in data if m!='main') +
 '<details><summary>Recorded native output</summary><h3>Original</h3><pre id="original-log"></pre>'
 '<h3>Corrected</h3><pre id="compared-log"></pre></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/246">issue246</a>.</p>'
 '</main><script>'
 'document.getElementById("original-log").textContent='+json.dumps(d['before']['raw'])+';'
 'document.getElementById("compared-log").textContent='+json.dumps(d['after']['raw'])+';'
 '</script></html>')
(root/'replay.html').write_text(html)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
