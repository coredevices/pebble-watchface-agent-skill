---
name: pebble-emulator
description: Install and run the Pebble emulator (qemu-pebble) on macOS Apple Silicon, and pair it with the Core Devices Android app on an Android emulator so a watch app can talk to an Android app end to end. Use when `pebble install --emulator` fails with "No such file or directory: 'qemu-pebble'", when setting up the Pebble SDK on a Mac, when a watch app needs a phone companion while testing, or when the Core Devices app cannot be installed from Google Play.
compatibility: macOS on Apple Silicon with Homebrew. Optional - Android SDK with an emulator, JDK 17 or newer. Checked against pebble tool 5.0.40, Pebble SDK 4.33.1, PebbleOS SDK 0.1.11 and the basalt platform.
metadata:
  author: community
  pebble-tool-version: "5.0.40"
  sdk-version: "4.33.1"
  pebbleos-sdk-version: "0.1.11"
---

# Pebble emulator on macOS, and pairing it with an Android emulator

`pebble sdk install` does not provide the emulator binary. On macOS the emulator and the ARM
compiler come from the **PebbleOS SDK installer**. Do not use the old Homebrew tap
`pebble/pebble-sdk`: its `pebble-qemu` formula is from 2016 and Intel-only.

All scripts are in `scripts/` (paths below are relative to this skill). Run them from any directory.

| Script | What it does |
|---|---|
| `scripts/setup-macos.sh [--yes]` | One-time install. Without `--yes` it only prints the plan. |
| `scripts/pebble-env.sh` | **Source** it in every new shell: `pebble`, `qemu-pebble`, `arm-none-eabi-gcc`, `adb` on PATH. |
| `scripts/doctor.sh` | Read-only health check with a fix hint per failure. Run it first. |
| `scripts/pair-android-emulator.sh` | Attach the running emulator to the Core Devices app on Android. |
| `scripts/build-core-app.sh [--yes]` | Build the Core Devices app from source and install it, when Google Play is not usable. |

## Step 0: Ask before downloading

Setup downloads a few hundred MB, and the Core app build downloads more and takes about 10 minutes.
Tell the user what will be downloaded and wait for a yes. Never sign in to a Google account for them.

## Step 1: Check what is there

```bash
. scripts/pebble-env.sh
scripts/doctor.sh
```

If `doctor.sh` exits 0, skip to Step 3.

## Step 2: Install (once)

```bash
scripts/setup-macos.sh          # shows the plan
scripts/setup-macos.sh --yes    # applies it
```

It installs `node libpng uv`, `pebble-tool` on **Python 3.13**, the Pebble SDK, and the PebbleOS SDK
(`~/pebbleos-sdk-<version>`, which contains `qemu-pebble`). It downloads the PebbleOS installer to a
temp file, shows its first lines, and runs it; the installer checks the bundle's sha256.

Python matters: `pebble-tool` supports 3.10 to 3.13. 3.14 is unsupported, and 3.11 fails at build time
with `AssertionError: SRE module mismatch` because the SDK's own virtualenv is Python 3.13.

## Step 3: Run an app on the emulator

```bash
. scripts/pebble-env.sh
pebble build
pebble install --emulator basalt        # also emery, diorite, flint, chalk, aplite, gabbro
pebble screenshot --no-open --emulator basalt "$TMPDIR/shot.png"   # then view the PNG
pebble emu-button click select --emulator basalt                   # back, up, select, down
pebble logs --emulator basalt
pebble kill && pebble wipe              # emulator stuck, stale app, TimeoutError
```

Keys in the emulator window: Q back, W up, S select, X down. Add `--vnc` when there is no display.
Only basalt has been verified with these scripts; the other platforms use the same commands.

## Step 4 (optional): Pair with an Android emulator

Use this when the watch app needs its phone side (PebbleKit, AppMessage to an Android app).
The emulator started by `pebble install --emulator` is then used through the Core Devices app
(`coredevices.coreapp`), which relays to the Android apps.

1. **Get the Core app on the Android emulator.** Check with `scripts/doctor.sh`. If it is missing,
   Google Play needs a sign-in, so build it: `scripts/build-core-app.sh --yes`
   (clones `coredevices/mobileapp`, debug build, installs with `adb install -r`).
2. **Start the emulator first:** `pebble install --emulator basalt`.
3. **Pair:** `scripts/pair-android-emulator.sh` (add `--device SERIAL` if several devices are attached).
   It stops `pypkjs`, forwards the port with `adb reverse`, skips first-run watch onboarding, and sends the
   `ADD_QEMU_WATCH` broadcast. It reports `connected` when the app sees the watch.
4. **From now on, reach the watch through the phone:**
   ```bash
   pebble install --adb
   pebble screenshot --adb --no-open out.png
   ```
   `pebble install --emulator` and `emu-button` talk to the emulator directly, and only one client can use its port.
   While the Core app is paired, use `--adb`. `emu-button` has no `--adb`; press the keys in the emulator window.

What happens: the Core app connects to the first `-serial tcp::N` port of `qemu-pebble`, which is the Pebble protocol
port (the second is the console). `pebble-tool` normally keeps `pypkjs` connected there, and a second client is
refused (negotiation times out), which is why the script stops `pypkjs`. `pebble install --adb` starts the app's
dev-connection server (`DEV_CONNECTION` broadcast) and tunnels to it with `adb forward`.

## Verify, and say what was not checked

- Report what you ran and saw: the `doctor.sh` output, `App install succeeded.`, a screenshot you looked at,
  and for pairing the `connected` line plus the phone-side log (`adb logcat -d | grep -a <your service tag>`).
- Say plainly what was not checked: other platforms, button presses reaching the phone, a real watch.

## When something fails

See `references/troubleshooting.md` (symptoms and fixes: `qemu-pebble` not found, `SRE module mismatch`,
`NegotiationFailed`, "System UI isn't responding", an untrusted Homebrew tap, and more).
For writing watch apps themselves use the `pebble-watchface` skill.
