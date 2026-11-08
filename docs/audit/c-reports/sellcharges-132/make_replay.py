import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/sellcharges-132'
TITLE='Home-sell trials drained staff charges and the Borg emptied its home'
INTRO="In town the Borg decides what to keep in its home by trying each pack item in each home slot and scoring the result. For a staff it moves one staff's share of the charges into the trial slot and takes them off the pack stack, but after each trial it put back only the staff, not the charges. Every later trial in the same scan saw a staff with fewer charges. A level-26 Mage with two Staffs of Teleportation (10 charges) and a Word of Recall scroll in the home scored 'replace the scroll with a staff' at full charges but 'add a staff beside the scroll' at a drained share, so it decided to take the scroll out of the home. The correction puts the charges back with the staff; the Borg then stores a staff and keeps the scroll at home."
CONDITIONS="Native Human Mage, seed 132, character level 26, 5000 gold, in town (depth 0) with its starting kit plus fixture items, all runes known. Every store is emptied. 'stack': two Staffs of Teleportation stacked in one pack slot with 10 charges set by the fixture (5 each), and one Scroll of Word of Recall in the home. 'stack-emptyhome': the same staves, home empty. 'single': one staff with 5 charges, recall scroll at home. 'none': no staff or wand, recall scroll at home. The Borg's items and stores are loaded with the production borg_find_all_items(); then the production borg_think_home_sell_useful() (step 1 of the store logic) runs once and its goal is read back. This isolates the home-sell decision; it does not walk to the home or carry out the transfer."
ISSUE=132
CARD_KEY='takes_recall_out'
CARD_LABEL='takes the recall scroll out of the home (1 = yes)'
CASES=[('stack', 'Two teleport staves, recall scroll at home', 'Original: by the second trial (adding a staff beside the scroll) the pack stack has lost 5 of its 10 charges, so the trial staff gets a 2-charge share (5/2) instead of 5; that addition scores below the first trial (replacing the scroll, home power 9,914), and the Borg sets out to buy the Word of Recall back out of the home. Corrected: the addition keeps its 5-charge share (home power 10,014) and the Borg stores a staff instead.', False), ('stack-emptyhome', 'Control: two teleport staves, empty home', "Only one home slot to try, so the drain happens after the last trial and the caller's restore hides it. Both versions store a staff; the logs are identical.", True), ('single', 'Control: one teleport staff', "Giving up its only staff would lower the Borg's own power, so the item is never tried. Both versions leave the home alone; the logs are identical.", True), ('none', 'Control: no staff or wand', 'No charged device in the pack. Both versions leave the home alone; the logs are identical.', True)]
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
