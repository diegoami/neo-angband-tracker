import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/takestair-71'
TITLE='A far staircase put an item next to the Borg off its leash'
INTRO='A low-level Borg keeps item hunts within a leash of its escape staircase. borg_flow_take(), borg_flow_take_scum() and borg_flow_take_lunal() each look up the nearest known up staircase for that test, but the scan kept the farthest one. When the Borg knew a second up staircase far away, a Flask of Oil three steps from the Borg (five from the staircase it was standing by) was measured against the far staircase, judged too far from the stairs, and ignored by all three pickup flows. The correction keeps the nearest up staircase, and the Borg steps toward the flask.'
CONDITIONS="Native Human Warrior, seed 71, character level 1, depth 1 (pickup and explore leash both 12), starting kit plus a lit torch, a lit 70x22 arena. An up staircase two steps west of the Borg; in the main cases a second up staircase 55 steps east, both tracked by the Borg. A real Flask of Oil lies on the floor three steps east and is registered through the production observe_take_diff(). Fixture interventions: the Borg's map is copied from the game map and marked viewed, the staircases are written to track_less, and borg.panels is set to 1x1 (what borg_update() derives for a one-panel arena) before the detection arrays are allocated. The production pickup flow runs once and the first queued key is read back. This isolates the pickup decision; it does not play out the walk or the pickup."
ISSUE=71
CARD_KEY='goes_for_item'
CARD_LABEL='steps toward the flask (1 = key 6, east)'
CASES=[('take', 'borg_flow_take(true, 250): up staircases 2 and 55 steps away', "Original: the staircase 55 steps away is the reference, the flask is 52 flow steps from it (over the 12-step leash) and is skipped; the flow returns false. Corrected: the staircase two steps away is the reference (flask 5 steps from it); 'Flowing toward item at cost 3', key 6 (east).", False), ('scum', 'borg_flow_take_scum(true, 6): same layout', "Original: borg_flow_far_from_stairs() measures from the far staircase and the flask is skipped. Corrected: 'Flowing toward Scum item at cost 3', key 6.", False), ('lunal', 'borg_flow_take_lunal(true, 4): same layout', "Original: the flask is skipped as too far from the (far) staircase. Corrected: 'Flowing toward munchkin item at cost 3', key 6.", False), ('control', 'Control: only the near up staircase (borg_flow_take)', 'With a single staircase nearest and farthest are the same; both versions step toward the flask and the logs are byte-identical.', True)]
for mode,label,explanation,control in CASES:
 item={'label':label,'explanation':explanation,'control':control}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':SUITE+' finished: 1/1 passed' in raw}
  for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
   for match in re.findall(re.escape(tag)+r'(.*)',raw):
    entry.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
  notes=[l for l in raw.splitlines() if l.startswith(('PREPARED ','SWAP ','DECISION ','TRACK ','KEYS '))]
  entry['notes']=notes
  entry['value']=entry.get('outcome',{}).get(CARD_KEY,entry.get('state',{}).get(CARD_KEY,'n/a'))
  item[phase]=entry
 data[mode]=item
CSS=("body{margin:0;background:#f4f1ea;color:#253a3c;font:18px/1.6 system-ui}"
 "main{max-width:1100px;margin:auto;padding:32px 24px}"
 "h1{font-size:38px;line-height:1.2}h2{font-size:24px}"
 ".cards{display:grid;grid-template-columns:1fr 1fr;gap:20px}"
 ".card{background:white;border:1px solid #cbd6d0;border-radius:12px;padding:24px;margin:18px 0}"
 ".cards .card{border-top:6px solid #648e80}.num{font-size:34px;font-weight:750}"
 ".summary{background:#e6eee6;padding:20px;border-left:5px solid #377367}"
 ".hidden{display:none}button,select{font:inherit;padding:6px 14px;margin:4px 8px 4px 0}"
 "pre{white-space:pre-wrap;overflow-wrap:anywhere;font:13px/1.5 monospace;max-height:330px;overflow:auto}"
 "details{background:white;padding:20px;margin:20px 0}summary{font-weight:700;cursor:pointer}"
 "@media(max-width:650px){.cards{grid-template-columns:1fr}h1{font-size:28px}}")
def esc(s):return html.escape(str(s))
opts=''.join('<option value="%s">%s</option>'%(esc(m),esc(d['label'])) for m,d in data.items())
panels=''
for m,d in data.items():
 b,a=d['before'],d['after']
 panels+=('<section class="scenario%s" data-mode="%s">'%('' if m==CASES[0][0] else ' hidden',esc(m))+
  '<p class="summary">'+esc(d['explanation'])+('<br><strong>Unchanged control:</strong> both logs are byte-identical.' if d['control'] else '')+'</p>'
  '<button class="show-decision">Show decision</button><button class="show-log">Show raw logs</button>'
  '<div class="cards decision hidden">'
  '<section class="card original"><h2>Original</h2><div class="num">'+esc(b['value'])+'</div><p>'+esc(CARD_LABEL)+'</p><p>Scenario check: <strong>'+('passed' if b['passed'] else 'failed')+'</strong></p><pre>'+esc('\n'.join(b['notes']))+'</pre></section>'
  '<section class="card corrected"><h2>Corrected</h2><div class="num">'+esc(a['value'])+'</div><p>'+esc(CARD_LABEL)+'</p><p>Scenario check: <strong>'+('passed' if a['passed'] else 'failed')+'</strong></p><pre>'+esc('\n'.join(a['notes']))+'</pre></section>'
  '</div><div class="logs hidden"><h3>Original</h3><pre>'+esc(b['raw'])+'</pre><h3>Corrected</h3><pre>'+esc(a['raw'])+'</pre></div></section>')
page=('<!doctype html><html lang="en"><meta charset="utf-8">'
 '<meta name="viewport" content="width=device-width,initial-scale=1">'
 '<title>'+esc(TITLE)+'</title><style>'+CSS+'</style><main>'
 '<p>Recorded native C runs, original and corrected source</p>'
 '<h1>'+esc(TITLE)+'</h1><p>'+esc(INTRO)+'</p>'
 '<p><label>Scenario: <select id="scenario">'+opts+'</select></label></p>'+panels+
 '<details><summary>Experiment conditions and limits</summary><p>'+esc(CONDITIONS)+'</p></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/%d">issue %d</a>.</p>'%(ISSUE,ISSUE)+
 '</main><script>'
 'var sel=document.getElementById("scenario");'
 'function show(){var m=sel.value;document.querySelectorAll(".scenario").forEach(function(s){s.classList.toggle("hidden",s.getAttribute("data-mode")!==m);});}'
 'sel.addEventListener("change",show);'
 'document.querySelectorAll(".show-decision").forEach(function(b){b.addEventListener("click",function(){b.parentNode.querySelector(".decision").classList.remove("hidden");});});'
 'document.querySelectorAll(".show-log").forEach(function(b){b.addEventListener("click",function(){b.parentNode.querySelector(".logs").classList.remove("hidden");});});'
 'show();</script></html>')
(root/'replay.html').write_text(page)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
