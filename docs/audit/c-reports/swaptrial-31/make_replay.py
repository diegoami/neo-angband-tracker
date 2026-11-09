import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/swaptrial-31'
TITLE="Home backup-item trials erased the Borg's own backups from its pack record"
INTRO="In town the Borg's last two home steps look for a better backup weapon (7A) and backup armour (7B) to carry. They try each home item in the pack slot of the backup the Borg already carries, and after each trial only set that slot's count to zero instead of putting the old backup back. For the rest of that store turn the Borg's own record of its pack no longer held its backup weapon or backup helm. A level-50 Warrior carrying the quarterstaff of Olórin (resists nether) and the helm 'Holhenneth' as backups, with the amulet 'Evenstar' (also resists nether) and the long sword 'Elvagil' in its home, then valued 'Evenstar' as if no backup resisted nether, took it from the home, stored it again the next turn, took it back the turn after and left carrying a redundant amulet. The correction restores the slot after each trial; the Borg keeps its record of both backups, takes nothing and leaves."
CONDITIONS="Native Human Warrior, seed 31, character level 50, maximum depth 60 (the Borg carries backups from depth 50; the shipped borg.txt has borg_uses_swaps = TRUE), 5000 gold, standing on the home entrance in town, all runes known. The starting kit's torch, armour, dagger, food, Word of Recall scroll and Berserk Strength potion are replaced by: worn lantern, long sword 'Ringil', helm of Dor-Lómin (see invisible, telepathy, the four basic resists), ring of Free Action, amulet of Trickery and metal scale mail; carried 5 rations, 5 Potions of Cure Critical Wounds, 5 Scrolls of Teleportation and 2 of Word of Recall. Nothing worn resists nether, confusion or blindness. Backups carried: the quarterstaff of Olórin (except 'no-weapon-swap') and the helm 'Holhenneth'. Every store is emptied; the home holds 'Evenstar' and 'Elvagil' ('swap-held'), 'Elvagil' only ('no-weapon-swap') or nothing ('no-candidate'). Fixture intervention ('no-weapon-swap' only): one sale-history entry saying 'Elvagil' was just stored at home (the fields borg_think_shop_sell() writes), so that steps 4 and 5, which read that history, leave it and step 7A, which does not, reaches it through the empty-slot path. The Borg's items and stores are loaded with the production borg_find_all_items() and borg_notice(); the production borg_choose_shop() makes the first store decision (steps 1 to 6 find nothing to do) and the fixture prints the Borg's record of the two backup slots and the first free slot against the game's pack; then up to six rounds each run one production borg_think_store() turn after re-reading the pack as borg_think() does, and its queued keys are carried out with the real do_cmd_retrieve() or do_cmd_stash(). A slot whose recorded count is zero is printed without its stale description: every reader skips such a slot."
ISSUE=31
CARD_KEY='evenstar_retrieved'
CARD_LABEL="times the Borg takes the redundant amulet 'Evenstar' out of its home"
CASES=[('swap-held', "Backups carried, 'Evenstar' and 'Elvagil' at home", "Original: step 7A tries 'Elvagil' in the quarterstaff's slot and leaves that slot empty; step 7B tries 'Evenstar' in the helm's slot with no backup weapon in the record, so it gets the bonus for a high resist no backup covers and is chosen. After the decision the record shows both backup slots empty while the game's pack still holds both. The real commands then retrieve 'Evenstar' (round 1), stash it (round 2) and retrieve it again (round 3) before the Borg leaves carrying it. Corrected: both slots are restored after each trial, 'Evenstar' is valued with the quarterstaff present, no step chooses anything and the Borg leaves in round 1.", False), ('no-weapon-swap', "Control: no backup weapon, 'Elvagil' at home", "With no backup weapon, step 7A tries 'Elvagil' in the first empty pack slot (the changed restore line runs on an empty slot) and chooses it as the first backup weapon in both versions; the real retrieve takes it and the Borg leaves in round 2. The logs are identical.", True), ('no-candidate', 'Control: backups carried, home empty', 'Nothing in the home, so neither trial loop copies anything into the pack record; both versions keep both backups in the record and leave in round 1. The logs are identical.', True)]
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
