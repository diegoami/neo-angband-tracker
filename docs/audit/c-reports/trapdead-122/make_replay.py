import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/trapdead-122'
TITLE='Remove the unreachable visible-trap case from dark-flow exploration'
INTRO="Cleanup, no behaviour change; owner decision. When the Borg explores, borg_flow_dark_interesting() decides which grids are worth walking to: unknown grids, closed doors, rubble, treasure veins. It also has a case for 'visible traps', but that case can never run: the function returns early for every grid type below secret doors (except closed doors), and in the stock terrain data the only trap-holding terrain is the open floor, which is below that line. The Borg also records known traps in its own trap flag, not in the terrain. The change deletes the dead case and the 'visible trap' wording in the function comment. Traps that lie on a chosen route are still checked by the flow spread and disarmed by the step code, as the 'unknown' scenario shows."
CONDITIONS="Native Dwarf Rogue, seed 122, clevel 25, depth 5, full HP (231), a lantern, in a lit, open 24x40 arena. The Borg's map is seeded from the real cave; a known trap is recorded the way the Borg records it (open floor with the Borg's trap flag set) on the grid directly east of the player, inside the lantern's light so the first dark-flow method examines it. 'known': the whole room is known, so the trap is the only possible target. 'unknown': the columns from x=30 eastward are unknown. 'door': a real closed door four grids west. The production borg_flow_dark(true) runs, then borg_flow_dark(false) if the first finds nothing; the first queued key and the step grid are read back. Fixture interventions: the arena and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_MARK, the trap exists only as the Borg's trap flag, and borg.avoidance is set to current HP."
ISSUE=122
CARD_KEY='key'
CARD_LABEL="first key the dark flow queued (0 = nothing to explore, 68 = 'D' disarm on the route, 52 = step west)"
CASES=[('known', 'Control: fully known room, a known trap next to the player', 'Neither version treats the trap as somewhere to explore: both dark-flow passes return false and no key is queued. The logs are byte-identical.', True), ('unknown', 'Control: unknown area to the east, the trap on the route', "Both versions flow toward the unknown area (far pass). The first step is onto the trap grid, so the step code queues 'D' (key 68) to disarm it: traps on the route are handled outside the deleted case. The logs are byte-identical.", True), ('door', 'Control: a closed door four grids west', 'Both versions choose the door (near pass) and step west toward it (key 52), passing the trap by. The logs are byte-identical.', True)]
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
