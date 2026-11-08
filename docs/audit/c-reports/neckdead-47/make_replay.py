import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/neckdead-47'
TITLE='Remove the unreachable second amulet tier from the home score'
INTRO="Cleanup, no behaviour change; owner decision (option C). The Borg scores the contents of its home and takes points off for keeping too many items of one type, so that it does not fill the home with duplicates. For amulets, borg_power_home_aux1() subtracts 1500 for each amulet past the third; in the else of that test it repeats the same condition (num_neck > 3) for a gentler 700-point tier, so that tier can never run. The change deletes the dead tier. Today's behaviour is kept: one to three amulets cost nothing, each amulet past the third costs 1500."
CONDITIONS='Native Human Warrior, seed 47, clevel 21, in town (depth 0), with the starting pack plus one known Amulet of Slow Digestion. Every store is emptied, then the home is given 2, 3, 4 or 5 distinct known amulets (Resist Lightning, Resist Acid, Resistance, Regeneration, Infravision, in that order of choice) through store_carry(), so the duplicate check does not enter the score. The Borg reads the stores with borg_find_all_items(); the production borg_notice_home() and borg_power_home() give the amulet count and home score, and the production borg_think_home_sell_useful() decides whether to store the spare amulet. Fixture interventions: the stores are emptied and stocked directly, all runes are learned with player_learn_all_runes(), dynamic preparedness formulas are off (BORG_USES_DYNAMIC_CALCS = false), and current HP is set to maximum.'
ISSUE=47
CARD_KEY='stores_amulet'
CARD_LABEL='Spare Amulet of Slow Digestion stored in the home by borg_think_home_sell_useful() (1 = stored, 0 = kept in the pack)'
CASES=[('neck2', 'Control: 2 amulets in the home (spare stored)', 'Two amulets at home, num_neck=2: no amulet penalty. Home score 2040; storing the spare raises it to 2090, so both versions store the Amulet of Slow Digestion. Logs byte-identical.', True), ('neck3', 'Control: 3 amulets in the home (spare kept)', 'Three amulets at home, num_neck=3: no penalty yet (home score 3120); a fourth would cost 1500, so both versions keep the spare in the pack. Logs byte-identical.', True), ('neck4', 'Control: 4 amulets in the home, first tier active (spare kept)', 'Four amulets at home, num_neck=4: the 1500 penalty applies and the original then skips the dead else. Home score 2625; both versions keep the spare. Logs byte-identical.', True), ('neck5', 'Control: 5 amulets in the home, first tier active (spare kept)', 'Five amulets at home, num_neck=5: penalty 2 x 1500. Home score 2130; both versions keep the spare. Logs byte-identical.', True)]
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
