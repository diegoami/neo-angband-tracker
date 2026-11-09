import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/tporod-17'
TITLE='Only Rogues save up for the Black Market Rod of Teleport Other'
INTRO="The Borg has a Black Market exception for a few items it should save up for. For the Rod of Teleport Other the comment says Mages and Rogues are excluded because they can cast Teleport Other, but the code reads 'not a Mage and IS a Rogue', so the exception applies to Rogues only. A level-30 Rogue who knows Teleport Other sets a 15000 gold target and stays in town resting for gold; a level-30 Warrior, who cannot cast it, does not. After the correction ('not a Mage and not a Rogue') the Warrior saves up and the Rogue does not. On its own the Borg still gives the rod no value (issue #560), so a level-12 Warrior with 16000 gold accepts the rod but does not buy it; the stacked run in stacked/ applies #560's correction and shows that Warrior buying the rod only with this correction. A Mage who knows the spell behaves exactly as before."
CONDITIONS="Native Human, seed 17, starting kit off, character level by real experience gain (30, or 12 in warrior-buy), base Dexterity set to 18/120 so the Borg's anti-theft condition for saving up (adj_dex_safe + level > 90) holds for the level-30 characters, in town (depth 0), a worn lit Wooden Torch and 120 Rations of Food (the saving rule wants at least 100 food). Gold is 10000 (16000 in warrior-buy). The Rogue carries Arcane Control and the Mage Magical Defences, each with its spells studied through the game's spell_learn(); both books hold Teleport Other. Every store's stock is emptied and the Black Market is given one Rod of Teleport Other through store_carry() (price 15000; no normal store stocks this rod). The Borg reads inventory and stores with borg_find_all_items(); the production shop step borg_think_shop_buy_useful() runs, then the production town step borg_money_scum(), and the first queued key is recorded (57344 is the Borg's rest key). Only the decisions are recorded; the fixture does not walk to the shop, buy, or rest for gold. The default profile has borg_self_scum on."
ISSUE=17
CARD_KEY='saves_for_rod'
CARD_LABEL='sets a gold target to save up for the rod (1 = yes)'
CASES=[('warrior', 'Warrior, level 30, 10000 gold', 'Original: the Warrior is not on the exception, and the general Black Market rule rejects the rod (level under 35 needs 15000 gold); no gold target, no resting (0/1). Corrected: the exception applies; the Borg records a 15000 gold target and borg_money_scum() starts resting in town (first key 57344) (1/1). It cannot buy yet: 10000 gold is short of the price.', False), ('rogue', 'Rogue who knows Teleport Other, level 30, 10000 gold', 'Original: the exception applies to the Rogue only; a 15000 gold target is recorded and the Rogue rests in town for a rod it does not need (0/1). Corrected: no gold target, no resting (1/1).', False), ('warrior-buy', 'Warrior, level 12, 16000 gold (needs #560)', "Original: the general Black Market rule wants 20000 gold below level 15; the rod is rejected. Corrected: the exception accepts the rod and records the 15000 target, but the shop step still does not buy it because the rod adds no power without #560 (buy_useful=0). Both fail the buy contract here; with #560's correction applied (stacked/), the corrected Borg buys the rod.", False), ('mage', 'Control: Mage who knows Teleport Other, level 30, 10000 gold', 'The changed line is evaluated for the Mage and excludes it in both versions; the general rule rejects the rod; no target, no resting. The logs are byte-identical.', True)]
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
