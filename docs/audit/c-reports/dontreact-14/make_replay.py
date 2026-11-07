import json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
CASES={
 'main':('Knockout, then recovery',"A Warrior is knocked out (\"You have been knocked out.\"), then recovers. The production pump clears the ignore flag. The original keeps ignoring all combat messages forever after the knockout: the recovery message queues nothing (buffer count stays 1). The corrected version, using the borg's own dont_react field (cleared by the pump, as upstream intended), resumes reacting: the recovery message queues (count 2).")}
for mode,(label,explanation) in CASES.items():
 item={'label':label,'explanation':explanation}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':'native-c/dontreact-14 finished: 1/1 passed' in raw}
  for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
   for match in re.findall(re.escape(tag)+r'(.*)',raw):
    entry.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
  item[phase]=entry
 data[mode]=item
CSS=("body{margin:0;background:#f4f1ea;color:#253a3c;font:18px/1.6 system-ui}"
 "main{max-width:1100px;margin:auto;padding:32px 24px}"
 "h1{font-size:40px;line-height:1.2}h2{font-size:24px}"
 ".cards{display:grid;grid-template-columns:1fr 1fr;gap:20px}"
 ".card{background:white;border:1px solid #cbd6d0;border-radius:12px;padding:24px;margin:18px 0}"
 ".cards .card{border-top:6px solid #648e80}.num{font-size:34px;font-weight:750}"
 ".summary{background:#e6eee6;padding:20px;border-left:5px solid #377367}"
 "pre{white-space:pre-wrap;overflow-wrap:anywhere;font:13px/1.5 monospace;max-height:330px;overflow:auto}"
 "details{background:white;padding:20px;margin:20px 0}summary{font-weight:700;cursor:pointer}"
 "@media(max-width:650px){.cards{grid-template-columns:1fr}h1{font-size:30px}}")
d=data['main']
html=('<!doctype html><html lang="en"><meta charset="utf-8">'
 '<meta name="viewport" content="width=device-width,initial-scale=1">'
 '<title>The borg stayed deaf after knockout recovery</title>'
 '<style>'+CSS+'</style><main>'
 '<p>Recorded native message-queue states</p>'
 '<h1>The borg stayed deaf after knockout recovery</h1>'
 '<p>When the message "You have been knocked out." arrives, the Borg sets a flag that gates every further reaction; no combat message is processed while it is up. The flag was a static copy that nothing ever cleared, so after the knockout recovery the Borg remained deaf to combat messages for the rest of the session. Upstream intended the flag to be the Borg&#8217;s own dont_react field, which the production pump clears on every pass. The correction renames the copy to that field, and the Borg resumes reacting after recovery.</p>'
 '<div class="cards">'
 '<section id="original" class="card"><h2>Original</h2><div class="num">'
 + str(d['before']['state']['after_recovery']) +
 '</div><p class="state">Messages queued after recovery (baseline 1)</p></section>'
 '<section id="compared" class="card"><h2>Corrected</h2><div class="num">'
 + str(d['after']['state']['after_recovery']) +
 '</div><p class="state">Messages queued after recovery (baseline 1)</p></section>'
 '</div>'
 '<p class="summary">The original queues nothing after recovery (stays deaf); the corrected borg resumes reacting.</p>'
 '<h2>What the recorded run establishes</h2>'
 '<p>Baseline reaction queues 1 message; during the knockout the queue is gated (both versions; that gating is intended); after the recovery pump the original still gates (1) while the corrected borg queues the next message (2). The during-knockout gating is identical in both versions, so the correction changes only the post-recovery behaviour.</p>'
 '<details><summary>Experiment conditions and limits</summary><p>Native Human Warrior, seed 1000. The knockout message is parsed through the production parse/flush path; the pump replays the production sequence (borg_parse(NULL); borg.dont_react = false). Queued-message counts are read back from the borg&#8217;s message buffer. This isolates the flag lifecycle, not full autonomous play.</p></details>'
 '<details><summary>Recorded native output</summary><h3>Original</h3><pre id="original-log"></pre>'
 '<h3>Corrected</h3><pre id="compared-log"></pre></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/14">issue14</a>. Upstream counterpart: angband/angband#6808.</p>'
 '</main><script>'
 'document.getElementById("original-log").textContent='+json.dumps(d['before']['raw'])+';'
 'document.getElementById("compared-log").textContent='+json.dumps(d['after']['raw'])+';'
 '</script></html>')
(root/'replay.html').write_text(html)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
