import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/dimdoor-21'
TITLE='Dimension Door aimed at solid rock and landed the Borg back beside its attacker'
INTRO="Dimension Door lets a Mage teleport to a grid of its choice. When the Borg uses it to escape, it scores every grid within 50 steps by its danger estimate and aims at the least dangerous one, but it never checks that the player can stand there. The scan starts in the far north-west corner, so the winner is usually a grid of solid rock the Borg has never seen. The game does not put the player inside rock: it searches outward from the chosen grid for the nearest grid the player can stand on. In this run a badly hurt level-50 Mage is hit by a nether bolt from an invisible dread in its room, and a second, explored room lies 34 steps east. The original Borg aims at (16,1), unknown to it and granite in the game; the game puts the player at (60,31), the west edge of the same room, 9 steps from the dread and in its line of sight, inside the region the Borg was trying to leave. The corrected Borg only aims at grids it knows to be floor with no monster on them; it aims at (100,25) in the explored room, and the game puts the player exactly there, out of the dread's line of sight. When the least dangerous grid is already a known floor grid (an explored open level), both versions aim at it and land on it, with identical logs."
CONDITIONS="Native Human Mage, seed 21, character level 50, depth 40, maximum Intelligence, [Arcane Control] carried and only Dimension Door learned (5% failure when the escape is chosen; no other escape carried). Level 66x198. granite: solid granite except room A (x 60-72, y 25-35) holding the player at (66,30) and an invisible dread at (69,32), and room B (x 100-110, y 25-35). open: the whole level is lit floor (control). Real code: the game's monster spell code (do_mon_spell with the dread's nether bolt, which really damages the player), the Borg's message parser (borg_parse) and map/message update (borg_update, three passes), which raises its regional fear around the unseen caster, the production escape routine borg_escape() called with the danger of the Borg's grid as borg_caution() calls it, and the game's spell code (spell_cast, the TELEPORT_TO effect) which moves the player. Fixture interventions: the level, rooms and dread are placed directly; the player starts at a quarter of maximum hit points (as after earlier fighting); the level is mapped with the game's wiz_light() and the floor of room B (granite) or of the whole level (open) is memorised as seen; the dread's spell is cast by calling do_mon_spell directly (its own spell choice is not run); an EVENT_MESSAGE handler hands each game message to borg_parse instead of the Borg scraping the screen; the Borg's queued keys are decoded by the fixture (the free-targeting cursor walk from the player, then 'm', book, spell and '5'), checked to be Dimension Door from the carried book, and executed by setting that target with target_set_location() and calling spell_cast() with a direction hook that answers '5' (use target)."
ISSUE=21
CARD_KEY='on_aim'
CARD_LABEL='Dimension Door puts the player where the Borg aimed (1 yes, 0 no)'
CASES=[('granite', "Rock level: player's room with an invisible dread, an explored room 34 steps east", 'Original: aims at (16,1), a grid the Borg has never seen and granite in the game, with a scored danger of 0; the game puts the player at (60,31), in the same room, 9 steps from the dread and in its line of sight, regional fear there 78 (as at the start). Corrected: aims at (100,25), the first zero-danger known floor grid, and lands exactly there, 34 steps from the dread, out of its line of sight, regional fear 0.', False), ('open', 'Control: explored open level, least dangerous grid is floor', 'On an explored open level the least dangerous grid in scan order, (16,1), is known floor with no monster. Both versions aim at it and the game puts the player exactly there; byte-identical logs.', True)]
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
