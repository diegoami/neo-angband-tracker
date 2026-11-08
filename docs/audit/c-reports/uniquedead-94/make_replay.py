import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/uniquedead-94'
TITLE='A shape-changed monster made the Borg think Grip was dead'
INTRO="The Borg keeps a table of which uniques are dead. When it looks at a monster whose race is a unique the player has already killed, a fix-up marks that unique as dead. The fix-up used the number of the Borg's own monster record instead of the monster's race number, so it marked an unrelated race as dead, permanently for the session. This can happen in normal play: a Maia of Aulë can change into the shape of Quaker, Master of Earth, even after the player killed Quaker. In the fixture the Borg had 46 other monster records, so the Maia's record was number 47, which is the race number of Grip, Farmer Maggot's Dog, still alive. On the next level the Borg's living-unique list lost Grip, the depth of its third-shallowest living unique jumped from 17 to 41, and when it read Word of Recall at depth 15 (maximum depth 60) it answered 'n' to 'Set recall depth to current depth?', so the next recall returns to depth 60. The corrected Borg marks Quaker (the race it saw), keeps Grip alive and answers 'y', as it does without the bug."
CONDITIONS="Native Human Warrior, seed 94, character level 31, depth 15, maximum and recall depth 60, a lit 38x38 room, one Scroll of Word of Recall. Fixture interventions: the character's unique kills are recorded as the game records them (r_info max_num = 0 and a lore kill, mon-util.c:1101, load.c:533-535): every unique shallower than depth 40 except Grip, Fang and Nár, plus Quaker; the Borg's map grids are marked on-screen/in view. Real code: borg_init(), the first borg_update() (whose new-level branch runs the unique scan), 46 stock wild dogs (4 in the control) each placed and observed with the production observe_kill_diff(); a stock Maia of Aulë placed and changed with the game's monster_change_shape() until it takes Quaker's shape (one try with this seed); its first sighting through observe_kill_diff() -> borg_new_kill() -> borg_update_kill_old(); a second borg_update() on a new level (borg.status.old_depth changed, as on arrival); the production borg_recall(); the Borg's queued answer is given to the game's own 'Set recall depth to current depth?' question while the real read-scroll command runs."
ISSUE=94
CARD_KEY='reset_recall'
CARD_LABEL="answers 'y' to 'Set recall depth to current depth?' (1 = yes, recall depth becomes 15)"
CASES=[('grip', "Maia in Quaker's shape becomes record 47 (Grip's race number)", "Original: the fix-up marks race 47 (Grip, alive) dead instead of Quaker; on the next level the Borg counts 56 living uniques instead of 57, its first living unique becomes Fang and its hunted depth 41 instead of 17; it answers 'n' and the recall depth stays 60. Corrected: Quaker is marked, Grip stays alive, hunted depth 17, the Borg answers 'y' and the game sets recall depth (and maximum depth) to 15.", False), ('slot5', 'Control: the same sighting becomes record 5 (a non-unique race)', "With only four earlier records the Maia's record is 5 (blubbering idiot, not a unique), so the misplaced mark has no visible effect; both versions keep Grip alive and answer 'y'. The changed line runs in both; the logs are byte-identical.", True)]
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
