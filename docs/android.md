# FEMM on Android

The Android build is the regular femm-qt tree built with Qt 6 for Android.
It targets tablets and is organised around one workflow: write a FEMM
Python script (often with an AI assistant), paste it into the app, run it,
look at the results, and send any error back to the assistant.

## How it works

| Topic | Desktop | Android |
|---|---|---|
| Solvers (`fkn`, `belasolv`, `hsolv`, `csolv`) | executables next to `femm_gui` | the same executables, named `lib<name>.so` and packaged as native libraries, because an app may only execute files from its native library directory. libc++ is linked statically into them. |
| Finding the solvers | `exeDir() + name` | `solverExePath()` with `set_solver_dir_override(nativeLibraryDir)` |
| Material libraries, `python/` stdlib | next to the executable | APK assets under `femm/app`, extracted into private storage at start-up; `set_exe_dir_override()` points `exeDir()` there |
| Examples | `examples/` | copied into the app's `work` folder, which is the current directory |
| Scripting | MicroPython (in process) | same; the CPython/matplotlib bridge is not available |
| Lua 4 client, TCP clients | built | not built / not used |

Code: `gui/android_support.cpp`, `android/AndroidPackage.cmake`, and
`#ifdef __ANDROID__` blocks in `gui/main.cpp`, `gui/mainwindow.cpp` and
`gui/touch_nav.h`.

## Features added for the tablet workflow (all platforms)

* **Script panel** (`gui/script_panel.*`): a docked editor with line numbers
  and highlighting. Run / Stop / Paste / Open / Save, plus
  * **Copy Error**: the error message, its line, the nearby code and the output
    so far, ready to paste into an assistant;
  * **AI Prompt**: a description of FEMM's scripting environment
    (`gui/resources/ai_prompt.md`) to paste before describing the model.
  Shown by default on Android; on the desktop it is the *Script Editor* button
  in the left Script toolbar.
* `py_last_error_line()` in libfemm: the script line of the last error, without
  changing the error text.
* `femm_gui -selftest <script.py> <out.png>`: runs a script through the panel,
  saves a screenshot and `<out.png>.txt`, exits 0 on success.
* Two-finger pinch zoom and pan in the drawing and results views (Android).
* Tablet layout (`gui/tablet_mode.h`, `MainWindow::setupTabletMode`; on with
  Android, or `FEMM_TABLET=1` on the desktop): menus kept inside the window
  (Qt otherwise moves them into the Android action bar, where they did not
  show), the action bar hidden, toolbar icons doubled, and a labelled quick bar
  (Script, Mesh, Solve, Results, Values, Contour, Area, Plot, Integrate, Fit).
  Solve opens the results when it finishes and works from the results tab too.
* Plot X-Y and line integrals without a contour explain how to draw one, in a
  dialog, instead of a status-bar line (all platforms).

## Building

The APK is built by `.github/workflows/android.yml` on every push:

* Qt 6.8.3 for Android (installed with `aqtinstall`) plus the desktop Qt as
  the host tools, Android NDK r26b, JDK 17.
* Two APKs: `arm64-v8a` (tablets, the release APK) and `x86_64`
  (Chromebooks, and the emulator smoke test).
* The x86_64 APK is installed on a tablet emulator and runs
  `android/scripts/ai_demo_solenoid.py` through `-selftest`
  (`android/ci/smoke_test.sh`). The screenshot and log are uploaded as the
  `smoke-test` artifact.
* A tag `v*` attaches the arm64 APK to a GitHub release.

Locally, with Qt for Android and the NDK installed:

```bash
~/Qt/6.8.3/android_arm64_v8a/bin/qt-cmake -S . -B build-android -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DQT_HOST_PATH=$HOME/Qt/6.8.3/gcc_64 \
  -DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT -DANDROID_NDK_ROOT=$ANDROID_NDK_ROOT
cmake --build build-android
cmake --build build-android --target apk
```

### Release signing

Without secrets, CI signs with a throwaway key (the APK installs, but each
build has a different key, so updating needs an uninstall first). For a stable
key, create one once:

```bash
keytool -genkeypair -keystore femm-release.keystore -alias femm -keyalg RSA -keysize 2048 -validity 10000
```

and add these repository secrets (Settings > Secrets and variables > Actions):

* `ANDROID_KEYSTORE_BASE64`: output of `base64 -w0 femm-release.keystore`
* `ANDROID_KEYSTORE_PASSWORD`: the keystore password
* `ANDROID_KEY_ALIAS`: `femm`

Keep the keystore file safe; it cannot be recovered.
