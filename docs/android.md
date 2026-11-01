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

## Launcher icon

An adaptive icon (`android/package/res/mipmap-anydpi-v26/ic_launcher.xml`): the
name with a copper wire wound loosely around it. The background colour and the
foreground layer follow the system's light/dark mode (`values-night`,
`mipmap-night-*`), and a monochrome layer serves Android 13 themed icons. The
PNG layers are drawn by `node tools/app_icon/render.js` (Playwright Chromium,
Barlow from Google Fonts); rerun it after changing the drawing.

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

`android/release.keystore.enc` is the release key, encrypted with
`openssl enc -aes-256-cbc -pbkdf2 -iter 200000`. CI decrypts it with the
repository secret `FEMM_KEYSTORE_PASSWORD` (the same password protects the
key, alias `femm`). The file does not exist until the secret is added: the
first arm64 build after that creates the key, encrypts it with the secret and
commits it to the branch, so nobody but the repository owner ever sees the
password. With the secret set, every build carries the same
signature, so a new APK installs over the old one and the app keeps its files.
Without it, CI signs with a throwaway key and updating needs an uninstall.

Keep the password somewhere safe: without it no update can be signed with this
key, and a new key means one more uninstall for everyone.
