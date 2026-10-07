# Pebble watchface and watchapp skill

An [Agent Skill](https://agentskills.io) that teaches a coding agent to
build Pebble watchfaces and watchapps: design the layout, write the
project files (C or Alloy), run `pebble build`, install the result on
the emulator, screenshot it, check the screenshot, and fix what is
wrong. It also covers icons, preview GIFs, and publishing to the Pebble
App Store with `pebble publish`.

The skill is in `skills/pebble-watchface/`. It works with Claude Code,
Cursor, Codex, and any other agent that reads `SKILL.md` files.

Last checked against SDK 4.33.1 and pebble tool 5.0.40.

## Requirements

- The Pebble SDK and the `pebble` tool. Install them by following
  https://developer.repebble.com/sdk. The emulator (QEMU) comes with
  the SDK.
- Python 3 with Pillow (`pip install Pillow`) for the icon and GIF
  scripts.
- A coding agent that supports Agent Skills.

## Install

Pick the agent you use. Each one reads the same `SKILL.md`.

### Claude Code

Copy the skill directory into your project or into your home
directory:

```bash
# project only
mkdir -p .claude/skills
cp -r /path/to/pebble-watchface-agent-skill/skills/pebble-watchface .claude/skills/

# every project
mkdir -p ~/.claude/skills
cp -r /path/to/pebble-watchface-agent-skill/skills/pebble-watchface ~/.claude/skills/
```

Claude Code picks up changes to an existing skills directory without a
restart. If `.claude/skills` did not exist when the session started,
run `/reload-skills` or start a new session after copying. Skills are
documented at https://code.claude.com/docs/en/skills.

### Cursor

Cursor reads skills from `.cursor/skills/` or `.agents/skills/` in the
project and from `~/.cursor/skills/` or `~/.agents/skills/` for every
project. It also reads `.claude/skills/`.

```bash
mkdir -p .cursor/skills
cp -r /path/to/pebble-watchface-agent-skill/skills/pebble-watchface .cursor/skills/
```

See https://cursor.com/docs/context/skills.

### Codex

Codex reads skills from `.agents/skills/` in the repository and from
`~/.agents/skills/` for every project.

```bash
mkdir -p .agents/skills
cp -r /path/to/pebble-watchface-agent-skill/skills/pebble-watchface .agents/skills/
```

Codex also has a built-in `$skill-installer` that can fetch a skill
from a repository. See https://developers.openai.com/codex/skills.

### Agents that read AGENTS.md

If your agent does not load skills on its own, copy the skill directory
somewhere in the project and add a line to `AGENTS.md`:

```markdown
When building a Pebble watchface or watchapp, read
.agents/skills/pebble-watchface/SKILL.md first and follow it.
```

### With the `skills` CLI

The community `skills` CLI from Vercel Labs installs a skill from a
GitHub repository into the right directory for 75+ agents, including
the three above:

```bash
npx skills add coredevices/pebble-watchface-agent-skill
```

See https://github.com/vercel-labs/skills.

### Or clone this repository

`.claude/skills/pebble-watchface` and `.agents/skills/pebble-watchface`
are symlinks to `skills/pebble-watchface`, so running your agent inside
a clone of this repository also works:

```bash
git clone https://github.com/coredevices/pebble-watchface-agent-skill
cd pebble-watchface-agent-skill
claude
```

On Windows, Git with `core.symlinks=false` (the default) checks the two
symlinks out as text files, so agents will not find `SKILL.md`. Either
run `git config --global core.symlinks true` before cloning, or delete
the two placeholder files and copy `skills/pebble-watchface` into
`.claude/skills/` (or `.agents/skills/`).

## Usage

Describe what you want:

```
Create an animated underwater watchface with fish and bubbles
```

```
Make a watchapp that shows a countdown timer and vibrates at zero
```

The agent follows `SKILL.md`: it picks watchface or watchapp, C or
Alloy, writes the files, builds, installs on the emery emulator, takes
a screenshot, looks at it, and iterates. You get `build/<name>.pbw`,
`screenshot_emery.png`, and, for animated faces, `preview_emery.gif`.

Install the result on a watch with `pebble install --phone <ip>` or
`pebble install --cloudpebble`. Publish with `pebble login` then
`pebble publish`.

## What the skill covers

- Watchfaces and watchapps, in C or in Alloy (JavaScript on the watch,
  emery and gabbro only).
- Emery (Pebble Time 2, 200x228) as the default target. Other platforms:
  gabbro (Pebble Round 2), flint (Pebble 2 Duo), basalt, chalk, aplite,
  diorite.
- Layout planning so nothing is cropped, `MINUTE_UNIT` ticks for
  battery life, fixed-point math, resource cleanup.
- Weather and other web data through PebbleKit JS (C) or `fetch()`
  (Alloy), using the Open-Meteo API.
- The emulator loop: `pebble build`, `pebble install --emulator emery`,
  `pebble screenshot --no-open`, `pebble emu-button`, `pebble logs`,
  and how to recover when the emulator is in a bad state.
- App icons, preview GIFs, and `pebble publish`.

Layout:

```
skills/pebble-watchface/
├── SKILL.md                 # the workflow
├── references/
│   ├── live-docs.md         # URLs of the API pages on developer.repebble.com
│   ├── alloy-guide.md       # Alloy project anatomy, Poco, sensors, networking, pitfalls
│   ├── watchapp-guide.md    # buttons, window stack, menus, game loop
│   ├── animation-patterns.md
│   └── drawing-guide.md
├── scripts/
│   ├── create_project.py    # scaffold a C project from a template
│   ├── validate_project.py  # check package.json, wscript, sources
│   ├── create_app_icons.py  # 80x80 and 144x144 icons from a screenshot
│   ├── create_preview_gif.py
│   └── generate_uuid.py
└── templates/               # C and Alloy starting points, package.json, wscript
```

API reference is not bundled. `references/live-docs.md` points at the
Markdown version of each page on https://developer.repebble.com (append
`.md` to any page URL) and at https://developer.repebble.com/llms.txt,
so the agent reads current documentation instead of a copy.

