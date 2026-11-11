// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Fırat Türker Ircı and contributors
//
// render.js - draw the Android launcher icon (FEMM with a copper wire wound
// around the name) and write the adaptive-icon layers into
// android/package/res/:
//
//   mipmap-<dpi>/ic_launcher_foreground.png        light theme (black name)
//   mipmap-night-<dpi>/ic_launcher_foreground.png  dark theme (cream name)
//   mipmap-<dpi>/ic_launcher_monochrome.png        Android 13 themed icons
//
// The background colours are in values(-night)/femm_icon.xml. Uses the
// Playwright Chromium (and Google Fonts for the Barlow face):
//
//     node tools/app_icon/render.js
const path = require('path');
const fs = require('fs');
const { chromium } = require('playwright');

const ROOT = path.resolve(__dirname, '..', '..');
const RES = path.join(ROOT, 'android', 'package', 'res');
// Adaptive icon layers are 108 dp; the launcher shows the middle 72 dp and
// may cut it to a circle of 66 dp, so the artwork stays inside that circle.
const DENSITIES = { mdpi: 1, hdpi: 1.5, xhdpi: 2, xxhdpi: 3, xxxhdpi: 4 };

// A loosely wound coil along the name, seen a little from the side: a helix
// x = a*t + k*R*sin t, y = cy + R*cos t, with depth z = R*sin t. Where z > 0
// the wire passes in front of the letters, elsewhere behind them.
function coil({ x0, x1, cy, R, turns, k }) {
  const t0 = 0.55 * Math.PI, t1 = t0 + 2 * Math.PI * turns;
  const a = (x1 - x0 - 2 * k * R) / (t1 - t0);
  const n = turns * 160, pts = [];
  for (let i = 0; i <= n; i++) {
    const t = t0 + (t1 - t0) * i / n;
    pts.push({ x: x0 + k * R + a * (t - t0) + k * R * Math.sin(t), y: cy + R * Math.cos(t), f: Math.sin(t) > 0 });
  }
  const segs = { front: [], back: [] };
  let cur = null, side = null;
  for (const q of pts) {
    const sd = q.f ? 'front' : 'back';
    if (sd !== side) { if (cur) segs[side].push(cur); cur = [q]; side = sd; } else cur.push(q);
  }
  segs[side].push(cur);
  const d = list => list.map(s => 'M' + s.map(q => `${q.x.toFixed(2)} ${q.y.toFixed(2)}`).join(' L')).join(' ');
  return { front: d(segs.front), back: d(segs.back) };
}

// The artwork on a 512 grid, then placed inside the 66/108 safe circle.
function svg(theme) {
  const C = coil({ x0: 34, x1: 478, cy: 256, R: 128, turns: 4, k: 0.42 });
  const ink = theme === 'dark' ? '#f6f4e4' : theme === 'mono' ? '#ffffff' : '#121212';
  const front = theme === 'mono' ? '#ffffff' : '#ff9a2a';
  const back = theme === 'mono' ? '#ffffff' : '#c9711a';
  const line = (d, c) => `<path d="${d}" fill="none" stroke="${c}" stroke-width="16" stroke-linecap="round" stroke-linejoin="round"/>`;
  // 512 units across the 66 dp circle's width, centred in 108 dp.
  const s = 64 / 512, off = (108 - 64) / 2;
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 108 108" width="100%" height="100%">
    <g transform="translate(${off} ${off}) scale(${s})">
      ${line(C.back, back)}
      <text x="256" y="304" font-family="Barlow" font-weight="800" font-size="138"
            letter-spacing="4" text-anchor="middle" fill="${ink}">FEMM</text>
      ${line(C.front, front)}
    </g></svg>`;
}

(async () => {
  const browser = await chromium.launch();
  const page = await browser.newPage();
  await page.setContent(`<html><head>
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Barlow:wght@800&display=block">
    <style>html,body{margin:0;background:transparent}#c{width:432px;height:432px}</style></head>
    <body><div id="c"></div></body></html>`);
  await page.evaluate(() => document.fonts.load('800 100px Barlow'));
  const ok = await page.evaluate(() => document.fonts.check('800 100px Barlow'));
  if (!ok) throw new Error('the Barlow font did not load');
  for (const [theme, dirSuffix, file] of [['light', '', 'ic_launcher_foreground'],
                                          ['dark', '-night', 'ic_launcher_foreground'],
                                          ['mono', '', 'ic_launcher_monochrome']]) {
    for (const [dpi, scale] of Object.entries(DENSITIES)) {
      const px = Math.round(108 * scale);
      await page.evaluate(([html, px]) => {
        const c = document.getElementById('c');
        c.style.width = c.style.height = px + 'px';
        c.innerHTML = html;
      }, [svg(theme), px]);
      const dir = path.join(RES, `mipmap${dirSuffix}-${dpi}`);
      fs.mkdirSync(dir, { recursive: true });
      await page.locator('#c').screenshot({ path: path.join(dir, file + '.png'), omitBackground: true });
    }
  }
  await browser.close();
  console.log('icons written to', RES);
})();
