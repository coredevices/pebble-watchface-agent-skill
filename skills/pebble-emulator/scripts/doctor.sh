#!/usr/bin/env bash
# Check that the Pebble emulator tooling works. Changes nothing.
# Exit status 0 only when everything needed to run the emulator is present.
here="$(cd "$(dirname "$0")" && pwd)"
# shellcheck disable=SC1091
. "$here/pebble-env.sh" 2>/dev/null

fail=0
ok()   { printf '  ok    %s\n' "$*"; }
bad()  { printf '  FAIL  %s\n' "$*"; fail=1; }
info() { printf '  info  %s\n' "$*"; }

echo "Pebble tool and SDK"
if command -v pebble >/dev/null 2>&1; then
    v="$(pebble --version 2>/dev/null | tail -1)"; ok "${v:-pebble found}"
    pbin="$(readlink -f "$(command -v pebble)" 2>/dev/null || command -v pebble)"
    pyv="$("$(dirname "$pbin")/python" --version 2>&1 | awk '{print $2}')"
    case "$pyv" in
        3.13.*) ok "pebble runs on Python $pyv" ;;
        3.14.*) bad "pebble runs on Python $pyv (unsupported): uv tool install pebble-tool --python 3.13 --reinstall" ;;
        3.1[0-2].*) info "pebble runs on Python $pyv; if a build fails with 'SRE module mismatch' reinstall on 3.13" ;;
        *) info "could not read pebble's Python version" ;;
    esac
    pebble sdk list 2>/dev/null | sed -n '/^Installed/,/^$/p' | sed 's/^/  info  /'
else
    bad "pebble not found: run setup-macos.sh, then  . pebble-env.sh"
fi

echo "Emulator and compiler"
if command -v qemu-pebble >/dev/null 2>&1; then
    ok "$(qemu-pebble --version 2>/dev/null | head -1)"
else
    bad "qemu-pebble not on PATH: 'pebble sdk install' does not provide it; run setup-macos.sh (PebbleOS SDK) and source pebble-env.sh"
fi
if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    ok "arm-none-eabi-gcc $(arm-none-eabi-gcc -dumpversion 2>/dev/null)"
else
    bad "arm-none-eabi-gcc not on PATH (comes with the PebbleOS SDK)"
fi

echo "Running emulators"
if pgrep -f qemu-pebble >/dev/null 2>&1; then
    ps -o args= -p "$(pgrep -f qemu-pebble | head -1)" | grep -o 'tcp::[0-9]*' | head -2 | sed 's/^/  info  qemu-pebble listening on /'
    pgrep -f pypkjs >/dev/null 2>&1 && info "pypkjs is running (it holds the emulator's one client connection)"
else
    info "no qemu-pebble running (pebble install --emulator <platform> starts one)"
fi

echo "Android (optional, for pairing with the Core Devices app)"
if command -v adb >/dev/null 2>&1; then
    devs="$(adb devices 2>/dev/null | awk 'NR>1 && $2=="device"{print $1}')"
    if [ -z "$devs" ]; then info "adb found, no device attached"; fi
    for d in $devs; do
        if adb -s "$d" shell pm list packages 2>/dev/null | grep -q '^package:coredevices.coreapp$'; then
            ok "$d has coredevices.coreapp"
        else
            info "$d has no Core Devices app (build-core-app.sh builds one)"
        fi
    done
else
    info "adb not found"
fi

exit $fail
