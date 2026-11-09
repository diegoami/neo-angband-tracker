import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/rebirthkey-52'
TITLE='A respawning Borg got the wrong race or class'
INTRO="When the Borg dies with the game's 'cheat_live' option on (and its own borg_cheat_death off), or wins with borg_respawn_winners on, it starts a new character by typing keys into the normal birth screens: it picks a race and a class (chosen at random, or fixed by borg_respawn_race / borg_respawn_class in borg.txt) by typing that entry's menu letter. The Borg typed letters from the plain a-z table, but the birth menus label their entries with a table that skips h, j, k and l. From the eighth entry on the Borg typed the wrong letter. Wanting a Dunadan it typed 'i', which is the Half-Troll's letter, and got a Half-Troll; wanting a Blackguard it got a Ranger. Wanting a Half-Troll (or a High-Elf or Kobold) it typed a letter the menu does not have, so the race already highlighted was kept: the new character stayed Human. The random name is still made for the race wanted, so a Half-Troll could get a human name and a Human an orc name. The correction types the same letters the menus use, and the Borg gets the character it chose."
CONDITIONS="Native build with the Borg, seed 52. A previous Human Warrior (Elf Mage for one control) is made natively; the game option cheat_live is on and borg_cheat_death is off, so a fatal hit asks 'Die?'. The respawn race and class are set as borg_respawn_race / borg_respawn_class would set them. A fatal hit is dealt through take_hit(); from then on every key is produced by the Borg's own keypress hook: it answers 'Die?', queues the respawn keys (borg_reincarnate_start), and those keys are consumed by the real death menu (death_screen: New Game, confirm) and the real birth screens (textui_do_birth: quick-start prompt, race, class, roller, point-based stats, name, history, confirm). Fixture interventions: the previous character and the fatal hit are synthetic; the savefile save and the game re-initialisation that close_game()/play_game() do between the death menu and the birth screens are skipped; no message window is attached, so no -more- prompts appear. The race-menu highlight therefore starts on the previous character's race."
ISSUE=52
CARD_KEY='right_character'
CARD_LABEL='character created as wanted (1 = race and class both match)'
CASES=[('race8', 'Wanted Dunadan Warrior (race index 8), previous Human Warrior', "Original: the Borg types 'i' for race 8, which is the Half-Troll's letter in the birth menu, and a Half-Troll Warrior is created (with a name made for a Dunadan). Corrected: it types 'm' and a Dunadan Warrior is created.", False), ('class8', 'Wanted Human Blackguard (class index 8), previous Human Warrior', "Original: the Borg types 'i' for class 8, which is the Ranger's letter, and a Human Ranger is created. Corrected: it types 'm' and a Human Blackguard is created.", False), ('race7', 'Wanted Half-Troll Warrior (race index 7), previous Human Warrior', "Original: the Borg types 'h', which the birth menu does not use; the highlighted race (Human) is kept and a Human Warrior with an orc name is created. Corrected: it types 'i' and a Half-Troll Warrior is created.", False), ('dwarfpriest', 'Control: wanted Dwarf Priest (race 5, class 3), previous Human Warrior', "Below index 7 both tables give the same letters ('f' and 'd'); both versions create a Dwarf Priest and the logs are identical.", True), ('humanwarrior', 'Control: wanted Human Warrior (race 0, class 0), previous Elf Mage', "Letters 'a' and 'a' in both tables; both versions create a Human Warrior and the logs are identical.", True)]
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
