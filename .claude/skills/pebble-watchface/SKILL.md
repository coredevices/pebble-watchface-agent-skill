---
name: pebble-watchface
description: Generate complete Pebble smartwatch watchfaces, build PBW artifacts, and test in QEMU emulator. Use when creating watchfaces, Pebble apps, animated displays, clock faces. Produces ready-to-install PBW files and runs them in emulator.
---

# Pebble Watchface Generator

Generate complete, buildable Pebble watchfaces with full PBW artifact output and QEMU testing.

**Default target platform: Emery (Pebble Time 2, 200x228 color rectangular display).**

## CRITICAL: End-to-End Delivery

This skill MUST produce a final `.pbw` file, test it, AND visually verify it looks correct.

Every watchface request follows this complete flow:

1. **Research** → [SUBAGENT] Gather requirements, study samples and tutorials
2. **Design** → [SUBAGENT] Plan architecture and visuals
3. **Implement** → Write ALL project files
4. **Build** → Run `pebble build` to generate PBW
5. **Test** → Run in QEMU, capture screenshot, visually verify with Read tool
6. **Iterate** → Fix issues until screenshot looks good
7. **Generate Assets** → Capture screenshots and rollover GIFs using `pebble screenshot`
8. **Deliver** → Report PBW location with verified screenshots and GIFs
9. **Publish** → (Optional) Publish to Pebble App Store with `pebble publish`

**Never stop until:**
- PBW is built successfully
- Screenshots captured from QEMU for emery platform
- Visual verification confirms it looks correct
- User receives the final artifacts

## CRITICAL: Battery Efficiency

**ALWAYS use `MINUTE_UNIT` for `tick_timer_service_subscribe()`.** NEVER use `SECOND_UNIT` unless the user explicitly requests a seconds display. `SECOND_UNIT` causes the watchface to redraw every second, which drastically reduces battery life. Design all watchfaces to update on minute boundaries.

For animated watchfaces that use `app_timer_register()`, only run animations briefly (e.g., on a tap event or for a few seconds after minute change), then stop the timer. Continuous animation is acceptable only when the user explicitly requests it.

---

## Platform Reference

| Platform | Model | Resolution | Shape | Colors |
|----------|-------|------------|-------|--------|
| **emery** | **Pebble Time 2** | **200x228** | **Rect** | **64-color** |
| gabbro | Pebble Round 2 | 260x260 | Round | 64-color |
| basalt | Pebble Time | 144x168 | Rect | 64-color |
| chalk | Pebble Time Round | 180x180 | Round | 64-color |
| aplite | Pebble Classic | 144x168 | Rect | B&W |
| diorite | Pebble 2 | 144x168 | Rect | B&W |
| flint | Pebble 2 Duo | 144x168 | Rect | 64-color |

**Emery is the default and exclusive target.** If the user wants gabbro (round) support, that should be a second pass after the emery version is finalized.

---

## Phase 1: Research [USE SUBAGENT]

**Spawn a research subagent** using Agent tool with `subagent_type: "Explore"` to:

### Gather Requirements
Ask the user (use AskUserQuestion if unclear):
- **Type**: Digital, analog, animated, or artistic?
- **Elements**: Time, date, battery, weather, custom graphics?
- **Animation**: Static, subtle, or complex animations?
- **Weather/Web data**: Does it need weather or other internet data?

### Study Existing Code
The subagent should read and analyze:
- `samples/aqua-pbw/src/c/main.c` — animated watchface patterns
- `tutorials/c-watchface-tutorial/part1/` — basic time + date
- `tutorials/c-watchface-tutorial/part4/` — weather via AppMessage + pkjs

Key patterns to extract:
- Data structures for animated elements
- Animation loop structure
- Drawing functions
- Memory management patterns
- Battery-aware throttling
- Weather/AppMessage communication (if needed)

Also have subagent read relevant reference docs:
- `reference/pebble-api-reference.md`
- `reference/animation-patterns.md`
- `reference/drawing-guide.md`

---

## Phase 2: Design [USE SUBAGENT]

**Spawn a planning subagent** using Agent tool with `subagent_type: "Plan"` to:

