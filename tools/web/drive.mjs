// Headless browser verification + scripted play.
// Usage: node tools/web/drive.mjs <page.html> <outDir> "<script>"
//   script = semicolon-separated steps:  wait:MS | shot:NAME | key:CODE:MS | hold:CODE+CODE:MS
import { chromium } from 'playwright';
import path from 'node:path';
import fs from 'node:fs';

const [page_, outDir, plan = 'wait:1500;shot:boot'] = process.argv.slice(2);
fs.mkdirSync(outDir, { recursive: true });

const browser = await chromium.launch({
  executablePath: process.env.PW_CHROME || '/opt/pw-browsers/chromium-1194/chrome-linux/chrome',
  args: ['--use-gl=swiftshader', '--enable-unsafe-swiftshader', '--no-sandbox',
         '--disable-dev-shm-usage', '--hide-scrollbars'],
});
const ctx = await browser.newContext({ viewport: { width: 1280, height: 720 }, deviceScaleFactor: 1 });
const pg = await ctx.newPage();
const logs = [];
pg.on('console', m => logs.push(`[${m.type()}] ${m.text()}`));
pg.on('pageerror', e => logs.push(`[pageerror] ${e.message}`));

await pg.goto('file://' + path.resolve(page_));
try { await pg.waitForFunction('window.__ready === true', { timeout: 30000 }); }
catch { console.error('MODULE NEVER BECAME READY'); }
await pg.click('canvas').catch(()=>{});

const held = new Set();
for (const step of plan.split(';').map(s => s.trim()).filter(Boolean)) {
  const [op, a, b] = step.split(':');
  if (op === 'wait') await pg.waitForTimeout(+a);
  else if (op === 'shot') await pg.screenshot({ path: path.join(outDir, `${a}.png`) });
  else if (op === 'key') { await pg.keyboard.down(a); await pg.waitForTimeout(+(b||100)); await pg.keyboard.up(a); }
  else if (op === 'hold') {
    const keys = a.split('+');
    for (const k of keys) { await pg.keyboard.down(k); held.add(k); }
    await pg.waitForTimeout(+(b||100));
    for (const k of keys) { await pg.keyboard.up(k); held.delete(k); }
  }
  else if (op === 'down') { await pg.keyboard.down(a); held.add(a); }
  else if (op === 'up')   { await pg.keyboard.up(a);   held.delete(a); }
}
for (const k of held) await pg.keyboard.up(k).catch(()=>{});

console.log('--- console ---');
console.log(logs.filter(l => !/fonts\.googleapis|ERR_CONNECTION_RESET/.test(l)).join('\n') || '(clean)');
await browser.close();
