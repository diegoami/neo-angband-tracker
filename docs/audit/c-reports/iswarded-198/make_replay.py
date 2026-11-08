import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/iswarded-198'
TITLE='The Borg crashed reading a rune message after Word of Recall'
INTRO="When a monster breaks one of the Borg's Runes of Protection, or hits the Borg while it is tracking runes, the Borg forgets its rune list and checks whether a rune is still under its own position. If Word of Recall fires in the same game turn, the game shows that message with -more- just before it leaves the level. The Borg stores the line and parses it at its next keypress, which is the first prompt on the new level, before its map update has moved its own position there. Its position is still the old level's grid, (113,48) here, which lies outside the 66x22 town, and the rune check reads outside the town's map. In a normal debug build the game aborts on an assertion; in a release-style build (asserts off) AddressSanitizer reports a heap-buffer-overflow read. The correction skips the rune check when the position is not on the current level; the rune list is still cleared, and the Borg goes on to make its first town move."
CONDITIONS="Native Human Warrior, seed 198, character level 30, depth 40, on a full-size open level (198x66), standard 80x24 screen. Fixture interventions: the arena and the runes the Borg laid earlier (placed with the Borg's own rune records, as its rune-reading code writes them); an awake skeleton human (level 12, one blow, no spells, never afraid); Word of Recall set to fire on the next world tick; Escape answers at the old-level prompts between monster rounds; the EVENT_MESSAGE_FLUSH signal and the panel recentring that the game's static leave-level and new-level display code performs. Real code: monster turns (process_monsters), the recall (process_world), the game's own message line and -more- prompt (display_message and message_flush from ui-input.c), town generation (prepare_next_level) and the Borg's keypress hook, including its think and map update at the first town prompt. rune-*: a 5x5 Sea of Runes with the Borg on the centre rune; the monster breaks an outer rune. hit-*: one rune west of the Borg; the monster hits the Borg. -town: old position (113,48), outside the town; -inside: old position (30,10), also a town grid. The AddressSanitizer variant builds with -DNDEBUG (as a release build does), so the assertion that stops a debug build does not mask the read."
ISSUE=198
CARD_KEY='survived'
CARD_LABEL='first new-level keypress completed (1); n/a means the run aborted before it finished'
CASES=[('rune-town', 'Sea of Runes, rune broken, recall to town', "Line stored at -more-: 'The rune of protection is broken! You feel yourself yanked upwards!'. Original: at the first town prompt the parse checks the old grid (113,48) in the 66x22 town: assertion failure in square_istrap (debug), heap-buffer-overflow read (AddressSanitizer). Corrected: the rune list is cleared (25 to 0), the check is skipped, the map update moves the Borg to its town grid and it chooses a move (key 50).", False), ('hit-town', 'Hit while tracking a rune, recall to town', "Line stored at -more-: 'The skeleton human hits you. You feel yourself yanked upwards!'. Original: the hit-by branch checks the old grid (113,48) in the town and aborts the same way. Corrected: the rune list is cleared (1 to 0) and the Borg chooses a town move (key 51).", False), ('rune-inside', 'Control: rune broken, old grid inside the town', 'Same sequence from old grid (30,10), which exists in the town: the check reads a real town grid (no rune) in both versions; byte-identical logs.', True), ('hit-inside', 'Control: hit while tracking a rune, old grid inside the town', 'Same hit sequence from old grid (30,10): the hit-by branch runs the guarded check on a real town grid in both versions; byte-identical logs.', True), ('hit-noglyph', 'Control: hit with no rune tracked', 'Same hit sequence from (113,48) with no rune tracked: the hit-by branch skips the rune check in both versions; byte-identical logs. A hit alone never reaches the read.', True)]
for mode,label,explanation,control in CASES:
 item={'label':label,'explanation':explanation,'control':control}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':SUITE+' finished: 1/1 passed' in raw}
  for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
   for match in re.findall(re.escape(tag)+r'(.*)',raw):
    entry.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
  notes=[l for l in raw.splitlines() if l.startswith(('SCREEN ','STATE ','DECISION ','OUTCOME ')) or 'Assertion' in l]
  entry['notes']=notes
  entry['value']=entry.get('outcome',{}).get(CARD_KEY,entry.get('state',{}).get(CARD_KEY,'n/a'))
  asan=root/f'asan-{phase}-{mode}.log'
  entry['asan_raw']=asan.read_text() if asan.exists() else ''
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
  '</div><div class="logs hidden"><h3>Original</h3><pre>'+esc(b['raw'])+'</pre><h3>Corrected</h3><pre>'+esc(a['raw'])+'</pre>'
  '<h3>Original, AddressSanitizer build</h3><pre>'+esc(b['asan_raw'])+'</pre><h3>Corrected, AddressSanitizer build</h3><pre>'+esc(a['asan_raw'])+'</pre></div></section>')
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
