import json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
CASES={
 'main':('Stair-travel gate under ailments',"A munchkin-start Warrior at depth 9 stands on a known up stair while poisoned and hallucinating. Stair travel does not age the world clock (level entry grants minimum move energy; the world tick is skipped during level generation), so shuttling between stairs can never let these timed ailments wear off. The original munchkin branch directs the Borg at the stair (stair_directed=1); the corrected branch hands the turn back to the ordinary recovery path (stair_directed=0). A clean control confirms munchkin play is unchanged."),
 'clean':('Control: healthy munchkin Borg','The same position and depth without poison or hallucination, run on its own. Munchkin mode keeps handling the turn and power-climbs; the correction does not change healthy munchkin play.')}
for mode,(label,explanation) in CASES.items():
 item={'label':label,'explanation':explanation}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':('native-c/staircycle-403 finished: 2/2 passed' in raw) if mode=='main' else ('finished: 1/1 passed' in raw)}
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
 '<title>The poisoned, hallucinating Borg stair-cycled</title>'
 '<style>'+CSS+'</style><main>'
 '<p>Recorded native decision states</p>'
 '<h1>The poisoned, hallucinating Borg stair-cycled</h1>'
 '<p>In munchkin start mode, a poisoned and hallucinating Borg on a known stair kept choosing stair travel. Stair trips do not age the world clock, so the timed ailments could never wear off. The correction makes the munchkin branch step aside while such ailments are active, handing the turn to the ordinary recovery path, which ages turns normally.</p>'
 '<div class="cards">'
 '<section id="original" class="card"><h2>Original</h2><div class="num">'
 + str(d['before']['outcome'].get('stair_directed')) +
 '</div><p class="state">stair_directed while ailing (1 = the Borg chose stair travel)</p></section>'
 '<section id="compared" class="card"><h2>Corrected</h2><div class="num">'
 + str(d['after']['outcome'].get('stair_directed')) +
 '</div><p class="state">stair_directed while ailing (0 = handed back to recovery)</p></section>'
 '</div>'
 '<p class="summary">Control: without the ailments, munchkin mode still handles the turn (munchkin_handled=1 in both versions) &mdash; the correction changes only the ailed case.</p>'
 '<h2>What the recorded run establishes</h2>'
 '<p>Two native scenarios run per version: the ailed case (poisoned and hallucinating on a known up stair at depth 9) and the clean control. The original stair-cycles in the ailed case; the corrected Borg does not, and the clean control still power-climbs. Queued keys and Borg notes are recorded for both.</p>'
 '<details><summary>Experiment conditions and limits</summary><p>Native Human Warrior, seed 1045, depth 9 (past the munchkin depth target, engaging the Power Climb branch), one known up stair underfoot and a down stair three tiles east, no monsters. The ailments enter through the real timed-effect API and traits derive through the production notice path. The fixture isolates this decision stage; it does not replay the consumer zoo run that first stalled there, and it makes no survival or encounter-frequency claim.</p></details>'
 '<h2>Other recorded cases</h2>'
 + ''.join('<section class="card" id="case-'+m+'"><h3>'+html_escape(data[m]['label'])+'</h3><p>'+html_escape(data[m]['explanation'])+'</p>'+'<p class="summary">'+('Control: the original and corrected logs are byte-identical.' if data[m]['before']['raw']==data[m]['after']['raw'] else 'Diagnostic: the logs differ (recorded, not asserted).')+'</p>'+'<details><summary>Original log</summary><pre>'+html_escape(data[m]['before']['raw'])+'</pre></details>'+'<details><summary>Corrected log</summary><pre>'+html_escape(data[m]['after']['raw'])+'</pre></details></section>' for m in data if m!='main') +
 '<details><summary>Recorded native output</summary><h3>Original</h3><pre id="original-log"></pre>'
 '<h3>Corrected</h3><pre id="compared-log"></pre></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/403">issue403</a>.</p>'
 '</main><script>'
 'document.getElementById("original-log").textContent='+json.dumps(d['before']['raw'])+';'
 'document.getElementById("compared-log").textContent='+json.dumps(d['after']['raw'])+';'
 '</script></html>')
(root/'replay.html').write_text(html)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