### Create Design Specification
- Screen layout for **emery (200x228)** rectangular display
- Element positions and sizes
- Animation behavior and timing (intervals, speeds)
- Color scheme (64-color palette)
- Data structures needed
- Layer hierarchy
- Whether weather/web data is needed (requires pkjs)

### CRITICAL: Layout Planning to Prevent Cropping
**You MUST calculate exact pixel positions to ensure nothing is cropped.**

For emery (200x228):
```
Available space: X: 0-199, Y: 0-227
Safe margins: 2-5 pixels from edges
```

For each visual element, calculate:
1. **Y position**: Where does it start vertically?
2. **Element height**: How tall is it?
3. **Bottom edge**: Y_position + height must be < 228 (with margin)

Example layout calculation for emery:
```
SCREEN_WIDTH = 200, SCREEN_HEIGHT = 228
Time text:     Y=60,  height=50  → bottom at 110 ✓
Date text:     Y=115, height=26  → bottom at 141 ✓
Weather:       Y=145, height=24  → bottom at 169 ✓
Battery bar:   Y=0,   height=3   → bottom at 3   ✓
```

**FAIL CONDITIONS to check in design:**
- Element bottom edge >= SCREEN_HEIGHT (228 for emery)
- Element right edge >= SCREEN_WIDTH (200 for emery)
- GPath points with negative offsets that extend beyond anchor point
- Elements positioned relative to SCREEN_HEIGHT without accounting for element size

### GPath Positioning Guide
GPaths use **relative coordinates from an anchor point**. Calculate carefully:

```c
// GPath points are RELATIVE to where you move_to
static GPoint castle_points[] = {
    {-35, 0},    // 35px LEFT of anchor, AT anchor Y
    {-35, -40},  // 35px left, 40px ABOVE anchor
    {35, 0},     // 35px RIGHT of anchor
};

// Anchor positioning calculation:
// If castle_points go from Y=0 to Y=-40 (40px tall, extending UP)
// And you want bottom of castle at Y=223 (5px margin from 228)
// Then anchor Y = 223 (the base of the castle)
gpath_move_to(castle_path, GPoint(SCREEN_WIDTH/2, 223));
```

### Architecture Planning
- What structs are needed?
- How many animated elements?
- Update interval (MINUTE_UNIT for tick, 50ms for brief animations)
- Memory pre-allocation strategy
- Does it need pkjs for weather/web data?

---

## Phase 3: Implementation

**Do this directly** (not a subagent) — write all files:

### Create Project Directory
```bash
mkdir -p /path/to/watchface-name/src/c
mkdir -p /path/to/watchface-name/resources
```

### Write ALL Required Files

**1. package.json** (REQUIRED)
```json
{
  "name": "watchface-name",
  "author": "Author Name",
  "version": "1.0.0",
  "keywords": ["pebble-app"],
  "private": true,
  "dependencies": {},
  "pebble": {
    "displayName": "Watchface Display Name",
    "uuid": "GENERATE-NEW-UUID-HERE",
    "sdkVersion": "3",
    "enableMultiJS": true,
    "targetPlatforms": ["emery"],
    "watchapp": { "watchface": true },
    "resources": { "media": [] }
  }
}
```
Generate UUID: `python3 -c "import uuid; print(uuid.uuid4())"`

**If weather/web data is needed**, add to the `pebble` section:
```json
{
  "capabilities": ["location"],
  "messageKeys": ["TEMPERATURE", "CONDITIONS", "REQUEST_WEATHER"]
}
```

**2. wscript** (REQUIRED)
```python
import os.path

top = '.'
out = 'build'

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

def build(ctx):
    ctx.load('pebble_sdk')
    build_worker = os.path.exists('worker_src')
    binaries = []
    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/c/**/*.c'),
                      target=app_elf, bin_type='app')
        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform, 'app_elf': app_elf,
                             'worker_elf': worker_elf})
            ctx.pbl_build(source=ctx.path.ant_glob('worker_src/c/**/*.c'),
                          target=worker_elf, bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env
    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ant_glob(['src/pkjs/**/*.js',
                                         'src/pkjs/**/*.json']),
                   js_entry_file='src/pkjs/index.js')
```

