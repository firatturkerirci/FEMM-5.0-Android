#!/usr/bin/env bash
# Install the APK on a running emulator, run the demo script through the
# script panel (femm_gui -selftest), and collect the result, a screenshot and
# the log. Exit 0 when the script ran to completion inside the app.
#
#   smoke_test.sh <apk> <application id> <output dir>
set -u
APK=$1; PKG=$2; OUT=$3
mkdir -p "$OUT"
EXT="/sdcard/Android/data/$PKG/files"

adb wait-for-device
adb install -r -g "$APK" || exit 1
adb logcat -c
adb shell am start -W -n "$PKG/$PKG.FemmActivity" \
    -e applicationArguments "-selftest ai_demo_solenoid.py selftest.png"

# The app exits when the self-test is done; give it up to 4 minutes.
status=1
for i in $(seq 1 120); do
    sleep 2
    if adb shell "test -f $EXT/selftest.png.txt" 2>/dev/null; then
        sleep 2
        status=0
        break
    fi
done

adb exec-out screencap -p > "$OUT/screen.png" || true
adb pull "$EXT/selftest.png" "$OUT/" || true
adb pull "$EXT/selftest.png.txt" "$OUT/" || true
adb logcat -d > "$OUT/logcat.txt" || true
grep -iE "femm|qt|fatal|crash|selftest" "$OUT/logcat.txt" | tail -80 || true

if [ $status -ne 0 ]; then
    echo "self-test did not finish"
    exit 1
fi
echo "--- self-test result"
cat "$OUT/selftest.png.txt"
head -1 "$OUT/selftest.png.txt" | grep -q "^Completed" || exit 1
