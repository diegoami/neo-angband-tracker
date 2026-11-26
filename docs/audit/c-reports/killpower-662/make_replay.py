import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/killpower-662'
TITLE='A live monster can round down to 0 tracked power and lose its attack value'
INTRO="When the Borg tracks a monster it cheats the monster's true hit points from the game and stores an estimate called power, which drives danger math and attack scoring; damage against ordinary monsters is capped at twice that value. Both tracking paths - the first observation of a monster and the refresh of a known one - estimate the hit points by first rounding them into a percentage (100 * hp / maxhp) and then rounding again when converting back (maxhp * pct / 100). Small remainders are lost twice: a floating eye at 1 of its hit points becomes 9% and then 0 tracked power, so the Borg reads a living monster as having no health worth attacking - the recorded level-1 mage stopped shooting a wounded floating eye, stepped next to it, and was paralysed to death. The fixture places a visible floating eye in a lit arena, sets its true hp, runs two production borg_update() passes (so both tracking paths run) and requires the tracked power to equal the monster's true hp."
CONDITIONS="Native Human Warrior, level 20 (experience forced), depth 5 in a synthetic open lit arena, a wooden torch as the only light, infravision 0. The floating eye is the stock race (maxhp rolled per instance) one grid east, awake and visible; its true hp is set by the mode before the first observation. Two production borg_update() passes run: the first creates the kill record (new-record path), the second refreshes it (existing-record path). No combat, movement or random placement consumes the RNG (fixed seed 662); a scenario log is fully determined by its mode. Limits: the fixture measures the tracked power on both tracking paths; the attack score, danger and the walk-closer decision that consume power are identified in the source and their inputs change with it, but no individual attack is replayed here (the recorded paralysis death is the Rust port's motivating case, quoted on the issue)."
ISSUE=662
CARD_KEY='borg_power'
CARD_LABEL="the Borg's tracked power (hit points) for the living monster; the game's true hp is on each log's KILLINFO line"
MAP_LEGEND="@ = the Borg; o = the floating eye; the terrain glyphs are the game's own symbols"
CASES=[('full', 'Eye at full health (control)', 'The floating eye at full health. Both variants track its power correctly; the complete log is byte-identical.', True), ('one_of_hp', 'The 1-hp boundary', 'The eye at 1 of its hit points. The original borg rounds the percentage down (9%) and then rounds again, tracking power 0 - the eye reads as having no health worth attacking, and the attack-damage cap (twice the tracked power) drops to 0. The corrected borg tracks power 1 and a cap of 2.', False), ('above_edge', 'Two hp, just above the edge', 'The eye at 2 hp. The original borg tracks power 1 where the eye has 2; the corrected borg tracks 2 (cap 4).', False), ('zero_alive', 'Exactly 0 hp while alive (control)', 'The engine kills a monster only below 0 hp, so a monster standing at exactly 0 hp is alive and is tracked as 0 in both variants - the faithful reading. The complete log is byte-identical.', True)]
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
