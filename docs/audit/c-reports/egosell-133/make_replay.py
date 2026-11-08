import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/egosell-133'
TITLE='The Borg gave away an unidentified Cloak of Aman'
INTRO='In town the Borg looks for pack items it can sell without losing anything. It is meant to keep an ego item whose random extra power (for example the hidden high resist on a Cloak of Aman) it has not learned yet, unless it carries a stack of them. That check looked up the ego of the cloak the Borg was wearing instead of the item being sold. A level-21 Warrior wearing a plain Cloak and carrying a Cloak of Aman whose random resist (nether) was still unknown therefore chose to sell the Cloak of Aman to the Black Market; carrying out that sale with the real store command handed the cloak over and only then revealed its resist. The correction asks about the item being sold; the Borg then keeps the cloak.'
CONDITIONS="Native Human Warrior, seed 133, character level 21, max depth 20, in town standing on the Black Market entrance, birth_no_selling at its default (on), BORG_WORSHIPS_GOLD and BORG_MONEY_SCUM_AMOUNT off, static preparedness. The starting pack is emptied (equipment kept) and every store is emptied. The carried spare is a Cloak of Aman created first from the same RNG state in both modes; every rune is known except the spare's random high resist (nether), so its ego name is shown but it is not identified ({??}). 'plain-worn': a plain Cloak is worn. 'aman-worn': a Cloak of Aman is worn instead. The production borg_find_all_items(), borg_notice() and borg_power() load the state; the production borg_think_shop_sell_useless() (step 2 of the store logic) runs once and its goal is read back; if it chose the cloak, the real do_cmd_sell() runs on it. This isolates the sell choice and the sale; it does not play the walk through town or the Borg's other store steps."
ISSUE=133
CARD_KEY='sold_unidentified_aman'
CARD_LABEL='sold the unidentified Cloak of Aman (1 = sold)'
CASES=[('plain-worn', 'Plain Cloak worn, unidentified Cloak of Aman carried', "Original: the random-power check asks about the worn plain Cloak (no ego, no random power), so the Cloak of Aman counts as safe to sell; the Borg picks it for the Black Market (shop 6, item a) and the real sale hands it over ('You had a Cloak of Aman [1,+14] <+1> (a).'), learning the nether rune only as it goes. Corrected: the check asks about the Cloak of Aman itself and the Borg sells nothing.", False), ('aman-worn', 'Control: Cloak of Aman worn, unidentified Cloak of Aman carried', 'The worn cloak and the carried one have the same random-power ego, so both versions give the same answer: nothing is sold. The changed check runs in both versions; the logs are byte-identical.', True)]
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
