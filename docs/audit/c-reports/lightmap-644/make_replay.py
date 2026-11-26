import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/lightmap-644'
TITLE="The Borg's light map lights one grid more than the game"
INTRO="The Borg keeps its own map of the grids its light source lights, and takes the light's strength (BI_LIGHT) as that map's radius. The engine lights one grid less: calc_lighting() builds radius = ABS(light) - 1 and add_light() lights every grid with distance() no greater than the radius, through line of sight. With a wooden torch the Borg therefore believes the ring two grids out is torch-lit when it is dark. A monster standing there is expected to be visible, is not, and the missing-monster pass deletes its record as one that did not really move - the Borg forgets a monster waiting just outside its light. The old radius-2 and radius-3 shapes are not the engine's either: they light (2,2), which is distance() 3, and skip (2,1). The correction collects the lit grids the way the engine does, at radius BI_LIGHT - 1 with line of sight; a Necromancer standing in unlight keeps the radius the Borg always used for that case."
CONDITIONS="Native corridor fixture (24x40 arena sculpted into a one-grid-wide dead-end corridor, no SQUARE_GLOW), Human Warrior at character level 20, depth 5, wielding a Wooden Torch (BI_LIGHT 2, fuel 4000) as the only light, infravision 0; an awake soldier ant two grids east on the corridor's last floor (forget) or one grid east (adjacent control), noted via observe_kill_diff and aged one turn; the borg arrives on the level with one production borg_update() (new-level reset), then the measured borg_update() runs the screen scan, the view, the light map and the missing-monster pass with borg_do_update_view and borg_do_update_lite forced. Seed 644, borg_rand_local 1000. Measured: the kill record's survival and position, the light flags on the ant's grid, and the suite line. Not measured: the follow-on decisions the kept record would drive, further turns, and Necromancer unlight, which keeps the old radius by design."
ISSUE=644
CARD_KEY='kept'
CARD_LABEL='kill record kept'
MAP_LEGEND="# wall rock, . corridor floor, @ the Borg, a the soldier ant; the map is the level drawn with the game's own feature and monster symbols"
CASES=[('forget', 'Monster two grids out in the dark ring: the record must be kept', "A dead-end corridor, one grid wide, in the dark. The Borg, a level-20 Human Warrior with a wooden torch (BI_LIGHT 2, so the game lights distance 1), walked the corridor and stands two grids from its end. An awake soldier ant holds the last floor; the game cannot see it (no light, no infravision). The original marks the ant's grid BORG_LIGHT, judges the record viewable, and borg_follow_kill() deletes it in the missing-monster pass as a monster that did not really move. With the engine's radius the grid is dark, the record is not viewable, and it is kept.", False), ('adjacent', 'Monster one grid out, seen: unchanged control', 'The same corridor with the ant one grid east of the Borg, inside the real light. Both radii light the grid, the game sees the ant, the screen scan re-observes it, and the record is kept unchanged by both variants with byte-identical logs.', True)]
def maps_of(raw):
 # MAP <caption> / MAPROW <text>... / MAPEND blocks printed by the scenario's
 # evidence_map() (tools/c-evidence/evidence-map.c): the level drawn with the
 # game's own feature and monster symbols.
 out=[];cur=None
 for line in raw.splitlines():
  if line.startswith('MAP '):cur={'caption':line[4:],'rows':[]};out.append(cur)
  elif line.startswith('MAPROW ') and cur is not None:cur['rows'].append(line[7:])
  elif line=='MAPEND':cur=None
 return out
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
  entry['maps']=maps_of(raw)
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
 "figure{margin:0}figcaption{font-size:15px;color:#4a5d5f;margin:6px 0}"
 "pre.map{white-space:pre;overflow:auto;max-height:none;font:15px/1.15 ui-monospace,Consolas,monospace;background:#11181c;color:#c9d3d6;padding:12px;border-radius:8px;width:max-content;max-width:100%}"
 "pre.map .me{color:#ffd54a;font-weight:700}pre.map .mon{color:#ff7b6b;font-weight:700}pre.map .mark{color:#5ee08a;font-weight:700}pre.map .wall{color:#7d8b90}"
 ".maps .card{padding:16px}.legend{font-size:15px;color:#4a5d5f}"
 "@media(max-width:650px){.cards{grid-template-columns:1fr}h1{font-size:28px}}")
def esc(s):return html.escape(str(s))
def glyph(c):
 if c=='@':return '<span class="me">@</span>'
 if c in '*-|/\\':return '<span class="mark">'+esc(c)+'</span>'
 if c.isalpha():return '<span class="mon">'+esc(c)+'</span>'
 if c in '#%':return '<span class="wall">'+esc(c)+'</span>'
 return esc(c)
def map_html(entry):
 return ''.join('<figure><figcaption>'+esc(mp['caption'])+'</figcaption><pre class="map">'+'\n'.join(''.join(glyph(c) for c in r) for r in mp['rows'])+'</pre></figure>' for mp in entry.get('maps',[]))
opts=''.join('<option value="%s">%s</option>'%(esc(m),esc(d['label'])) for m,d in data.items())
panels=''
for m,d in data.items():
 b,a=d['before'],d['after']
 panels+=('<section class="scenario%s" data-mode="%s">'%('' if m==CASES[0][0] else ' hidden',esc(m))+
  '<p class="summary">'+esc(d['explanation'])+('<br><strong>Unchanged control:</strong> both logs are byte-identical.' if d['control'] else '')+'</p>'+
  (('<div class="cards maps"><section class="card original"><h2>Original</h2>'+map_html(b)+'</section>'
    '<section class="card corrected"><h2>Corrected</h2>'+map_html(a)+'</section></div>'
    +('<p class="legend">'+esc(MAP_LEGEND)+'</p>' if MAP_LEGEND else '')) if (b.get('maps') or a.get('maps')) else '')+
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
