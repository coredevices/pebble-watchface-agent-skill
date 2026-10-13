#!/usr/bin/env bash
# One-time setup of the Pebble tool, SDK and emulator on macOS Apple Silicon.
#
#   setup-macos.sh          print the plan and what is already installed; change nothing
#   setup-macos.sh --yes    do it (downloads a few hundred MB; asks nothing else)
#
# Steps: brew install node libpng uv; uv tool install pebble-tool (Python 3.13);
# pebble sdk install latest; PebbleOS SDK installer (supplies qemu-pebble and the
# ARM compiler). The installer script is downloaded to a temp dir, its first lines
# are shown, and the SDK download is checksum-verified by the installer itself.
set -euo pipefail

YES=0
for a in "$@"; do
    case "$a" in
        --yes) YES=1 ;;
        -h|--help) sed -n '2,12p' "$0"; exit 0 ;;
        *) echo "unknown argument: $a" >&2; exit 2 ;;
    esac
done

[ "$(uname -s)" = Darwin ] || { echo "This script is for macOS. On Linux follow https://developer.repebble.com/sdk" >&2; exit 1; }
command -v brew >/dev/null 2>&1 || { echo "Homebrew is required: https://brew.sh" >&2; exit 1; }
[ "$(uname -m)" = arm64 ] || echo "note: only tested on Apple Silicon (arm64); this machine is $(uname -m)" >&2

have_brew() { brew list --formula "$1" >/dev/null 2>&1; }
installer_url="https://github.com/coredevices/PebbleOS-SDK/releases/latest/download/pebbleos-sdk-installer.sh"
export PATH="$HOME/.local/bin:$PATH"

missing_brew=()
for f in node libpng uv; do have_brew "$f" || missing_brew+=("$f"); done
have_pebble=0; command -v pebble >/dev/null 2>&1 && have_pebble=1
have_sdk=0; compgen -G "$HOME/pebbleos-sdk-*/qemu/bin/qemu-pebble" >/dev/null && have_sdk=1
have_core_sdk=0; [ -d "$HOME/Library/Application Support/Pebble SDK/SDKs" ] && ls "$HOME/Library/Application Support/Pebble SDK/SDKs" 2>/dev/null | grep -qE '^[0-9]' && have_core_sdk=1

echo "Plan:"
echo "  1. brew install: ${missing_brew[*]:-(nothing, all present)}"
echo "  2. pebble-tool on Python 3.13: $([ $have_pebble = 1 ] && echo 'already installed' || echo 'will install')"
echo "  3. Pebble SDK (pebble sdk install latest): $([ $have_core_sdk = 1 ] && echo 'an SDK is present' || echo 'will install')"
echo "  4. PebbleOS SDK (qemu-pebble, ARM compiler) from $installer_url: $([ $have_sdk = 1 ] && echo 'already installed' || echo 'will install to ~/pebbleos-sdk-<version>')"
if [ $YES = 0 ]; then
    echo; echo "Dry run. Re-run with --yes to apply."; exit 0
fi

if [ ${#missing_brew[@]} -gt 0 ]; then
    HOMEBREW_NO_AUTO_UPDATE=1 brew install "${missing_brew[@]}"
fi

# pebble-tool needs Python 3.10-3.13. 3.14 is unsupported, and 3.11 fails against the
# SDK's Python 3.13 virtualenv ("SRE module mismatch"), so pin 3.13.
if [ $have_pebble = 0 ]; then
    uv tool install pebble-tool --python 3.13
fi

pebble sdk install latest || true   # exits non-zero with "already installed" when present

if [ $have_sdk = 0 ]; then
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' EXIT
    curl -LsSf "$installer_url" -o "$tmp/installer.sh"
    echo "--- installer.sh (first 25 lines) ---"; sed -n '1,25p' "$tmp/installer.sh"; echo "---"
    # A file, not `curl | sh`: --defaults takes the default prefix, and the installer verifies the bundle's sha256.
    sh "$tmp/installer.sh" --defaults
fi

echo
echo "Done. In each new shell run:  . $(cd "$(dirname "$0")" && pwd)/pebble-env.sh"
echo "Then verify with:             $(cd "$(dirname "$0")" && pwd)/doctor.sh"
