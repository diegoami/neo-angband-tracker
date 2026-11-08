import json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
CASES={
 'main':('The drained Borg power-dived anyway',"A drained Borg (experience below its maximum) at depth 15 is otherwise fully prepared: borg_prepared(15) and borg_prepared(20) are NULL, nothing has chosen a direction, and the power-dive check borg_prepared(CDEPTH + 5) is satisfied. The \"do not dive when drained\" guard read if (g && BI_ISFIXEXP), but g is only non-zero there when an earlier branch already decided to head up - so for a Borg that had not yet chosen a direction the guard was dead and the drained Borg took the dive. The correction applies the guard unconditionally, matching the sibling \"full of items\" check and the original dive-cancellation shape."),
 'not-drained':('Control: not drained','The same fully prepared Borg at depth 15 with full experience (exp equals max_exp). Both versions take the power dive ("# power dive, playing too shallow.").')}
for mode,(label,explanation) in CASES.items():
 item={'label':label,'explanation':explanation}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':('native-c/drainedive-235 finished: 1/1 passed' in raw) if mode=='main' else ('finished: 1/1 passed' in raw)}
  for tag,key in (('OUTCOME ','outcome'),):
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
 '<title>The drained Borg power-dived anyway</title>'
 '<style>'+CSS+'</style><main>'
 '<p>Recorded native decision states</p>'
 '<h1>The drained Borg power-dived anyway</h1>'
 '<p>The &#8220;do not dive when drained&#8221; guard was gated on g, the direction chosen before the check: g is only non-zero when an earlier branch already decided to head up. For a drained Borg that had not yet chosen a direction the guard was dead, and the power-dive branch (&#8220;playing too shallow&#8221;) fired anyway. The correction applies the drained guard unconditionally, like the sibling &#8220;full of items&#8221; guard.</p>'
 '<div class="cards">'
 '<section id="original" class="card"><h2>Original</h2><div class="num">'
 + str(d['before']['outcome'].get('dive_notes')) +
 '</div><p class="state">power-dive notes for the drained, prepared Borg (1 = dove)</p></section>'
 '<section id="compared" class="card"><h2>Corrected</h2><div class="num">'
 + str(d['after']['outcome'].get('dive_notes')) +
 '</div><p class="state">power-dive notes for the drained, prepared Borg</p></section>'
 '</div>'
 '<p class="summary">Same fixture state on both sides (See Invisible and Free Action worn, exp drained, clevel 49): the original records &#8220;# power dive, playing too shallow.&#8221;; the corrected Borg does not dive while drained.</p>'
 '<details><summary>Experiment conditions and limits</summary><p>Native Human Warrior, seed 235, clevel 49, depth 15, no monsters. Restock and preparedness satisfied through depth 20 (fueled lantern, food, phase and teleportation scrolls, worn See Invisible and Free Action rings), static preparedness path (BORG_USES_DYNAMIC_CALCS off), exp 0 against max_exp 10000 so BI_ISFIXEXP derives true. borg_leave_level(false) runs once and the borg notes are read back directly. This isolates the drained guard and the leave-level dive decision; it does not measure autonomous play.</p></details>'
 '<h2>Other recorded cases</h2>'
 + ''.join('<section class="card" id="case-'+m+'"><h3>'+html_escape(data[m]['label'])+'</h3><p>'+html_escape(data[m]['explanation'])+'</p>'+'<p class="summary">'+('Control: the original and corrected logs are byte-identical.' if data[m]['before']['raw']==data[m]['after']['raw'] else 'Diagnostic: the logs differ (recorded, not asserted).')+'</p>'+'<details><summary>Original log</summary><pre>'+html_escape(data[m]['before']['raw'])+'</pre></details>'+'<details><summary>Corrected log</summary><pre>'+html_escape(data[m]['after']['raw'])+'</pre></details></section>' for m in data if m!='main') +
 '<details><summary>Recorded native output</summary><h3>Original</h3><pre id="original-log"></pre>'
 '<h3>Corrected</h3><pre id="compared-log"></pre></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/235">issue235</a>.</p>'
 '</main><script>'
 'document.getElementById("original-log").textContent='+json.dumps(d['before']['raw'])+';'
 'document.getElementById("compared-log").textContent='+json.dumps(d['after']['raw'])+';'
 '</script></html>')
(root/'replay.html').write_text(html)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
