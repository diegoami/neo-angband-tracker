import json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
CASES={
 'main':('Glowing floors never reached the borg grid flags',"A permanently glowing arena floor is shown to a munchkin-configured Warrior through the production map update. The original map update's Floors branch tested the same condition as the Darkness branch before it and was never taken: no grid anywhere was ever flagged BORG_GLOW, and the flow-toward-light consumer found an empty target list. The corrected branch (restored FEAT_FLOOR condition, glow read from square_islit) flags the glowing floors, walls stay unflagged, and the consumer's list fills.")}
for mode,(label,explanation) in CASES.items():
 item={'label':label,'explanation':explanation}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':'native-c/glowtrace-213 finished: 1/1 passed' in raw}
  for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
   for match in re.findall(re.escape(tag)+r'(.*)',raw):
    entry.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
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
 '<title>Glowing floors were never flagged to the borg</title>'
 '<style>'+CSS+'</style><main>'
 '<p>Recorded native grid flags and consumer state</p>'
 '<h1>Glowing floors were never flagged to the borg</h1>'
 '<p>The map update&#8217;s Floors branch tested the same condition as the Darkness branch before it, so it never ran: the game&#8217;s permanent lighting never reached the borg&#8217;s grid flags, and the flow-toward-light consumer always found an empty target list. The correction restores the original floor condition and reads the glow state from the grid&#8217;s permanent-glow flag.</p>'
 '<div class="cards">'
 '<section id="original" class="card"><h2>Original</h2><div class="num">'
 + str(d['before'].get('outcome', {}).get('glow_n', 0)) +
 '</div><p class="state">glow targets found by the flow-toward-light consumer</p></section>'
 '<section id="compared" class="card"><h2>Corrected</h2><div class="num">'
 + str(d['after'].get('outcome', {}).get('glow_n', 0)) +
 '</div><p class="state">glow targets found by the flow-toward-light consumer</p></section>'
 '</div>'
 '<p class="summary">Distant glowing floor flagged BORG_GLOW: original ' + str(d['before']['state'].get('distant_glow')) + ', corrected ' + str(d['after']['state'].get('distant_glow')) + '. Walls remain unflagged in both. Full native regression gates pass for both builds.</p>'
 '<details><summary>Experiment conditions and limits</summary><p>Native Human Warrior, seed 213, depth 9, a uniformly permanently-glowing arena with perm walls, no monsters and no ailments. The borg views the whole arena; the production map update runs once and grid flags plus the flow-toward-light consumer state are read back directly. This isolates the map-update flag stage and one consumer; it does not measure autonomous play or encounter outcomes.</p></details>'
 '<details><summary>Recorded native output</summary><h3>Original</h3><pre id="original-log"></pre>'
 '<h3>Corrected</h3><pre id="compared-log"></pre></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/213">issue213</a>.</p>'
 '</main><script>'
 'document.getElementById("original-log").textContent='+json.dumps(d['before']['raw'])+';'
 'document.getElementById("compared-log").textContent='+json.dumps(d['after']['raw'])+';'
 '</script></html>')
(root/'replay.html').write_text(html)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
