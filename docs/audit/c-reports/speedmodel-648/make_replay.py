import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/speedmodel-648'
TITLE="The borg's speed model misses Slow and under-rates Terror"
INTRO="While a speed effect is active, the borg keeps a one-number model of how fast its character is (BI_SPEED, starting at the game's normal 110) and compares it with every monster's speed to decide when it can safely approach, wait, or trade blows. The model added +10 for haste or sprint and otherwise +5 for terror, and never applied the slow penalty at all - the engine applies +10 haste/sprint, -10 slow and +10 terror, independently. So a slowed character is planned at full speed, a terrified one at half the real bonus, and haste plus terror loses the terror bonus. The fixture sets each timed effect through the real engine function, recomputes the game's own speed and one production borg_notice(), and requires the two numbers to match."
CONDITIONS="Native Human Warrior, level 18 (experience forced), depth 5 in a synthetic open lit arena with no monsters, a wooden torch as the only light, infravision 0, unencumbered. The timed effects are set with the engine's own player_set_timed() (no message), the game's speed is recomputed with PU_BONUS, and one production borg_notice() derives the borg's model. No combat, movement or random placement consumes the RNG (fixed seed 648), so a scenario log is fully determined by its mode. Limits: the fixture measures the speed model itself, not a specific fight; the consumers of BI_SPEED (monster-speed comparisons in approach/wait decisions, energy-per-turn danger math, power valuation) are quoted from the source and not separately exercised."
ISSUE=648
CARD_KEY='borg'
CARD_LABEL="the Borg's modelled speed score (the game's own speed is on the same log's SPEEDINFO line)"
MAP_LEGEND="@ = the Borg; the terrain glyphs are the game's own symbols"
CASES=[('none', 'No timed effect (control)', "No timed effect is active. Both variants report speed 110, the game's own value, and the complete logs are byte-identical.", True), ('fast', 'Haste only (control)', "Only haste is active. Both variants already add the engine's +10 and report 120; the complete logs are byte-identical.", True), ('slow', 'Slowed', 'The character is slowed. The game grants speed 100 (its -10 penalty); the original borg keeps planning at 110 - it believes it acts twice as fast as it does (energy 10 per turn budgeted where the game grants 5). The corrected borg reports 100.', False), ('terror', 'Terrified', 'Only terror is active. The engine adds +10 (speed 120); the original borg adds only 5 (115), so it thinks a monster at 120 is not faster. The corrected borg reports 120.', False), ('fast_terror', 'Haste and terror together', "Haste and terror are active at once. The engine adds +10 and +10 (130); the original borg's else-if drops the terror bonus entirely and reports 120. The corrected borg reports 130.", False), ('slow_terror', 'Slow and terror together', 'Slow and terror are active at once. The engine nets -10 + 10 (110); the original borg ignores the slow penalty and adds a half terror bonus (115). The corrected borg reports 110.', False)]
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
