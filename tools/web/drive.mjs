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

// The artifact page is a fragment: the host supplies the doctype and <meta charset=utf8>.
// Loaded bare from file://, Chrome sniffs the encoding, and on some builds it guesses
// wrong -- the wasm is a binary-coded string, so one mis-decoded byte and the module
// fails to instantiate. Give it what the host gives it.
let target = path.resolve(page_);
const src = fs.readFileSync(target);            // bytes, not text: the wasm string must not be re-decoded here
if (!/^\s*<!doctype/i.test(src.subarray(0, 64).toString('latin1'))) {
  target = path.resolve(outDir, 'page.html');
  fs.writeFileSync(target, Buffer.concat([Buffer.from('<!doctype html><meta charset="utf-8">\n'), src]));
}
await pg.goto('file://' + target);
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
  else if (op === 'audio') {
    // miniaudio's device list: is the context running, and is it producing callbacks?
    const st = await pg.evaluate(() => {
      const m = window.miniaudio; if (!m || !m.devices) return 'no miniaudio';
      return m.devices.filter(Boolean).map(d => (d.webaudio ? d.webaudio.state : 'no ctx') + '/' + (d.webaudio ? d.webaudio.sampleRate : '-')).join(',') || 'no devices';
    });
    console.log('AUDIO ' + a + ': ' + st);
  }
  else if (op === 'down') { await pg.keyboard.down(a); held.add(a); }
  else if (op === 'up')   { await pg.keyboard.up(a);   held.delete(a); }
}
for (const k of held) await pg.keyboard.up(k).catch(()=>{});

console.log('--- console ---');
console.log(logs.filter(l => !/fonts\.googleapis|ERR_CONNECTION_RESET/.test(l)).join('\n') || '(clean)');
await browser.close();
