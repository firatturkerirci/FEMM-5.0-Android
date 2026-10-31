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
ACT=$(adb shell cmd package resolve-activity --brief -c android.intent.category.LAUNCHER "$PKG" | tail -1 | tr -d '\r')
echo "launcher activity: $ACT"
# One string for the device shell, so the argument list keeps its spaces.
adb shell "am start -W -n $ACT -e applicationArguments '-selftest ai_demo_solenoid.py selftest.png'"

# The app exits when the self-test is done; give it up to 4 minutes.
status=1
for i in $(seq 1 80); do
    sleep 3
    if adb shell "test -f $EXT/selftest.png.txt" 2>/dev/null; then
        sleep 2
        status=0
        break
    fi
    if [ $i -eq 10 ]; then
        echo "--- after 30 s: process: $(adb shell pidof "$PKG" | tr -d '\r')"
    fi
done

adb exec-out screencap -p > "$OUT/screen.png" || true
adb pull "$EXT/selftest.png" "$OUT/" || true
adb pull "$EXT/selftest.png.txt" "$OUT/" || true
adb logcat -d > "$OUT/logcat.txt" || true

echo "--- process still running: $(adb shell pidof "$PKG" | tr -d '\r')"
echo "--- app log (Qt, FEMM, crashes)"
grep -E "libfemm_gui|femm|Qt[A-Z ]|qt\.|AndroidRuntime|DEBUG   :|F DEBUG|libc    :|selftest|FATAL|Abort|SIGSEGV|SIGABRT" "$OUT/logcat.txt" \
    | grep -v NamespaceManager | tail -120 || true
echo "--- external files"
adb shell ls -la "$EXT" 2>&1 || true

# A small copy of the screen in the log, for when the artifact cannot be
# downloaded: base64 JPEG between markers.
if [ -s "$OUT/screen.png" ] && python3 -m pip install --quiet --user pillow > /dev/null 2>&1; then
    python3 - "$OUT/screen.png" "$OUT/screen_small.jpg" <<'PY'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert('RGB')
im.thumbnail((900, 900))
im.save(sys.argv[2], 'JPEG', quality=60)
PY
    echo "SCREEN-B64-BEGIN"; base64 -w 0 "$OUT/screen_small.jpg"; echo; echo "SCREEN-B64-END"
fi

if [ $status -ne 0 ]; then
    echo "self-test did not finish"
    exit 1
fi
echo "--- self-test result"
cat "$OUT/selftest.png.txt"
head -1 "$OUT/selftest.png.txt" | grep -q "^Completed" || exit 1