**3. src/c/main.c** (REQUIRED)
Write complete watchface code following the design from Phase 2.

Use templates as starting points:
- [templates/animated-watchface.c](templates/animated-watchface.c) — animated watchfaces
- [templates/static-watchface.c](templates/static-watchface.c) — static/analog watchfaces
- [templates/weather-watchface.c](templates/weather-watchface.c) — watchfaces with weather data

**4. src/pkjs/index.js** (REQUIRED if weather/web data needed)
Use [templates/pkjs-weather.js](templates/pkjs-weather.js) as starting point.

The pkjs file runs on the phone and handles:
- GPS location via `navigator.geolocation.getCurrentPosition()`
- HTTP requests via `XMLHttpRequest` to web APIs
- Sending data to watch via `Pebble.sendAppMessage()`
- Receiving requests from watch via `appmessage` event

**Open-Meteo API** (free, no API key):
```
https://api.open-meteo.com/v1/forecast?latitude=LAT&longitude=LON&current=temperature_2m,weather_code
```

### Code Requirements
- `#include <pebble.h>`
- Implement `main()`, `init()`, `deinit()`
- Window with load/unload handlers
- `tick_timer_service_subscribe(MINUTE_UNIT, tick_handler)` — **ALWAYS MINUTE_UNIT**
- For brief animations: `app_timer_register()` with 50ms interval
- Pre-allocate GPath in window_load
- Destroy all resources in unload handlers
- Fixed-point math only (sin_lookup/cos_lookup)
- Use `layer_get_bounds()` for screen dimensions — don't hardcode sizes
- Register AppMessage callbacks BEFORE calling `app_message_open()`

---

## Phase 4: Build PBW

### Run the Build
```bash
cd /path/to/watchface-name
pebble build
```

### Verify Build Success
```bash
ls -la build/*.pbw
```

### Handle Build Errors
If build fails:
1. Read error message
2. Fix C code (syntax, types, missing includes)
3. Run `pebble build` again
4. Repeat until successful

---

## Phase 5: Test in QEMU Emulator

**REQUIRED** — Must test AND visually verify before delivering.

### Step 1: Launch Emulator and Install
```bash
# Primary test - emery (Pebble Time 2, 200x228 color)
pebble install --emulator emery
```

Wait a few seconds for the watchface to load and render.

### Step 2: Capture Screenshot (MANDATORY)
```bash
pebble screenshot --no-open --emulator emery screenshot_emery.png
```

### Step 3: Visual Verification (MANDATORY)

**Use the Read tool to view the screenshot image:**
```
Read tool: screenshot_emery.png
```

**CRITICAL: Perform thorough visual verification using this detailed checklist.**

#### A. Cropping Check (FAIL if any element is cut off)
- [ ] **All visual elements fully visible** — No element should be cut off at screen edges
- [ ] **Key graphics not clipped** — Main visual elements must be 100% within 200x228 bounds
- [ ] **No overflow at bottom** — Elements near y=228 must have margin
- [ ] **No overflow at sides** — Elements near x=0 or x=200 must have margin
- [ ] **Text not truncated** — All text fits within its designated area

#### B. Positioning Check (FAIL if layout doesn't match design)
- [ ] **Time in correct position** — Matches the designed location
- [ ] **Visual elements properly placed** — Each element appears where designed
- [ ] **Proportional spacing** — Elements have appropriate margins
- [ ] **Center alignment** — Centered elements are actually centered (x=100 center)

#### C. Color Scheme Check (FAIL if colors don't match design)
- [ ] **Primary colors correct** — Main colors match design spec
- [ ] **Contrast sufficient** — Text and elements are readable

#### D. Design Intent Check (FAIL if doesn't match user request)
- [ ] **Theme recognizable** — Watchface represents the requested theme
- [ ] **Key features prominent** — Main visual features are visible
- [ ] **Overall composition balanced** — Layout looks intentional

**STOP AND FIX if ANY check fails.** Do not proceed to delivery with visual issues.

### Step 4: Fix Issues and Re-test

If visual verification fails:

