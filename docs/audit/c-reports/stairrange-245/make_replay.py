import json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
CASES={
 'main':('The shrunk stair range stopped the munchkin climb',"A level-5 munchkin Warrior at depth 10 has a known up staircase five grids away. The base stair-search range is 8, but the \"increase the range\" adjustment adds (clevel - 10) + (10 - depth), which is negative below level 10: the range shrinks to 3, no staircase counts as close, and the borg never power-climbs toward the stairs it can see - it falls back to the generic any-stair flow. The correction gates the adjustment on clevel > 10, restoring the base range, and the climb decision fires."),
 'clevel11':('Control: level-11 Borg, range really grows','Munchkin mode only runs below maximum level 12, so level 11 is the one level above the gate. The adjustment adds (11-10)+(10-10)=1 (range 9) in both versions, and the staircase is 8 grids away: only the grown range counts it as close (8 < 9), so both versions power-climb.')}
for mode,(label,explanation) in CASES.items():
 item={'label':label,'explanation':explanation}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':('native-c/stairrange-245 finished: 1/1 passed' in raw) if mode=='main' else ('finished: 1/1 passed' in raw)}
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
 '<title>The shrunk stair range stopped the munchkin climb</title>'
 '<style>'+CSS+'</style><main>'
 '<p>Recorded native decision states</p>'
 '<h1>The shrunk stair range stopped the munchkin climb</h1>'
 '<p>The munchkin stair-range adjustment adds (clevel &#8722; 10) + (10 &#8722; depth) to the base range of 8. Below character level 10 the first term is negative, so the &#8220;increase&#8221; shrinks the range: a level-5 borg at depth 10 gets range 3, and its known staircase five grids away no longer counts as close. The correction gates the adjustment on clevel &gt; 10, restoring the base range below level 10.</p>'
 '<div class="cards">'
 '<section id="original" class="card"><h2>Original</h2><div class="num">'
 + str(d['before']['state'].get('goal_less')) +
 '</div><p class="state">climb decision (goal_less; 1 = power-climb toward the visible staircase)</p></section>'
 '<section id="compared" class="card"><h2>Corrected</h2><div class="num">'
 + str(d['after']['state'].get('goal_less')) +
 '</div><p class="state">climb decision (goal_less)</p></section>'
 '</div>'
 '<p class="summary">The original falls back to the generic any-stair flow (&#8220;Munchkin Mode. Any Stair.&#8221;); the corrected borg makes the targeted climb decision (&#8220;Munchkin Mode. Power Climb.&#8221;).</p>'
 '<details><summary>Experiment conditions and limits</summary><p>Native Human Warrior, seed 245, level 5, depth 10, no monsters, a known up staircase five grids north. The borg&#8217;s own tracked staircase list is seeded to match a viewed staircase; the production munchkin decision runs once and the goal state plus borg notes are read back directly. This isolates the range arithmetic and the climb decision; it does not measure autonomous play.</p></details>'
 '<h2>Other recorded cases</h2>'
 + ''.join('<section class="card" id="case-'+m+'"><h3>'+html_escape(data[m]['label'])+'</h3><p>'+html_escape(data[m]['explanation'])+'</p>'+'<p class="summary">'+('Control: the original and corrected logs are byte-identical.' if data[m]['before']['raw']==data[m]['after']['raw'] else 'Diagnostic: the logs differ (recorded, not asserted).')+'</p>'+'<details><summary>Original log</summary><pre>'+html_escape(data[m]['before']['raw'])+'</pre></details>'+'<details><summary>Corrected log</summary><pre>'+html_escape(data[m]['after']['raw'])+'</pre></details></section>' for m in data if m!='main') +
 '<details><summary>Recorded native output</summary><h3>Original</h3><pre id="original-log"></pre>'
 '<h3>Corrected</h3><pre id="compared-log"></pre></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/245">issue245</a>.</p>'
 '</main><script>'
 'document.getElementById("original-log").textContent='+json.dumps(d['before']['raw'])+';'
 'document.getElementById("compared-log").textContent='+json.dumps(d['after']['raw'])+';'
 '</script></html>')
(root/'replay.html').write_text(html)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