The `samples/projects/` and `tutorials/` directories hold complete
projects built with this skill and the source of the C watchface
tutorial. They are for reading; the skill does not depend on them.

## Updating

Pull the repository and copy `skills/pebble-watchface` over your
installed copy, or run `npx skills add` again. The SDK version the
skill was last checked against is in the `metadata` block at the top of
`SKILL.md`; after an SDK release, check
https://developer.repebble.com/sdk/changelogs for changes to commands
or APIs the skill uses.

## Troubleshooting

- `pebble build` fails: read the compiler error, fix `src/c/main.c`,
  build again. Make sure `package.json`, `wscript`, and `src/c/main.c`
  exist.
- The emulator does not start: run `pebble kill`, then
  `pebble install --emulator emery` again. If it is still wedged,
  `pebble wipe` clears emulator state.
- The screenshot shows a different app: the emulator kept state from an
  earlier project. `pebble kill && pebble wipe`, then reinstall.
- GIF or icon scripts fail: install Pillow, and make sure the emulator
  is running before `create_preview_gif.py`.

## Links

- Developer documentation: https://developer.repebble.com
- Markdown index for agents: https://developer.repebble.com/llms.txt
- The `pebble` tool: https://developer.repebble.com/guides/tools-and-resources/pebble-tool
- Agent Skills specification: https://agentskills.io/specification
- C watchface tutorial source: https://github.com/coredevices/c-watchface-tutorial

## Acknowledgments

This skill started from
[pebble-wf-agent-skill](https://github.com/priyankark/pebble-wf-agent-skill)
by [priyankark](https://github.com/priyankark).

## License

The skill and templates are provided for creating Pebble watchfaces
and watchapps. Watchfaces and apps you create with it are your own.
