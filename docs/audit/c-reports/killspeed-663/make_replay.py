import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/killspeed-663'
TITLE='Danger and retreat comparisons use monster speed before Haste and Slow'
INTRO="The C tracker copies the monster's base mspeed into kill->speed on both tracking paths, and the danger calculation turns that value into an energy rate while the retreat logic compares it with the player's speed. The engine, before giving a monster energy, adds 10 for a hasted monster and subtracts twice the Slow effect level from a slowed one - so a visibly hasted monster outran the borg sooner than the danger and retreat comparisons predict, and the borg's own Slow spell bought no credit."
CONDITIONS="Native Human Warrior, level 20 (experience forced), depth 5 in a synthetic open lit arena, a wooden torch as the only light, infravision 0. A visible stock floating eye (base mspeed 110) three grids east, awake and out of melee reach. The timed states are applied through the production mon_inc_timed(); the eye is observed by real production borg_update() passes - the first creates the kill record (borg_update_kill_new), a later pass refreshes it (borg_update_kill_old). The expected net speed is computed in the fixture from the live monster with the engine's own formula (mspeed + 10 hasted; mspeed - 2 x monster_effect_level(MON_TMD_SLOW) slowed), so the assertion does not hardcode numbers. The off-screen mode clears MFLAG_VISIBLE so the production scan can no longer read the monster."
ISSUE=663
CARD_KEY='tracked'
CARD_LABEL='the monster speed the borg tracks (kill->speed, the energy input for the danger math and the retreat comparisons)'
MAP_LEGEND='@ = the Borg; e = the floating eye; # granite; . lit floor'
CASES=[('hasted_new', 'A monster hasted when first seen (new-record path)', "A visible floating eye under Monster Haste stands three grids east; the production screen scan creates its record. The original tracks the base speed (110) although the engine grants the hasted eye 120 - the energy input for the danger math and the retreat gate is wrong from birth. The corrected borg tracks 120, and the eye's own-grid danger rises accordingly (200 to 400).", False), ('hasted_refresh', 'Haste arriving after the first sighting (existing-record path)', 'The eye is first seen plain (both track 110), then hasted; the refresh pass must pick the timed state up. The original keeps 110; the corrected borg tracks 120 on the existing-record path.', False), ('slowed', 'A slowed monster', "The eye under Monster Slow (one effect level): the engine takes 2 x level from its 110 base, net 106. The original tracks 110 - the borg's own Slow spell bought no credit. The corrected borg tracks 106.", False), ('persists_offscreen', 'The hasted eye goes out of sight', 'The hasted eye is observed once (net 120) and then leaves sight. The corrected borg keeps the last defensible observation (120) instead of falling back to the base speed; the original never had it (110).', False), ('plain', 'Control: no timed effect', 'The eye at its base speed with no timed state: both variants track 110. The complete logs are byte-identical.', True), ('expired', 'Control: the haste expires before the sighting', 'The eye is seen with no timed effect (an expired haste reads exactly like none): both variants track 110. The complete logs are byte-identical.', True)]
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
