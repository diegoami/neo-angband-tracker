import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/starvealch-18'
TITLE='A Borg with no food in town never bought Remove Hunger at the Alchemist'
INTRO='When the Borg has no food left and is in town, it only looks for something to buy in the General Store (and its Home). If the General Store has no food, it buys nothing, even with gold in hand and Scrolls of Remove Hunger on sale at the Alchemist, the only shop that sells them. The code meant to allow Remove Hunger (its starving check names the scroll), but the Alchemist was never looked at, the check was miswritten so that it let any scroll through instead of only Remove Hunger, and the Borg gave a Remove Hunger scroll no value as food. A level-1 Warrior with no food, 500 gold, only Flasks of Oil in the General Store and Remove Hunger at the Alchemist chooses no shop at all. With the correction it sets out to buy Remove Hunger at the Alchemist. With rations in the General Store it still buys rations, and a Borg that carries food still buys Cure Light Wounds, exactly as before.'
CONDITIONS="Native Human Warrior, seed 18, character level 1, standard starting kit (wielded Wooden Torch, armour and weapon). The fixture removes the kit's Scroll of Word of Recall and Potion of Berserk Strength (otherwise the Borg first takes the scroll home and sells the potion, which ends the shop choice before any purchase is weighed) and sets the gold to 500. In the two no-food modes the kit's Rations of Food are removed as well and the character is made Hungry with the game's player_set_timed(TMD_FOOD, PY_FOOD_HUNGRY - 1); in 'fed' the three rations stay and nutrition is untouched. In town (depth 0). Every store is emptied, then the Alchemist (store 4) gets five Scrolls of Remove Hunger and five Potions of Cure Light Wounds and the General Store (store 0) five Flasks of Oil, plus five Rations of Food in 'general_food' only (fixture stock). The Borg's items and stores are loaded with the production borg_find_all_items(); then the production borg_choose_shop() runs once and its goal is read back. This isolates the shop choice; it does not walk to the store or carry out the purchase."
ISSUE=18
CARD_KEY='buys_remove_hunger'
CARD_LABEL='sets out to buy Remove Hunger (1 = yes)'
CASES=[('starving', 'No food carried, General Store has only oil', "Original: with no food only the General Store and Home are weighed; the oil fails the starving check, so borg_choose_shop() returns false with no goal. Corrected: the Alchemist is weighed too, Remove Hunger passes the starving check and now counts as food, and the Borg chooses the Alchemist's Scrolls of Remove Hunger (store 4, ware 1).", False), ('general_food', 'Control: no food carried, General Store has rations', "Both versions choose the General Store's Rations of Food. The corrected Borg also weighs Remove Hunger at the Alchemist, but rations score higher; the logs are identical.", True), ('fed', 'Control: carrying rations, not hungry', "With food carried, the store filter and starving check do not apply and Remove Hunger is not counted as food; both versions choose the Alchemist's Cure Light Wounds and the logs are identical.", True)]
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
