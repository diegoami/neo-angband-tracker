import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/torodpower-560'
TITLE='The Borg gives a Rod of Teleport Other no value and never buys one'
INTRO="The Borg values Teleport Other highly, especially for a Warrior, who cannot cast it: its power score rises by 60000 for the first charge of a Wand of Teleport Other. But only the wand and the spell count. A Rod of Teleport Other adds nothing to the Borg's Teleport Other count, so its power score does not move for the rod and the shop step never buys one. A level-30 Warrior with 20000 gold standing in front of a Black Market that offers the rod for 15000 leaves it there. After the correction each carried rod counts like one wand charge: the same Warrior sees its power rise by 60000 and the production shop step chooses to buy the rod. A Warrior who already carries Wands of Teleport Other with 16 charges, and a Mage who knows the Teleport Other spell, gain nothing from the rod and behave exactly as before."
CONDITIONS="Native Human, seed 560, character level 30 by real experience gain, 20000 gold, in town (depth 0). The starting kit is off; the character carries a lit Wooden Torch (worn) and 5 Rations of Food, plus per case: nothing else (Warrior), two Wands of Teleport Other with 16 charges in all (Warrior, wand control; object.txt gives 6+d5 charges per wand), or Magical Defences with its spells studied through the game's spell_learn() (Mage control). Every store's stock is emptied and the Black Market is given one Rod of Teleport Other through store_carry() (price 15000); no normal store stocks this rod (store.txt). The Borg reads inventory and stores with borg_find_all_items(), then the production shop step borg_think_shop_buy_useful() runs. A TRACK line reports, as a diagnostic, the Borg's power with and without the rod in an empty pack slot (the same evaluation the shop step makes). Only the purchase decision is recorded; the fixture does not walk to the shop or carry out the buy command."
ISSUE=560
CARD_KEY='buys_rod'
CARD_LABEL='chooses to buy the Black Market Rod of Teleport Other (1 = yes)'
CASES=[('rod', 'Warrior with 20000 gold, no Teleport Other', 'Original: Teleport Other count 0 with the rod too, power 805500 with and without it, so borg_think_shop_buy_useful() chooses nothing (buy_useful=0). Corrected: the rod counts as 1, power rises to 865500 (+60000: 5000 for having Teleport Other, 50000 more for a Warrior, 5000 for the charge), and the shop step chooses Black Market ware 0, the rod (buy_useful=1 goal_shop=6 goal_ware=0).', False), ('wand', 'Control: Warrior already carrying Wands of Teleport Other (16 charges)', "The Warrior's count is already 16 from the wand charges. The corrected code counts the rod as one more (the changed line runs while the shop step evaluates the rod), but the reward per charge stops at 15, so power stays 935500 and neither version buys the rod. The logs are byte-identical.", True), ('mage', 'Control: Mage who knows Teleport Other', "The Mage's count is already 1000 from the spell. The rod adds one in the corrected code (the changed line runs), power stays 1516641 and neither version buys the rod. The logs are byte-identical.", True)]
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
