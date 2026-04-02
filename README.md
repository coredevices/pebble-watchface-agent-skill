# Pebble Watchface Generator Skill

Generate complete, buildable Pebble smartwatch watchfaces using Claude Code with full PBW artifact output and QEMU emulator testing.

## Get Started

```bash
git clone https://github.com/coredevices/pebble-watchface-agent-skill
cd pebble-watchface-agent-skill
claude
```

Then just ask:

```
Create an animated underwater watchface with fish and bubbles
```

Claude automatically detects the skill and handles everything — design, code generation, build, emulator testing, and screenshot verification.

> **Requires:** [Claude Code](https://docs.anthropic.com/en/docs/claude-code) and the [Pebble SDK](https://developer.repebble.com/sdk/) installed.

## What This Does

This repository contains a **Claude Code skill** that transforms natural language descriptions into fully functional Pebble watchfaces. Simply describe what you want, and Claude will:

1. **Design** the watchface architecture
2. **Generate** all source files (C code, package.json, wscript, pkjs)
3. **Build** a ready-to-install `.pbw` file
4. **Test** in the QEMU emulator
5. **Verify** visually with screenshots
6. **Deliver** the final artifacts with rollover GIFs
7. **Publish** to the Pebble App Store (optional)

**Default target: Emery (Pebble Time 2, 200x228 color rectangular display).**

## Prerequisites

Before using this skill, ensure you have:

- **Claude Code CLI** installed and configured
- **Pebble SDK** Follow the instructions in the official [documentation](https://developer.repebble.com/sdk/)
- **QEMU** for emulator testing (bundled with Pebble SDK)
- **Python 3** with Pillow (`pip install Pillow`) for icon and GIF generation

## Example Prompts

Here are some things you can ask for:

```
Create a retro digital watchface with a neon green display on black background
```

Or with weather:

```
Make a watchface that shows the time, date, and current weather conditions
```

Claude will automatically invoke the `pebble-watchface` skill and handle everything from design to delivery.

## How the Skill Works

### Skill Location

```
.claude/skills/pebble-watchface/
├── SKILL.md              # Main skill definition
├── reference/            # API documentation
│   ├── pebble-api-reference.md
│   ├── animation-patterns.md
│   └── drawing-guide.md
├── samples/              # Working example watchfaces
│   └── aqua-pbw/         # Animated aquarium watchface
├── scripts/              # Helper utilities
│   ├── create_project.py
│   ├── generate_uuid.py
│   └── validate_project.py
└── templates/            # Code templates
    ├── animated-watchface.c
    ├── static-watchface.c
    ├── weather-watchface.c
    ├── pkjs-weather.js
    ├── package.json.template
    └── wscript.template
```

### The Workflow

| Phase | What Happens |
|-------|--------------|
| **1. Research** | Gathers requirements, studies sample code and tutorials |
| **2. Design** | Plans layout for emery (200x228), animations, data structures |
| **3. Implement** | Writes all project files (main.c, package.json, wscript, pkjs) |
| **4. Build** | Runs `pebble build` to generate the PBW |
| **5. Test** | Installs in QEMU, captures screenshots |
| **6. Iterate** | Fixes issues until visual verification passes |
| **7. Assets** | Generates icons and preview GIFs via helper scripts |
| **8. Deliver** | Reports PBW location with screenshots and GIFs |
| **9. Publish** | Publishes to Pebble App Store via `pebble publish` (optional) |

### Supported Platforms

| Platform | Model | Display | Resolution | Colors |
|----------|-------|---------|------------|--------|
| **emery** | **Pebble Time 2** | **Rectangular** | **200x228** | **64 colors** |
| gabbro | Pebble Round 2 | Round | 260x260 | 64 colors |
| basalt | Pebble Time | Rectangular | 144x168 | 64 colors |
| chalk | Pebble Time Round | Round | 180x180 | 64 colors |
| aplite | Pebble Classic | Rectangular | 144x168 | B&W |
| diorite | Pebble 2 | Rectangular | 144x168 | B&W |
| flint | Pebble 2 Duo | Rectangular | 144x168 | 64 colors |

**Emery is the default target.** Gabbro (round) support can be added as a second pass.

## Weather Watchfaces

The skill supports watchfaces that display weather and other web data using the **AppMessage + PebbleKit JS** pattern:

- Watch C code communicates with phone-side JavaScript via AppMessage
- PebbleKit JS (`src/pkjs/index.js`) fetches data from web APIs
- Uses [Open-Meteo API](https://open-meteo.com/) (free, no API key needed)
- Weather refreshes every 30 minutes for battery efficiency

See `tutorials/c-watchface-tutorial/part4/` for a complete working example.

## Tutorial Source Code

Complete Pebble C watchface tutorials are included in `tutorials/c-watchface-tutorial/`, sourced from [coredevices/c-watchface-tutorial](https://github.com/coredevices/c-watchface-tutorial):

| Part | Topic | Key Concepts |
|------|-------|--------------|
| part1 | Basic time + date | Window, TextLayer, TickTimerService, system fonts |
| part4 | Weather data | AppMessage, PebbleKit JS, Open-Meteo API, XMLHttpRequest |
| part6 | User settings | Clay configuration, persistent storage, color pickers |

## Publishing

Publish directly to the Pebble App Store:

```bash
# Login (one-time, opens browser)
pebble login

# Publish (interactive — prompts for details)
pebble publish

# Or non-interactive
pebble publish --non-interactive --description "My watchface"
```

## Output Artifacts

After successful generation, you'll receive:

```
your-watchface/
├── build/
│   └── your-watchface.pbw    # Ready-to-install watchface
├── src/c/
│   └── main.c                # Generated C source code
├── src/pkjs/
│   └── index.js              # Phone-side JS (if weather/web data)
├── package.json              # Pebble project manifest
├── wscript                   # Build configuration
├── screenshots/              # Captured by pebble screenshot
│   ├── emery_*.png           # Static screenshots
│   └── emery_*.gif           # Rollover GIFs
└── screenshot_emery.png      # Verification screenshot
```

## Installing Your Watchface

### On Emulator
```bash
cd your-watchface
pebble install --emulator emery
```

### On Physical Watch
```bash
pebble install --phone
```

## Key Technical Constraints

1. **No Floating Point** — Uses `sin_lookup()`/`cos_lookup()` for trigonometry
2. **MINUTE_UNIT Updates** — Always uses minute-based tick updates for battery efficiency
3. **Pre-allocated Memory** — Creates GPaths in `window_load`
4. **Dynamic Bounds** — Uses `layer_get_bounds()` instead of hardcoded screen sizes
5. **Resource Cleanup** — Properly destroys all resources in unload handlers

## Troubleshooting

### Build Fails
- Check for syntax errors in the generated C code
- Verify `pebble-sdk` is properly installed
- Ensure all required files exist (package.json, wscript, src/c/main.c)

### Emulator Won't Start
- Run `pebble sdk install-emulator emery` to install emulator
- Check QEMU is installed: `which qemu-system-arm`

### GIF Capture Fails
- Ensure Pillow is installed: `pip install Pillow`
- Make sure the emulator is running before running `create_preview_gif.py`

### Watchface Looks Wrong
- The skill includes visual verification — it will iterate until correct
- If issues persist, provide specific feedback about what's wrong

## Resources

- [Pebble SDK Documentation](https://developer.repebble.com/)
- [Open-Meteo Weather API](https://open-meteo.com/en/docs)
- [C Watchface Tutorial (source)](https://github.com/coredevices/c-watchface-tutorial)
- [Claude Code Documentation](https://docs.anthropic.com/claude-code)

## Acknowledgments

This skill is derived from the excellent [pebble-wf-agent-skill](https://github.com/priyankark/pebble-wf-agent-skill) by [priyankark](https://github.com/priyankark).

## License

This skill and associated templates are provided for creating Pebble watchfaces. Individual watchfaces you create are your own.
