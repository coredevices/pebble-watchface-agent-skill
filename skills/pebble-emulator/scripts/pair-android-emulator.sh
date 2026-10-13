#!/usr/bin/env bash
# Attach the running Pebble emulator (qemu-pebble) to the Core Devices app on an
# Android emulator or phone, so watch apps can talk to Android apps end to end.
#
#   pair-android-emulator.sh [--device SERIAL] [--port N] [--keep-pypkjs] [--dry-run]
#
# Needs: the Core Devices app (coredevices.coreapp, debug build or any build that has
# the DUMP-guarded debug receivers) on the device, adb on PATH, and a running emulator
# started with `pebble install --emulator <platform>`.
#
# Why pypkjs is stopped: qemu-pebble's Pebble protocol port accepts ONE client.
# pebble-tool's own helper (pypkjs) holds it; the Core app is locked out until it is gone.
# `pebble kill` stops both the emulator and pypkjs; `pebble install --emulator` brings them back.
set -euo pipefail

DEVICE=""; PORT=""; KEEP=0; DRY=0
while [ $# -gt 0 ]; do
    case "$1" in
        --device) DEVICE="$2"; shift 2 ;;
        --port) PORT="$2"; shift 2 ;;
        --keep-pypkjs) KEEP=1; shift ;;
        --dry-run) DRY=1; shift ;;
        -h|--help) sed -n '2,15p' "$0"; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

here="$(cd "$(dirname "$0")" && pwd)"
# shellcheck disable=SC1091
. "$here/pebble-env.sh" 2>/dev/null
command -v adb >/dev/null 2>&1 || { echo "adb not found (set ANDROID_HOME or install platform-tools)" >&2; exit 1; }

# Pick the Android device.
if [ -z "$DEVICE" ]; then
    devs=($(adb devices | awk 'NR>1 && $2=="device"{print $1}'))
    case ${#devs[@]} in
        0) echo "no Android device attached" >&2; exit 1 ;;
        1) DEVICE="${devs[0]}" ;;
        *) echo "several devices attached (${devs[*]}); pass --device SERIAL" >&2; exit 1 ;;
    esac
fi
A=(adb -s "$DEVICE")

"${A[@]}" shell pm list packages 2>/dev/null | grep -q '^package:coredevices.coreapp$' \
    || { echo "$DEVICE has no coredevices.coreapp; run build-core-app.sh --yes" >&2; exit 1; }

# The Pebble protocol port is the FIRST `-serial tcp::N` of qemu-pebble (the second is the console).
if [ -z "$PORT" ]; then
    pid="$(pgrep -f qemu-pebble | head -1 || true)"
    [ -n "$pid" ] || { echo "no qemu-pebble running; start it with: pebble install --emulator basalt" >&2; exit 1; }
    PORT="$(ps -o args= -p "$pid" | grep -o -- '-serial tcp::[0-9]*' | head -1 | grep -o '[0-9]*$')"
    [ -n "$PORT" ] || { echo "could not find the Pebble protocol port in the qemu-pebble arguments" >&2; exit 1; }
fi
echo "device: $DEVICE   Pebble protocol port: $PORT"

run() { if [ $DRY = 1 ]; then echo "+ $*"; else "$@"; fi; }

if [ $KEEP = 0 ] && pgrep -f pypkjs >/dev/null 2>&1; then
    echo "stopping pypkjs (it holds the single client connection)"
    run pkill -f pypkjs
    [ $DRY = 1 ] || sleep 2
fi

run "${A[@]}" reverse "tcp:$PORT" "tcp:$PORT"
# Skip the first-run watch onboarding, which would otherwise start a firmware upgrade on connect.
run "${A[@]}" shell am broadcast -a coredevices.coreapp.SET_SETTING \
    -n coredevices.coreapp/coredevices.coreapp.debug.SetSettingReceiver \
    --es key hasSeenWatchOnboarding --ez value true
[ $DRY = 1 ] || "${A[@]}" logcat -c
run "${A[@]}" shell am broadcast -a coredevices.coreapp.ADD_QEMU_WATCH \
    -n coredevices.coreapp/coredevices.coreapp.debug.QemuSetupReceiver \
    --es host 127.0.0.1 --ei port "$PORT"

[ $DRY = 1 ] && exit 0
for _ in $(seq 1 20); do
    sleep 2
    if "${A[@]}" logcat -d 2>/dev/null | grep -a "ConnectedPebbleDevice" | grep -aq "127.0.0.1:$PORT"; then
        echo "connected: the Core app sees the emulator as a watch"
        echo "next: pebble install --adb   (and pebble screenshot --adb --no-open out.png)"
        exit 0
    fi
done
echo "not connected after 40 s. Likely causes: another client holds the port (pgrep -f pypkjs), the app's first-run screen is blocking, or negotiation timed out. See references/troubleshooting.md." >&2
"${A[@]}" logcat -d | grep -a -E "Negotiator|NegotiationFailed" | tail -3 >&2 || true
exit 1
