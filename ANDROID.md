# FEMM 5.0 for Android (unofficial)

An Android build of [FEMM 5.0](https://github.com/dcm3c/femm-qt) for tablets.
This is an **unofficial** port maintained by
[@firatturkerirci](https://github.com/firatturkerirci); it is not published or
supported by David Meeker, the author of FEMM. Report problems with the Android
build here, not to the FEMM user group.

The solvers, the document model and the file formats are FEMM's own and are
unchanged; the port adds a tablet layer around them. Technical notes:
[docs/android.md](docs/android.md).

## What it adds

* **Script panel**: paste, open or share a pyFEMM script, run it, see the
  output. An error highlights its line, and **Copy Error** puts a report (the
  message, the line, the code around it, the output so far) on the clipboard.
* **AI Prompt**: copies a description of FEMM's scripting environment
  (MicroPython, the pyFEMM API, the material names) to paste into an AI
  assistant before describing a model, so the script it writes runs in FEMM.
* **Tablet layout**: menus and a quick bar (Mesh, Solve, Results, Contour,
  Plot, Integrate, ...) as large buttons, two-finger pinch and pan, panels
  that move to the left, right or bottom with one tap, and a light/dark theme
  that follows the system.
* **Sessions survive**: open models and the script are saved when the app goes
  to the background and come back if Android stopped it.

A physical keyboard and mouse or pen work as on the desktop.

## Install

1. On the tablet, download the newest APK:
   <https://github.com/firatturkerirci/FEMM-5.0-Android/releases/latest/download/femm-android-arm64-v8a.apk>
   (all versions: [Releases](https://github.com/firatturkerirci/FEMM-5.0-Android/releases)).
2. Open it. If Android asks, allow installing apps from that source.
3. Builds are not yet signed with a fixed key, so remove an older version
   before installing a newer one. Save any models you want to keep first.

Requires Android 9 or later on a 64-bit ARM device (arm64-v8a). An x86_64
build for Chromebooks is attached to each CI run.

## Using it with an AI assistant

1. Press **AI Prompt** and paste the text into your assistant, then describe
   the model after it.
2. Copy the script it returns and press **Paste** (code fences are removed).
3. Press **Run**. The model is drawn, meshed and solved; printed values appear
   under the editor.
4. On an error, press **Copy Error**, paste it into the assistant, and paste
   the corrected script back.

## Limits

* Scripts run in FEMM's embedded MicroPython: matplotlib, SciPy and pandas are
  not available; numpy is the ulab subset.
* Driving FEMM over TCP from MATLAB, Octave or an external Python is not
  supported on Android.

## License

As FEMM 5.0 (femm-qt): the application is MIT, the solvers are
GPL-3.0-or-later, Qt is LGPL-3.0. The full source for every APK is this
repository. See [README.md](README.md) and [LICENSE](LICENSE).
