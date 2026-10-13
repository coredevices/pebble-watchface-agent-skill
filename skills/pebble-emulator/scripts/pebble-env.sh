# Source this file (do not execute it) to put the Pebble tools on PATH:
#   . skills/pebble-emulator/scripts/pebble-env.sh
#
# Adds: the `pebble` command (uv tool dir), the newest PebbleOS SDK
# (qemu-pebble and arm-none-eabi-gcc), and Android platform-tools (adb).
# Safe to source more than once. Prints a warning for anything it cannot find.

_pe_warn() { printf 'pebble-env: %s\n' "$*" >&2; }

# `pebble` installed with `uv tool install` lives here.
case ":$PATH:" in *":$HOME/.local/bin:"*) ;; *) export PATH="$HOME/.local/bin:$PATH" ;; esac

# Newest ~/pebbleos-sdk-<version> (or $PEBBLEOS_SDK_ROOT if already set and valid).
if [ -z "${PEBBLEOS_SDK_ROOT:-}" ] || [ ! -f "$PEBBLEOS_SDK_ROOT/env.sh" ]; then
    _pe_root=""
    for _pe_d in "$HOME"/pebbleos-sdk-*; do
        [ -f "$_pe_d/env.sh" ] && _pe_root="$_pe_d"   # glob sorts ascending; last wins
    done
    unset _pe_d
else
    _pe_root="$PEBBLEOS_SDK_ROOT"
fi
if [ -n "${_pe_root:-}" ]; then
    # shellcheck disable=SC1091
    . "$_pe_root/env.sh"
else
    _pe_warn "no ~/pebbleos-sdk-* found (qemu-pebble and the ARM compiler come from it); run setup-macos.sh"
fi
unset _pe_root

# adb: Android SDK platform-tools.
for _pe_sdk in "${ANDROID_HOME:-}" "${ANDROID_SDK_ROOT:-}" "$HOME/Library/Android/sdk" "$HOME/Android/Sdk"; do
    if [ -n "$_pe_sdk" ] && [ -x "$_pe_sdk/platform-tools/adb" ]; then
        case ":$PATH:" in *":$_pe_sdk/platform-tools:"*) ;; *) export PATH="$_pe_sdk/platform-tools:$PATH" ;; esac
        break
    fi
done
unset _pe_sdk

command -v pebble >/dev/null 2>&1 || _pe_warn "pebble not on PATH; run setup-macos.sh"
command -v qemu-pebble >/dev/null 2>&1 || _pe_warn "qemu-pebble not on PATH"
unset -f _pe_warn
