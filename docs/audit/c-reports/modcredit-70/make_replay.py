import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/modcredit-70'
TITLE="Credit stat and speed bonuses in the Borg's item value guess"
INTRO="Behaviour change; owner decision (option A). The Borg guesses what each known item would fetch in a shop and uses that guess to decide what to sell first, what to junk and what is worth carrying. For wearables the guess is meant to add credit for stat bonuses (200 per point), speed (30000 per point), stealth, blows, shots and the other bonuses, but the whole credit block was skipped whenever the item's pval was zero, and in 4.2 that is every ring, amulet, armour and weapon (bonuses are modifiers, pval is set only for launchers, a flask of oil and a mushroom). The stat lines also tested stat numbers as object flags. So a Ring of Strength <+2> was guessed at its kind cost of 500, the same as a Ring of Strength <+1>, and the Cloak 'Colannon' (speed +2) at 21500, below an Amulet of ESP. When both items are spare, the Borg sold the bonus item first. The change removes the pval gate and credits the five stat modifiers directly (a negative stat modifier earns no credit). Other negative modifiers subtract, and the total is floored at 0 (owner decision, option B) in place of the old pval < 0 test, which never fires in 4.2: an Amulet of Inertia <-2> is guessed at 0 rather than -59990, and the Boots of Thror (speed +3, stealth -1) keep their credit. The guessed value of every wearable with modifiers changes; whole-game effects are not measured."
CONDITIONS="Native Human Warrior, seed 70, clevel 21, max depth 20, in town, 1000 gold, the starting equipment worn and the pack emptied except for two known items, all runes learned. 'statring': a Ring of Strength <+2> and a Ring of Soulkeeping. 'speed': the Cloak 'Colannon' [1,+15] <+2> (speed +2, stealth +2; the standard artifact made with copy_artifact_data()) and an Amulet of ESP. 'nomods': a Ring of Free Action and a Ring of Bodykeeping, neither with any modifier. 'inertia': an Amulet of Inertia <-2> and a Ring of Soulkeeping. 'thror': the Boots of Thror [7,+20] <+3, -1> (STR +3, CON +3, speed +3, stealth -1; the standard artifact) and a Ring of Soulkeeping. Rings and amulets are made with object_prep() at level 40 with average rolls. Every store is emptied so no shop is full. The production borg_think_shop_sell_useless() chooses the sale (the cheaper item by the Borg's guess, capped at 30000, among items whose sale does not lower the Borg's power); the chosen item is then sold with the real do_cmd_sell() while standing in the chosen shop. The game's own valuation (object_value_real()) is printed for reference. Fixture interventions: the pack is emptied and the items are added directly, all runes are learned with player_learn_all_runes(), the stores are emptied directly, dynamic formulas, gold worship and money scumming are off, HP is set to maximum, and the player's grid is turned into the chosen shop's entrance before the sale."
ISSUE=70
CARD_KEY='sold_b'
CARD_LABEL='The second item sold (1 = sold the second item and kept the first: the plain ring, the ESP amulet or the Soulkeeping ring; 0 = it did not)'
CASES=[('statring', 'Stat ring: Ring of Strength <+2> vs Ring of Soulkeeping', "Original: the Ring of Strength <+2> is guessed at 500 (no stat credit), below the Ring of Soulkeeping at 600, so the Borg sells the Ring of Strength. Corrected: 500 + 2 x 200 = 900, so it sells the Ring of Soulkeeping and keeps the Ring of Strength. The game's own values are 1116 (Strength <+2>) and 1050 (Soulkeeping).", False), ('speed', "Speed item: Cloak 'Colannon' <+2> vs Amulet of ESP", "Original: Colannon is guessed at 21500 (artifact cost 20000 + armour bonus 1500, no speed credit), below the Amulet of ESP at 25000, so the Borg sells Colannon. Corrected: 21500 + 2 x 30000 (speed) + 2 x 100 (stealth) = 81700, capped at 30000 for the sale order, so it sells the Amulet of ESP and keeps Colannon. The game's own values are 11550 (Colannon) and 7134 (ESP).", False), ('nomods', 'Control: two rings without modifiers (Free Action vs Bodykeeping)', 'Neither ring has a modifier, so the corrected code adds nothing: Free Action 1500, Bodykeeping 750 in both versions, and both sell the Ring of Bodykeeping. Logs byte-identical.', True), ('inertia', 'Speed penalty: Amulet of Inertia <-2> vs Ring of Soulkeeping', "Original: the Amulet of Inertia is guessed at its kind cost of 10, below the Ring of Soulkeeping at 600, so the Borg picks it to sell; the shop refuses it ('I do not wish to purchase this item', game value 0) and nothing is sold. Without the floor (earlier fix head 46c6188ec): 10 - 2 x 30000 = -59990. Corrected (floor): 0, so the Borg leaves the amulet out of the sale and sells the Ring of Soulkeeping. The earlier fix head makes the same sale; the floor changes the guess, not this decision.", False), ('thror', 'Stealth penalty: Boots of Thror <+3, -1> vs Ring of Soulkeeping', 'Original: the Boots of Thror are guessed at 14000 (artifact cost 12000 + armour bonus 2000). Corrected: 14000 + 3 x 200 (STR) + 3 x 200 (CON) + 3 x 30000 (speed) - 100 (stealth -1) = 105100, the same as without the floor; the stealth penalty only lowers the credit. Both versions sell the Ring of Soulkeeping and keep the boots.', False)]
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
