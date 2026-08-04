# Alloy (JavaScript) Guide

Alloy is Pebble's JavaScript framework: JS runs **on the watch** via the Moddable XS engine (ES6+/ES2025, strict mode, frozen primordials, no `eval`/`Function`). Marked by `"projectType": "moddable"` in package.json.

**Platform support: emery and gabbro ONLY.** Any other target platform requires C.

## When to Use Alloy vs C

| Use Alloy when... | Use C when... |
|---|---|
| Targeting only emery/gabbro | Targeting aplite/basalt/chalk/diorite/flint |
| Want `fetch`/`WebSocket`/`localStorage`/modern JS | Need HealthService, App Glances, timeline C APIs, background workers |
| Fast iteration on UI-heavy apps (Piu components) | Need maximum performance / tight memory control |
| Web-API-driven apps (networking is first-class) | Need MenuLayer/ActionBarLayer/system UI components (no Alloy equivalents — Piu builds UI from primitives) |
| Touchscreen interaction (emery/gabbro) | Games needing every frame of performance |

Hot paths in an Alloy app can call C via FFI (see below).

## Project Anatomy

```
project/
├── package.json              # "projectType": "moddable"
├── wscript                   # stock Pebble wscript — identical to C projects, never edit
├── src/
│   ├── c/mdbl.c              # boilerplate VM boot stub — never edit
│   ├── embeddedjs/
│   │   ├── main.js           # watch-side entry point (all watch logic)
│   │   ├── manifest.json     # Moddable manifest: modules, font resources, FFI
│   │   └── assets/           # custom .ttf fonts etc.
│   └── pkjs/
│       ├── index.js          # phone-side JS: pebbleproxy for networking, Clay
│       └── config.js         # Clay config (only if settings page)
```

Scaffold: `pebble new-project --alloy <name>` — or write files directly.

### package.json (Alloy essentials)

```json
{
  "name": "my-face",
  "author": "Author",
  "version": "1.0.0",
  "keywords": ["pebble-app"],
  "private": true,
  "dependencies": {},
  "pebble": {
    "displayName": "My Face",
    "uuid": "GENERATE-NEW-UUID",
    "projectType": "moddable",
    "sdkVersion": "3",
    "enableMultiJS": true,
    "targetPlatforms": ["emery"],
    "watchapp": { "watchface": true },
    "messageKeys": [],
    "resources": { "media": [] }
  }
}
```

- Watchapp: `"watchapp": { "watchface": false }`.
- Location sensor needs `"capabilities": ["location"]`; Clay settings need `"configurable"`.
- Networking: `"dependencies": { "@moddable/pebbleproxy": "^0.1.7" }` (install via `pebble package install @moddable/pebbleproxy`).
- Clay: `"@rebble/clay": "^1.0.8"`.

### src/c/mdbl.c (verbatim, never changes)

```c
#include <pebble.h>

int main(void) {
  Window *w = window_create();
  window_stack_push(w, true);

  moddable_createMachine(NULL);

  window_destroy(w);
}
```

### src/embeddedjs/manifest.json (minimal)

```json
{
    "include": ["$(MODDABLE)/examples/manifest_mod.json"],
    "modules": { "*": "./main.js" }
}
```

**Every extra JS module file must be registered** or you get module-not-found at runtime: `"modules": { "*": ["./main", "./math"] }`.

### wscript

Use the stock Pebble wscript (same as C projects — the one in the skill templates). Nothing Alloy-specific.

## Poco Rendering (procedural graphics)

```javascript
import Poco from "commodetto/Poco";
const render = new Poco(screen);          // `screen` is a global

render.width; render.height;              // emery: 200x228
render.unobstructed.width / .height;      // area not covered by Timeline Quick View

const white = render.makeColor(255, 255, 255);   // reuse color objects
const font = new render.Font("Gothic-Bold", 24); // system font

render.begin();                           // or begin(x, y, w, h) to clip = faster
render.fillRectangle(color, x, y, w, h);
render.drawRoundRect(x, y, w, h, color, radius);   // corners bitmask optional
render.frameRoundRect(x, y, w, h, color, radius);  // outline
render.drawLine(x1, y1, x2, y2, color, thickness);
render.drawCircle(color, cx, cy, radius, startAngle, endAngle); // degrees — FILLED pie slice, NOT a stroked arc
render.drawText(str, font, color, x, y);
const w = render.getTextWidth(str, font); // font.height for vertical layout
render.end();
```

