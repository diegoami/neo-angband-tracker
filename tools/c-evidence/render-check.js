// Headless render check of a committed replay.html: every scenario panel,
// both decision cards populated from replay-data.json, no "undefined", no
// console errors, buttons reveal the cards and logs.
// usage: node render-check.js replay.html replay-data.json
const { chromium } = require('playwright');
const fs = require('fs');
(async () => {
  const [page_path, data_path] = process.argv.slice(2);
  const data = JSON.parse(fs.readFileSync(data_path, 'utf8'));
  const browser = await chromium.launch();
  const page = await browser.newPage();
  const errors = [];
  page.on('console', m => { if (m.type() === 'error') errors.push(m.text()); });
  page.on('pageerror', e => errors.push(String(e)));
  await page.goto('file://' + require('path').resolve(page_path));
  let checks = 0;
  const fail = (msg) => { console.error('RENDER FAIL: ' + msg); process.exitCode = 1; };
  for (const [mode, item] of Object.entries(data)) {
    await page.selectOption('#scenario', mode);
    const panel = page.locator(`.scenario[data-mode="${mode}"]`);
    if (!(await panel.isVisible())) fail(mode + ' panel hidden after select'); checks++;
    await panel.locator('.show-decision').click();
    await panel.locator('.show-log').click();
    const orig = (await panel.locator('.original .num').innerText()).trim();
    const corr = (await panel.locator('.corrected .num').innerText()).trim();
    if (orig !== String(item.before.value)) fail(`${mode} original ${orig} != ${item.before.value}`); checks++;
    if (corr !== String(item.after.value)) fail(`${mode} corrected ${corr} != ${item.after.value}`); checks++;
    if (!(await panel.locator('.decision').isVisible())) fail(mode + ' decision not shown'); checks++;
    if (!(await panel.locator('.logs').isVisible())) fail(mode + ' logs not shown'); checks++;
  }
  const body = await page.locator('main').innerText();
  if (body.includes('undefined')) fail('undefined in rendered text'); checks++;
  if (errors.length) fail('console errors: ' + errors.join('; ')); checks++;
  await browser.close();
  console.log(`RENDER checks=${checks} errors=${errors.length} ${process.exitCode ? 'FAILED' : 'OK'}`);
})();
