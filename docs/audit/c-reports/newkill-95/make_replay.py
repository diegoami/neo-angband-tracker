import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/newkill-95'
TITLE='A newly seen wall-walker made the Borg forget the wall'
INTRO="Some monsters, such as an earth spirit, can stand inside solid rock. The Borg has a rule not to shoot at such a monster unless it is on known floor, because the game stops a missile in front of a wall. When the Borg first sees a monster it creates a tracking record; the last step of that routine looked up the monster's race before the record was filled in, so it read an empty race (one with no wall-walking ability) and overwrote the grid in the Borg's map with floor. An earth spirit seen inside a granite pillar four steps away therefore turned the pillar into floor in the Borg's map, and on that turn the Borg fired a sling shot at it: the real fire command spent the turn and the shot, and the earth spirit took no damage. The correction drops that last step, because the routine has already placed the monster on floor or wall with the right race a few lines earlier; the Borg's map keeps the granite and it does not shoot into the rock."
CONDITIONS="Native Human Warrior, seed 1000, character level 1, depth 20, wielding a sling with 20 iron shots, in a lit 38x38 room. The fixture's terrain change is one granite grid four steps east of the Borg (wall case only); the Borg's map is filled from the real terrain, as borg_update_map() does for seen grids. A stock earth spirit is placed one grid further east and moved into that grid with the game's monster_swap(); it is awake and visible. The production first-sighting path observe_kill_diff() -> borg_new_kill() runs once, then the production borg_attack(false) chooses an action and its queued keys are read back. When the Borg chose to fire, the fixture carries it out with the real CMD_FIRE command (quiver shot, target = the monster's grid); game messages are printed as MSG lines. This isolates one decision turn; it does not play out the fight."
ISSUE=95
CARD_KEY='fired'
CARD_LABEL='fires a sling shot at the earth spirit (1 = yes)'
CASES=[('wall', 'Earth spirit inside a granite pillar', "Original: the first sighting overwrites the pillar with floor in the Borg's map (map_granite=0); the Borg fires (keys *p66665f05); the real fire command uses a full turn (energy 100) and one shot, the shot stops at the wall and the earth spirit keeps 56/56 hit points. Corrected: the map keeps granite (map_granite=1), the Borg's own 'never shoot walls' rule holds and no attack is queued.", False), ('floor', 'Control: earth spirit on open floor', 'The same earth spirit on a floor grid. Both versions keep the grid as floor and fire; the shot hits for 4 damage. The logs are byte-identical.', True)]
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