**System fonts — name AND size must match a real system font. An invalid combination (e.g. `Bitham-Bold` at 48) builds fine but kills the JS VM at launch: white screen, app exits instantly, NO error in logs (only a "Heap Usage for App" exit line). White screen at launch = check your font sizes first.**

Valid system font sizes (from the Pebble system font set):

| Family | Valid sizes |
|---|---|
| Gothic-Regular / Gothic-Bold | 14, 18, 24, 28 |
| Bitham-Bold | 42 only |
| Bitham-Black | 30 only |
| Bitham-Light | 42 |
| Roboto-Condensed | 21 |
| Leco-Regular (numbers) | 20, 26, 28, 32, 36, 38, 42 |
| Droid-Serif | 28 |

For anything else, use a custom TTF (next section).

Bitmaps/vectors: `new Poco.PebbleBitmap(resourceId)` + `render.drawBitmap(bmp, x, y)`; PDC vector via `new Poco.PebbleDrawCommandImage(resourceId)` + `render.drawDCI(pdc, x, y)` with `pdc.clone()/rotate(rad, px, py)/scale(f)`; animated sequences via `Poco.PebbleDrawCommandSequence`.

Frame pattern: every redraw = `begin()` → fill full-screen background → draw → `end()`.

No stroked-circle/ring primitive. Ring (e.g. orbit line): draw filled circle in ring color, then punch out a smaller filled circle in the background color. Arc outline: same trick with `startAngle`/`endAngle` on both circles.

## Watchface Events

```javascript
watch.addEventListener("minutechange", e => draw(e.date));  // ALWAYS prefer over secondchange
watch.addEventListener("hourchange", e => refetch());
watch.addEventListener("connected", checkConnection);       // watch.connected.app boolean
watch.addEventListener("resize", drawScreen);               // Quick View appear/disappear
```

- **Time listeners (`minutechange`, `hourchange`) fire immediately on registration** — that IS the initial draw/fetch; do not add a separate startup call. `connected`/`resize` are NOT confirmed to fire immediately — read `watch.connected.app` directly at startup for initial state.
- Handlers may be invoked without an event (from your own code); use `const now = event?.date ?? lastDate;` pattern.
- Watchface Up/Down buttons reserved for timeline; use accelerometer taps for input.
- Quick View: clear background with `render.width/height`, position content with `render.unobstructed.*`, recompute layout inside the draw function, listen to `"resize"`.

## Custom Fonts

1. Put TTF in `src/embeddedjs/assets/`.
2. Declare in manifest.json:

```json
"resources": {
    "*-alpha": [
        { "source": "./assets/Jersey10-Regular", "size": 56, "monochrome": true, "blocks": ["Basic Latin"] }
    ]
}
```

3. Load in main.js:

```javascript
import parseBMF from "commodetto/parseBMF";
import parseRLE from "commodetto/parseRLE";
function getFont(name, size) {
    const font = parseBMF(new Resource(`${name}-${size}.fnt`));
    font.bitmap = parseRLE(new Resource(`${name}-${size}-alpha.bm4`));
    return font;
}
const timeFont = getFont("Jersey10-Regular", 56);
```

`"blocks": ["Basic Latin"]` subsets characters to save memory; `"monochrome": true` for crisp 1-bit rendering.

## Sensors & Input (ECMA-419 style)

