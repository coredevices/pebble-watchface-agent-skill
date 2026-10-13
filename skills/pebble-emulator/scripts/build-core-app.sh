#!/usr/bin/env bash
# Build the Core Devices (Pebble) Android app from source and install it on an
# Android emulator. Use this when Google Play is not available (no Google sign-in) and
# the app is not on F-Droid.
#
#   build-core-app.sh [--dir DIR] [--device SERIAL] [--no-install] [--yes]
#
# Without --yes it prints the plan and stops. The build takes about 10 minutes, downloads
# Gradle dependencies and an Android platform, and needs about 8 GB of RAM.
# Source: https://github.com/coredevices/mobileapp (GPLv3). Debug build, applicationId coredevices.coreapp.
set -euo pipefail

DIR="${PEBBLE_CORE_APP_DIR:-$HOME/coredevices-mobileapp}"; DEVICE=""; INSTALL=1; YES=0
while [ $# -gt 0 ]; do
    case "$1" in
        --dir) DIR="$2"; shift 2 ;;
        --device) DEVICE="$2"; shift 2 ;;
        --no-install) INSTALL=0; shift ;;
        --yes) YES=1; shift ;;
        -h|--help) sed -n '2,11p' "$0"; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

here="$(cd "$(dirname "$0")" && pwd)"
# shellcheck disable=SC1091
. "$here/pebble-env.sh" 2>/dev/null

# The build needs a JDK 17+ (the project targets 17; JDK 21 works) and the Android SDK.
if [ -z "${JAVA_HOME:-}" ]; then
    for j in /opt/homebrew/opt/openjdk@21/libexec/openjdk.jdk/Contents/Home \
             /opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home; do
        [ -x "$j/bin/java" ] && JAVA_HOME="$j" && break
    done
fi
[ -n "${JAVA_HOME:-}" ] || { echo "no JDK found: brew install openjdk@21, or set JAVA_HOME" >&2; exit 1; }
export JAVA_HOME
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}"
[ -d "$SDK/platforms" ] || { echo "Android SDK not found at $SDK; set ANDROID_HOME" >&2; exit 1; }
export ANDROID_HOME="$SDK"

echo "Plan: clone coredevices/mobileapp into $DIR (full history: the version comes from 'git describe --tags'),"
echo "      add the committed dummy google-services.json and a local.properties (sdk.dir=$SDK),"
echo "      JAVA_HOME=$JAVA_HOME ./gradlew :androidApp:assembleDebug  (~10 min),"
echo "      $([ $INSTALL = 1 ] && echo 'adb install -r the debug APK' || echo 'no install'), then ./gradlew --stop."
[ $YES = 1 ] || { echo "Dry run. Re-run with --yes to apply."; exit 0; }

if [ ! -d "$DIR/.git" ]; then
    git clone --recurse-submodules https://github.com/coredevices/mobileapp "$DIR"
fi
cd "$DIR"
git describe --tags --abbrev=0 >/dev/null 2>&1 || git fetch --unshallow --tags 2>/dev/null || git fetch --tags
cp -n androidApp/src/google-services-dummy.json androidApp/src/google-services.json
if [ ! -f local.properties ]; then
    printf 'sdk.dir=%s\nLOCAL_RELEASE_BUILD=true\n' "$SDK" > local.properties
fi

./gradlew :androidApp:assembleDebug
apk="$(find androidApp/build/outputs/apk/debug -name '*.apk' | head -1)"
echo "built: $DIR/$apk"

# The Gradle and Kotlin daemons use many GB; with them running the Android emulator
# often shows "System UI isn't responding". Stop them before installing.
./gradlew --stop >/dev/null 2>&1 || true
pkill -f KotlinCompileDaemon 2>/dev/null || true

if [ $INSTALL = 1 ]; then
    if [ -z "$DEVICE" ]; then
        devs=($(adb devices | awk 'NR>1 && $2=="device"{print $1}'))
        [ ${#devs[@]} -eq 1 ] || { echo "install: need exactly one device or --device SERIAL (found ${#devs[@]}); APK is at $DIR/$apk" >&2; exit 1; }
        DEVICE="${devs[0]}"
    fi
    adb -s "$DEVICE" install -r "$apk"
    echo "installed on $DEVICE. Next: pair-android-emulator.sh"
fi