#### Fixing Cropping Issues
- **Bottom cropping**: Reduce Y coordinates, use `bounds.size.h - H - margin` formula
- **Side cropping**: Use `bounds.size.w / 2 - element_width / 2` for centering
- **Common mistake**: Hardcoding 144x168 values instead of using `layer_get_bounds()`

#### Iteration Process:
1. Identify which check(s) failed
2. Apply the specific fix
3. Rebuild: `pebble build`
4. Reinstall: `pebble install --emulator emery`
5. New screenshot: `pebble screenshot --no-open --emulator emery screenshot_emery.png`
6. Re-verify with Read tool
7. **Repeat until ALL checks pass**

### Step 5: Check Logs for Errors
```bash
pebble logs --emulator emery
```

Look for:
- APP_LOG errors
- Crashes or exceptions
- Memory warnings

---

## Phase 6: Generate Assets

After the watchface passes visual verification, generate marketing assets.

### Capture Screenshots
```bash
pebble screenshot --no-open --emulator emery screenshot_emery.png
```

### Generate App Icons
```bash
python3 /path/to/skills/pebble-watchface/scripts/create_app_icons.py .
```

Creates `icon_80x80.png` and `icon_144x144.png` from the screenshot.

### Generate Preview GIFs (for animated watchfaces)
```bash
python3 /path/to/skills/pebble-watchface/scripts/create_preview_gif.py . --frames 8 --delay 400
```

Creates `preview_emery.gif` (and other platforms if their emulators are running).

---

## Phase 7: Deliver

### Report to User
After successful build AND visual verification:

1. **PBW Location**: `build/watchface-name.pbw`

2. **Verified Screenshots**: Show the captured screenshots
   - `screenshot_emery.png` — Primary emery display

3. **Preview GIFs** (if animated): `preview_emery.gif` in project root

4. **Visual Confirmation**: Describe what the watchface shows

5. **Install Commands**:
   - Emulator: `pebble install --emulator emery`
   - Device: `pebble install --cloudpebble`

6. **Publish**: Offer to publish to the Pebble App Store (see Phase 8)

7. **Gabbro support**: Suggest a second pass for round display (260x260) if the user wants it

---

## Phase 8: Publish to Pebble App Store (Optional)

If the user wants to publish, use the built-in `pebble publish` command.

### Prerequisites
```bash
# Login first (opens browser for Firebase OAuth)
pebble login

# Check login status
pebble login --status
```

### Interactive Publish (Recommended)
```bash
pebble publish
```

This will:
1. Build the PBW
2. Prompt for screenshot/GIF capture method
3. For new apps: prompt for name, description, category, icons
4. Upload to the Pebble App Store

### Non-Interactive Publish (CI)
```bash
pebble publish --non-interactive \
  --description "Your watchface description" \
  --release-notes "Initial release"
```

### Publish Flags
| Flag | Description |
|------|-------------|
| `--release-notes TEXT` | Release notes for this version |
| `--is-published` | Make release immediately visible |
| `--gif-all-platforms` | Capture rollover GIFs before upload (default: on) |
| `--no-gif-all-platforms` | Skip GIF capture |
| `--non-interactive` | No prompts, use flags/defaults |
| `--name NAME` | App name (new apps only) |
| `--description DESC` | Short description (required for new non-interactive) |
| `--category CAT` | Category: daily, tools, notifications, remotes, health, games |

---

## Weather Watchface Architecture

When a watchface needs weather or other web data, use the **AppMessage + PebbleKit JS** pattern:

```
Watch (C code) ←AppMessage→ Phone (PebbleKit JS) ←HTTP→ Web API
```

### Required Files
1. **src/c/main.c** — C code with AppMessage handlers
2. **src/pkjs/index.js** — JavaScript running on phone
3. **wscript** — Must use `pbl_build(..., bin_type='app')` with `js_entry_file='src/pkjs/index.js'` (the `pbl_program` pattern does NOT work with pkjs). Use the wscript template.

### Required package.json Changes
```json
{
  "pebble": {
    "enableMultiJS": true,
    "capabilities": ["location"],
    "messageKeys": ["TEMPERATURE", "CONDITIONS", "REQUEST_WEATHER"]
  }
}
```