```javascript
import Battery from "embedded:sensor/Battery";
const battery = new Battery({ onSample() { pct = this.sample().percent; draw(); } });
pct = battery.sample().percent;    // immediate read; keep instance open (do NOT close)

import Location from "embedded:sensor/Location";   // needs "location" capability + pebbleproxy
new Location({ onSample() {
    const s = this.sample();       // s.latitude, s.longitude
    this.close();                  // Location is ONE-SHOT: must close after read
    fetchWeather(s.latitude, s.longitude);
}});

import Accelerometer from "embedded:sensor/Accelerometer";
new Accelerometer({ onSample() {}, onTap(dir) {}, onDoubleTap(dir) {} }); // sample() → {x,y,z}

import Compass from "embedded:sensor/Compass";     // sample() → {heading} 0-360

import Button from "pebble/button";                 // WATCHAPPS ONLY
new Button({ types: ["select","up","down","back"], onPush(down, type) {} }); // down: 1=pressed 0=released

// Touch (emery/gabbro): feature-detect first
const hasTouch = device.sensor.Touch ? true : false;
new device.sensor.Touch({ onSample() {} });        // sample() → array of {x,y} or falsy
```

## Networking (proxied through phone)

Watch has no direct internet. Setup:

```bash
pebble package install @moddable/pebbleproxy
```

`src/pkjs/index.js`:

```javascript
const moddableProxy = require("@moddable/pebbleproxy");
Pebble.addEventListener('ready', moddableProxy.readyReceived);
Pebble.addEventListener('appmessage', moddableProxy.appMessageReceived);
```

Then watch-side gets standard web APIs:

```javascript
const url = new URL("https://api.open-meteo.com/v1/forecast");
url.search = new URLSearchParams({ latitude, longitude, current: "temperature_2m,weather_code" });
const response = await fetch(url);      // response.ok, .status, .json(), .text()
const data = await response.json();
```

- Wrap in try/catch; requires connected phone with internet.
- Wait for `watch.connected.pebblekit === true` before fetch/WebSocket. If not connected yet, retry in ~1s rather than failing.
- **First fetch after launch can fail transiently even when connected.** Always auto-retry (~10s) on failure — a face that only refetches on `hourchange` would otherwise pin "NO LINK" on screen for an hour.
- `WebSocket` also available; low-level `embedded:network/http/client` / `websocket/client` for streaming.

### Known fetch() crash + robust fallback

`fetch()` can hard-crash the XS VM intermittently: the pebbleproxy http client splits proxied header lines on `":"`, and a colon-less line yields `undefined`, which `Headers.prototype.set` calls `.toString()` on → `TypeError: cannot coerce undefined to object` → `fxAbort`. **Uncatchable from app code** (fires inside the transport callback); app exits at fetch time with that TypeError in logs.

If you hit it (or want immunity for a production app), bypass fetch with the raw client — `headersMask` avoids the broken header path:

```javascript
let client = null;
function httpGetJSON(host, path, onSuccess, onFail) {
    let watchdog = setTimeout(() => fail("timeout"), 20000);  // fail(), not finish(): must close the stuck client so retry gets a fresh one
    const chunks = [];
    let httpStatus = 0;
    function finish(cb, arg) {
        if (!watchdog) return;
        clearTimeout(watchdog); watchdog = null;
        cb(arg);
    }
    function fail(e) {
        try { client?.close(); } catch (_) {}
        client = null;
        finish(onFail, e);
    }
    try {
        client ??= new device.network.https.io({
            ...device.network.https, host, port: 443,
            onError(e) { fail(e); }
        });
        client.request({
            path,
            headersMask: ["content-length"],
            onHeaders(status) { httpStatus = status; },
            onReadable(count) { if (count) chunks.push(String.fromArrayBuffer(this.read())); },
            onDone() {
                if (httpStatus < 200 || httpStatus > 299) return finish(onFail, "http " + httpStatus);
                try { finish(onSuccess, JSON.parse(chunks.join(""))); } catch (e) { finish(onFail, e); }
            },
            onError(e) { fail(e); }
        });
    } catch (e) { fail(e); }
}
```

Still requires the pebbleproxy pkjs shim; only the watch-side API changes.

## Storage

- `localStorage` (strings only): `JSON.stringify`/`parse`, merge defaults via spread `{...DEFAULTS, ...JSON.parse(stored)}`, try/catch the parse.
- `device.keyValue.open({path, format})` → `write/read/delete/close`.
- `device.files.openFile({path, mode, size})` for binary files.

