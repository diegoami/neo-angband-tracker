import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/torodvalue-549'
TITLE='The Borg never zaps a Rod of Teleport Other at a dangerous monster'
INTRO="When a monster next to the Borg is dangerous enough, the Borg's defensive step can teleport the monster away with Teleport Other. It accepts the Teleport Other spell, an item that activates for it, or a charged Wand of Teleport Other, but it never looks for a Rod of Teleport Other. A Borg that cannot cast the spell (for example a Warrior) and carries only the rod therefore never uses it: a Warrior down to a third of its hit points next to a hill giant keeps hitting the giant in melee, while the same Warrior with the wand aims it and the giant is sent across the room. After the correction the rod is accepted like the wand (if the Borg's device skill is good enough to zap it), and the Warrior zaps the rod at the giant; the real zap command teleports the giant away and the rod starts recharging. A wand-only Warrior and a Warrior next to a harmless soldier ant behave exactly as before."
CONDITIONS="Native Human Warrior, seed 549, character level 25 (device skill 36), depth 25, in a lit open 22x38 arena; the starting kit's carried items are removed and the Warrior carries only a lit Wooden Torch (worn), 5 Rations of Food and the one device of the case (a charged Rod of Teleport Other, or in the wand control a Wand of Teleport Other with 11 charges); no Teleport Other spell, no escapes, no healing potions. Hit points are set to 33% (106/322) and borg.avoidance to the current hit points, as the Borg does. A stock hill giant (or, in the weak control, a stock soldier ant) is placed adjacent to the east, awake and aware, and registered through the production first-sighting path observe_kill_diff(). The Borg's map is filled from the real terrain. The Borg then runs its production 'try not to die' step borg_caution() (which calls borg_defend()); if that chooses nothing, the next fight step of borg_think_dungeon(), borg_attack(false), is run and its keys recorded. Queued keys are read back; when they are a zap ('z') or aim ('a') of the case's device, the fixture carries it out with the real CMD_USE_ROD / CMD_USE_WAND command at the monster's grid (the target the Borg's '*' keys select), one attempt per command with the game's auto-repeat disabled, up to five attempts, the monster not acting between attempts; game messages are printed as MSG lines. The melee keys chosen by the original Borg in the rod case are recorded, not carried out. This isolates one decision turn; it does not play out the fight."
ISSUE=549
CARD_KEY='used_teleport_other'
CARD_LABEL='uses its Teleport Other device on the hill giant (1 = yes)'
CASES=[('rod', 'Warrior with a Rod of Teleport Other next to a hill giant', "Grid danger 64 against 106 hit points. Original: borg_caution() chooses nothing (the defence never considers the rod), and the attack step queues melee on the giant (keys +6); the rod is not used. Corrected: borg_caution() chooses the Teleport Other defence and queues *p65zb5 (target the giant, zap the rod in slot b, use the target). The real zap command fails three times (the game's device failure chance for this Warrior is 24.2%) and succeeds on the fourth attempt: the giant is moved from (11,12) to (38,22), 33 grids away, and the rod starts recharging.", False), ('wand', 'Control: the same Warrior with a Wand of Teleport Other', 'The same scene with the wand instead of the rod. Both versions choose the defence, aim the wand (keys *p65ab5), and the real aim command teleports the giant to (38,22) at the first attempt (10 charges left). This case runs through both changed lines (the wand is accepted before the rod check and aimed before the zap). The logs are byte-identical.', True), ('weak', 'Control: rod carried, harmless soldier ant adjacent', 'The same Warrior with the rod, next to a soldier ant (grid danger 2 against 106 hit points). No danger to speak of, so borg_caution() chooses nothing and neither version zaps the rod; both queue melee (keys +6). The logs are byte-identical. This control shows the rod is not zapped at harmless monsters; it does not reach the changed lines.', True)]
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
