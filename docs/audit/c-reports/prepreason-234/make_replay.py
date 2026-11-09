import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/prepreason-234'
TITLE="The next depth's reason hid 'Collect from house'"
INTRO="When the Borg decides whether to leave a level, it asks borg_prepared() why it is not ready for the current depth and for the next one. Every formatted reason is written into the same static text buffer, so asking about the next depth overwrote the current depth's answer before it was used. At depth 99 with Potions of *Healing* waiting at home, the current depth's reason is 'Collect from house (20 potions).', which should send the Borg back to town to fetch them. With borg_no_deeper set to 100 (stop before Morgoth's level), the next depth's reason is 'No deeper 100.', and that text replaced it: the Borg printed the wrong reason, did not decide to collect, and walked to an up staircase instead of reading Word of Recall. The correction copies the current depth's reason before the next call; the Borg then recalls to town to collect."
CONDITIONS="Native Human Warrior, seed 234, character level 50, maximum depth 99, at depth 99 ('nodeeper100', 'default127') or 97 ('prepared97'), in a lit 40x24 arena with one up staircase 25 steps east that the Borg has seen. Endgame kit from real game items with every rune known: 'Bladeturner' (all resists, blind, confusion, hold life), 'Dal-i-thalion' (free action, +5 speed), an Amulet of ESP, two Rings of Speed, a Mace of Disruption, a Lantern; carried: food, oil, Word of Recall x4, Teleportation x8, Teleport Level x3, Phase Door x10, Healing x5, Speed x15, and *Healing* x15 (x30 in 'prepared97'). The home holds 20 Potions of *Healing*. Fixture interventions, disclosed: STR, DEX and CON set to 18/100; the Borg's clock set so it has just arrived on the level and was last in town 5000 game turns ago; borg_cfg[BORG_NO_DEEPER] set to 100 (the same as the borg.txt line 'borg_no_deeper = 100') except in 'default127', which keeps the profile's 127. The production borg_leave_level(false) runs once; its notes, goal and first queued keys are read back. This isolates the leave-level decision; it does not play the recall or the visit home."
ISSUE=234
CARD_KEY='collect'
CARD_LABEL="decided to collect from the house (1 = 'Returning to town to Collect stock')"
CASES=[('nodeeper100', "Depth 99, borg_no_deeper = 100: current 'Collect from house', next 'No deeper 100.'", "Original: the depth-100 call overwrites the buffer, so the notes say 'not prep for current level: No deeper 100.', the 'Collect from house' test fails, and the Borg flows toward the up staircase (key 6). Corrected: the notes give 'Collect from house (20 potions).', the Borg decides 'Returning to town to Collect stock.' and reads Word of Recall.", False), ('default127', "Control: depth 99, borg_no_deeper = 127: both depths say 'Collect from house'", 'The next depth gives the same text, so the overwrite changes nothing; both versions decide to collect and read Word of Recall, and the logs are identical.', True), ('prepared97', 'Control: depth 97 with 30 *Healing* carried: current depth prepared', 'No reason for the current depth; neither version takes the collect branch (both power-dive), and the logs are identical.', True)]
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