## Settings (Clay) + AppMessage

pkjs side: standard Clay (`@rebble/clay` + config.js). Watch side:

```javascript
import Message from "pebble/message";
const message = new Message({
    keys: ["BackgroundColor", "ShowDate"],   // must match package.json messageKeys AND config.js messageKey
    onReadable() {
        const msg = this.read();
        const bg = msg.get("BackgroundColor");   // colors: 24-bit int → (bg>>16)&0xFF, (bg>>8)&0xFF, bg&0xFF
        const show = msg.get("ShowDate");        // toggles: 0/1
    }
});
message.write(new Map([["COMMAND", 1]]));        // send to phone
```

Coexisting with pebbleproxy in pkjs: `if (moddableProxy.appMessageReceived(e)) return;`

## Other Services

```javascript
import Vibes from "pebble/vibes";     // shortPulse/longPulse/doublePulse/pattern([ms,...])/cancel
import WakeUp from "pebble/wakeup";   // schedule(timeMs, cookie, notifyIfMissed) → id; watch.wake at launch
import Dictation from "pebble/dictation";  // needs phone+internet
watch.light(true|false);              // backlight; watch.light() = auto
// Device info: watch.model, watch.firmwareVersion, watch.hour12, watch.launch
// Screen: screen.width/height/round/color
```

## Timers & Animation

- `setInterval`/`setTimeout`/`setImmediate` work. Animation loop: `setInterval(draw, 33)` ≈ 30fps (17ms ≈ 60fps costs battery). Stop timers when done.
- Piu Timeline for tweens: `import Timeline from "piu/Timeline"` + easing on `Math.*Ease*` (quadEaseOut, bounceEaseOut, etc.). UI transitions 200–500ms.

## Piu (declarative UI — alternative to Poco)

`import {} from "piu/MC";` gives Application/Content/Label/Text/Container/Column/Row/Skin/Style/Behavior globals. Use for component-based watchapp UIs; use Poco for pixel control. Port bridges both (`onDraw` callback inside Piu layout). See https://developer.repebble.com/guides/alloy/piu-guide/ if needed — Poco is simpler and covers most watchface/game cases.

## FFI (call C from JS)

C file next to mdbl.c; declare in manifest.json `"ffi": {"sources": [...], "functions": {"add": {"arguments": ["int32_t","int32_t"], "returns": "int32_t"}}}`; call via global `Natives.add(2, 3)`. Requires mdbl.c modification to pass `.fxBuildFFI` — only for hot paths.

## Build & Test

```bash
pebble build
pebble install --emulator emery
pebble logs --emulator emery              # console.log output
pebble screenshot --no-open --emulator emery shot.png
pebble emu-battery --percent 30
pebble emu-bt-connection --connected no
pebble emu-set-timeline-quick-view on
pebble emu-app-config                     # open Clay page
pebble package install <pkg>              # npm-style pebble packages
```

## Gotchas Checklist

1. Alloy = emery/gabbro only. Check targetPlatforms.
2. `projectType: "moddable"` required; `watchapp.watchface: true` for faces.
3. `minutechange` not `secondchange` (battery).
4. Time events fire immediately on registration — no separate initial draw.
5. Extra JS modules must be in manifest.json `modules`.
6. Custom fonts need manifest `resources["*-alpha"]` declaration + parseBMF/parseRLE load with exact `Name-Size.fnt`/`Name-Size-alpha.bm4` names.
7. `Location` one-shot → `close()`; `Battery` stays open.
8. `fetch` requires pebbleproxy in pkjs + connected phone; wait for `watch.connected.pebblekit`.
9. Redraw handlers called without event → `event?.date ?? lastDate`.
10. `| 0` to truncate float math to ints (perf).
11. Quick View: clear with `render.width/height`, position with `render.unobstructed.*`, handle `"resize"`.
12. Clay keys must match in 3 places: package.json messageKeys, config.js messageKey, watch Message keys.
13. localStorage = strings only; spread-merge defaults.
14. No eval/Function; primordials frozen; strict mode always.
15. Emery vs gabbro: branch on `screen.round` or `render.unobstructed.height`.
