import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/healchance-364'
TITLE='A Borg in real danger never drank its Healing potion'
INTRO="Before using any healing item, the Borg rolls a chance and skips healing when the roll is below a threshold for how hurt it is. Three adjustments to that roll go the opposite way from their comments. The worst is 'if danger is close to the hp and healing will help, do it': it subtracts 75, so whenever the danger next to the Borg is at least its current hit points (but below its maximum) it skips every healing step until it has lost 75% of its hit points. A level-31 Warrior on 149 of 374 hit points beside a fire giant (danger 192), carrying three Potions of Healing, never drank one in 100 decisions: it tried to teleport (none carried) and read Word of Recall, which only takes effect many turns later, while staying next to the giant. With the three signs flipped as the comments say, it drinks a Potion of Healing in 100 of 100 decisions and the real quaff command takes it to 374 hit points. Next to a unique it now heals more often (18 to 32 of 100, against 23 with no unique) and a risky Borg less often than a careful one (27 to 20, against 23)."
CONDITIONS="Native Human Warrior, seed 364, level 31, depth 30, 149 of 374 hit points (60% lost: the 51-65% band of the gate), starting kit (Potion of Berserk Strength, Word of Recall, no teleport) plus three known Potions of Healing, a lit 38x22 room, one awake monster adjacent (south-east). For each case the production borg_think_dungeon() unique scan (borg_near_monster_type) and borg_caution() run 100 times with the RNG seeded 1..100, and each decision's queued keys and '# Healing Level' note are recorded; a further decision (seed 1000) is carried out with the real quaff command when the Borg queued one. Modes: 'danger' (fire giant, danger 192, at or above current HP), 'unique' (Golfimbul, danger 44), 'risky' (cave orc, danger 8, borg_plays_risky turned on, compared with the same seeds with it off) and 'control' (cave orc, danger 8). Fixture interventions: the arena, lighting and hit points are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_VIEW, borg.avoidance is set to current HP (as borg_update does), the monster is marked awake in the Borg's list, the RNG is fully reset and reseeded per decision (the WELL index state_i is zeroed, since Rand_state_init() does not reset it), and borg_plays_risky is set in the risky mode."
ISSUE=364
CARD_KEY='heals'
CARD_LABEL='decisions (of 100) in which borg_heal() healed'
CASES=[('danger', 'Fire giant adjacent: danger 192 at or above current HP 149', "Original: 0 of 100 decisions heal; the Borg flees by reading Word of Recall ('Fleeing (failed to teleport)') next to the giant. Corrected: 100 of 100 drink a Potion of Healing ('Healing Level 7'); the real quaff command takes the Warrior from 149 to 374 hit points.", False), ('unique', 'Golfimbul adjacent: ordinary danger 44, unique nearby', "'Increase the odds of healing' near a unique. Original: 18 of 100 decisions heal, fewer than the 23 of the control with the same seeds and no unique. Corrected: 32 of 100.", False), ('risky', 'Cave orc adjacent, borg_plays_risky on', "'Risky Borgs are less likely to heal'. Original: 27 of 100 heal, more than the 23 with the same seeds and risky off. Corrected: 20 of 100, fewer than 23.", False), ('control', 'Control: cave orc adjacent, ordinary danger 8', 'No adjustment applies (no unique, not risky, danger below current HP): the gate draws the same roll and only the unchanged class adjustment applies. 23 of 100 heal in both versions; logs byte-identical.', True)]
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