### C Side Pattern
```c
// In init(), register callbacks BEFORE opening:
app_message_register_inbox_received(inbox_received_callback);
app_message_open(128, 128);

// Receive weather data:
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
    Tuple *cond_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
    // Update display...
}

// Request refresh every 30 minutes from tick_handler:
if (tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    AppMessageResult result = app_message_outbox_begin(&iter);
    if (result == APP_MSG_OK) {
        dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
        app_message_outbox_send();
    }
}
```

### JS Side Pattern (src/pkjs/index.js)
```javascript
// Use Open-Meteo API (free, no API key)
function getWeather() {
    navigator.geolocation.getCurrentPosition(function(pos) {
        var url = 'https://api.open-meteo.com/v1/forecast?' +
            'latitude=' + pos.coords.latitude +
            '&longitude=' + pos.coords.longitude +
            '&current=temperature_2m,weather_code';
        // Fetch and send via Pebble.sendAppMessage()...
    });
}

Pebble.addEventListener('ready', function() { getWeather(); });
Pebble.addEventListener('appmessage', function(e) {
    if (e.payload['REQUEST_WEATHER']) getWeather();
});
```

See `tutorials/c-watchface-tutorial/part4/` for a complete working example.

### Visual Weather Reactions (C Side)

The pkjs sends weather as human-readable strings ("Clear", "Cloudy", "Rain", etc.). To change visuals based on weather (sky color, particles, accessories), reverse-map the string to a numeric code on the C side:

```c
static int s_weather_code = -1;  // -1 = no data yet

// In inbox_received_callback, after reading CONDITIONS:
const char *c = cond_tuple->value->cstring;
if (strcmp(c, "Clear") == 0) s_weather_code = 0;
else if (strcmp(c, "Cloudy") == 0) s_weather_code = 2;
else if (strcmp(c, "Rain") == 0 || strcmp(c, "Showers") == 0) s_weather_code = 63;
else if (strcmp(c, "Snow") == 0) s_weather_code = 73;
else if (strcmp(c, "Fog") == 0) s_weather_code = 45;
else if (strcmp(c, "T-Storm") == 0) s_weather_code = 95;
else s_weather_code = 2;

// Then in draw functions, branch on s_weather_code:
if (s_weather_code == 0) { /* draw sun, blue sky */ }
else if (s_weather_code >= 61) { /* draw rain drops */ }
else if (s_weather_code >= 71) { /* draw snowflakes, white ground */ }
```

### Battery-Efficient Visual Variety

Even with `MINUTE_UNIT` updates (no animation timer), you can create visual variety by using deterministic math tied to the minute counter. Each minute tick increments a frame counter, and drawing functions use it to offset positions:

```c
static int s_frame = 0;  // incremented in tick_handler

// In draw function — "animated" rain/snow without a timer:
int rx = (i * 37 + s_frame * 7) % bounds.size.w;
int ry = 40 + (i * 23 + s_frame * 11) % sky_height;
```

This gives a different scene each minute without burning battery on sub-second redraws.

---

## Tutorial Reference

Complete working tutorial examples are in `tutorials/c-watchface-tutorial/`:

| Part | What It Teaches |
|------|-----------------|
| part1 | Basic time + date display with system fonts |
| part4 | Weather via AppMessage + PebbleKit JS + Open-Meteo API |
| part6 | User settings via Clay configuration framework |

