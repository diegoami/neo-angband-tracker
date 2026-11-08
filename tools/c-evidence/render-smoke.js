// Generic headless render check for any replay page format.
// Loads the page, clicks every button and visits every <select> option,
// and fails on console/page errors or rendered text containing "undefined"
// or "NaN".  usage: node render-smoke.js replay.html
const { chromium } = require('playwright');
const path = require('path');
(async () => {
  const file = path.resolve(process.argv[2]);
  const browser = await chromium.launch();
  const page = await browser.newPage();
  const errors = [];
  page.on('console', m => { if (m.type() === 'error') errors.push(m.text()); });
  page.on('pageerror', e => errors.push(String(e)));
  await page.goto('file://' + file);
  let actions = 0;
  for (const sel of await page.locator('select').all()) {
    const values = await sel.locator('option').evaluateAll(os => os.map(o => o.value));
    for (const v of values) {
      await sel.selectOption(v); actions++;
      for (const b of await page.locator('button:visible').all()) {
        try { await b.click({ timeout: 1000 }); actions++; } catch (e) { /* hidden or detached */ }
      }
    }
  }
  if (!actions) {
    for (const b of await page.locator('button:visible').all()) {
      try { await b.click({ timeout: 1000 }); actions++; } catch (e) { /* ignore */ }
    }
  }
  const text = await page.locator('body').innerText();
  const bad = [];
  if (/\bundefined\b/.test(text)) bad.push('rendered text contains "undefined"');
  if (/\bNaN\b/.test(text)) bad.push('rendered text contains "NaN"');
  if (errors.length) bad.push('console errors: ' + errors.join('; '));
  await browser.close();
  console.log(`RENDER-SMOKE actions=${actions} errors=${errors.length} ${bad.length ? 'FAILED: ' + bad.join(' | ') : 'OK'}`);
  process.exitCode = bad.length ? 1 : 0;
})();
