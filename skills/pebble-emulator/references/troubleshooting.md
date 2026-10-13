# Troubleshooting the Pebble emulator on macOS

| Symptom | Cause | Fix |
|---|---|---|
| `FileNotFoundError: ... 'qemu-pebble'` on `pebble install --emulator` | The emulator binary is not part of `pebble sdk install` | Run `scripts/setup-macos.sh --yes`, then `. scripts/pebble-env.sh`. Or set `PEBBLE_QEMU_PATH` to a `qemu-pebble` binary. |
| `AssertionError: SRE module mismatch` during `pebble build` | `pebble-tool` was installed on Python 3.11 (or another version) while the SDK's virtualenv is 3.13 | `uv tool install pebble-tool --python 3.13 --reinstall` |
| `Program ['arm-none-eabi-gcc'] is not executable` | The ARM compiler is not on PATH | Source `scripts/pebble-env.sh` (the PebbleOS SDK bundles the compiler and picolibc). Homebrew's `arm-none-eabi-gcc` has no newlib headers. |
| `pebble sdk install latest` says "already installed" but there is no `toolchain` or emulator | The SDK folder is incomplete (an earlier partial install) and the installer only checks the folder exists | The PebbleOS SDK supplies the emulator and compiler anyway. Only rename the old SDK folder if you must reinstall; other projects may use it. |
| `Refusing to load formula ... from untrusted tap pebble/pebble-sdk` | The old tap | Do not trust it. Its `pebble-qemu` is 2016 and Intel-only. |
| `pebble` not found after install | `uv tool` installs to `~/.local/bin` | `. scripts/pebble-env.sh` or `uv tool update-shell` |
| `NegotiationFailed` / "negotiation timed out" in the Core app log | The Pebble protocol port already has a client (`pypkjs`), or the port is the console one | `pgrep -f pypkjs` then `pkill -f pypkjs`; use the FIRST `-serial tcp::N` of `qemu-pebble`. `pair-android-emulator.sh` does both. |
| `pebble install --adb`: "Connection to remote host was lost" once | The dev-connection server was restarting | Run it again. |
| `pebble emu-button ... --adb` is rejected | The command has no adb transport | Press the keys in the emulator window (Q W S X), or `pebble emu-button` while not paired. |
| Android emulator shows "System UI isn't responding" | Gradle and Kotlin daemons from the Core app build use many GB | `./gradlew --stop` in the Core app checkout, `pkill -f KotlinCompileDaemon`, tap Wait. `build-core-app.sh` does this. |
| Play Store on the emulator shows Sign in | Not signed in to Google | Do not sign in for the user. Use `scripts/build-core-app.sh --yes`. The official app is not on F-Droid. |
| Build of the Core app fails with "no reachable tag" | Shallow clone has no tags for `git describe` | `git fetch --unshallow --tags` (the script does it). |
| Screenshot shows another app or a crash dialog | Stale emulator state | `pebble kill && pebble wipe`, reinstall. |
| `libpebble2 ... TimeoutError` on install | Emulator wedged | Same as above. |

## Notes

- The emulator keeps its flash images under `~/Library/Application Support/Pebble SDK/<sdk version>/<platform>/`.
- The Core app's debug receivers (`ADD_QEMU_WATCH`, `SET_SETTING`, `DEV_CONNECTION`) are guarded by `android.permission.DUMP`,
  so only `adb` can send them.
- With no phone paired, an app that asks the phone for data usually shows its own "no answer" screen. That is the app working
  without a companion, not a crash.