These are sourced from [coredevices/c-watchface-tutorial](https://github.com/coredevices/c-watchface-tutorial).

---

## Subagent Summary

| Phase | Subagent Type | Purpose |
|-------|---------------|---------|
| Research | `Explore` | Read samples, tutorials, extract patterns |
| Design | `Plan` | Create implementation plan for emery (200x228) |
| Implement | Direct | Write all project files |
| Build | Direct | Run `pebble build` |
| Test | Direct | Run in QEMU, screenshot, verify with Read tool |
| Iterate | Direct | Fix code until screenshot looks correct |
| Assets | Direct | Run `create_app_icons.py` and `create_preview_gif.py` |
| Deliver | Direct | Report PBW + screenshots + GIFs to user |
| Publish | Direct | Run `pebble publish` if user requests |

---

## Quick Reference

### Emery Screen Dimensions (Default Target)
| Property | Value |
|----------|-------|
| Width | 200 px |
| Height | 228 px |
| Shape | Rectangular |
| Colors | 64-color |
| Center X | 100 |
| Center Y | 114 |

### All Platform Dimensions
| Platform | Resolution | Shape | Color |
|----------|------------|-------|-------|
| emery    | 200x228    | Rect  | 64-color |
| gabbro   | 260x260    | Round | 64-color |
| basalt   | 144x168    | Rect  | 64-color |
| chalk    | 180x180    | Round | 64-color |
| aplite   | 144x168    | Rect  | B&W |
| diorite  | 144x168    | Rect  | B&W |
| flint    | 144x168    | Rect  | 64-color |

### Key APIs
```c
// Drawing
graphics_fill_circle(ctx, center, radius);
graphics_draw_line(ctx, start, end);
graphics_fill_rect(ctx, rect, corner_radius, corners);
graphics_draw_arc(ctx, rect, scale_mode, angle_start, angle_end);
graphics_fill_radial(ctx, rect, scale_mode, inset, angle_start, angle_end);

// Fixed-point trig (NO FLOATS!)
sin_lookup(angle);  // 0 to TRIG_MAX_ANGLE (65536)
cos_lookup(angle);  // returns -TRIG_MAX_RATIO to +TRIG_MAX_RATIO
DEG_TO_TRIGANGLE(degrees);  // macro for conversion

// Time — ALWAYS USE MINUTE_UNIT
tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

// Screen dimensions — use dynamically, don't hardcode
Layer *window_layer = window_get_root_layer(window);
GRect bounds = layer_get_bounds(window_layer);
// bounds.size.w = 200 on emery, bounds.size.h = 228 on emery

// AppMessage (for weather/web data)
app_message_register_inbox_received(callback);
app_message_open(128, 128);
```

### Build & Test Commands
```bash
pebble build                                              # Build PBW
pebble install --emulator emery                           # Test in QEMU
pebble logs --emulator emery                              # View logs
pebble screenshot --no-open --emulator emery              # Capture screen
python3 scripts/create_preview_gif.py . --frames 8          # Capture preview GIFs
python3 scripts/create_app_icons.py .                       # Generate app icons
pebble install --cloudpebble                              # Deploy to device
pebble login                                              # Login for publishing
pebble publish                                            # Publish to App Store
```

### Emulator Interaction Commands

**Button Presses** (`pebble emu-button`):
```bash
pebble emu-button click select --emulator emery
pebble emu-button click back --duration 2000 --emulator emery
```

**Accelerometer Tap** (`pebble emu-tap`):
```bash
pebble emu-tap --emulator emery
```

---

## Constraints

1. **No Floating Point** — Use sin_lookup/cos_lookup only
2. **Pre-allocate Memory** — Create GPath in window_load for static shapes (clock hands, fixed elements). Small dynamic shapes that change position each frame (e.g. character silhouettes at computed coordinates) can use create/destroy in draw functions — this is acceptable for paths with ~3-6 points
3. **MINUTE_UNIT Only** — Never use SECOND_UNIT unless explicitly requested
4. **Clean Resources** — Destroy in unload handlers
5. **NULL Checks** — Verify pointers before use
6. **Overflow Protection** — Use modulo on counters
7. **Dynamic Bounds** — Use `layer_get_bounds()` not hardcoded screen sizes
8. **Register Before Open** — AppMessage callbacks must be registered before `app_message_open()`

---

## File Checklist

Before building:
- [ ] `package.json` with valid UUID and `"targetPlatforms": ["emery"]`
- [ ] `wscript` with build config
- [ ] `src/c/main.c` with complete code
- [ ] `src/pkjs/index.js` (if weather/web data needed)
- [ ] `resources/` directory exists

Build: `pebble build`
Test: `pebble install --emulator emery`
Screenshot: `pebble screenshot --no-open --emulator emery`
Output: `build/[name].pbw`
