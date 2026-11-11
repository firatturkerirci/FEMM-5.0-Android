// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Fırat Türker Ircı and contributors
//
// android_support.h - Android start-up for femm_gui (no-op elsewhere).
#pragma once

#include <QString>

#ifdef __ANDROID__
// Extract the shipped data, point libfemm at it and at the packaged solvers,
// and make the work folder the current directory. Call right after
// QApplication is constructed and before init_py().
void femmAndroidSetup();
// The folder holding the user's models and scripts.
QString femmAndroidWorkDir();
// Text shared to FEMM from another app (Share > FEMM, or opening a .py),
// or empty. Each share is returned once.
QString femmAndroidTakeSharedText();
// Hide the Android action bar (the title strip above the window).
void femmAndroidHideActionBar();
// The app's external files folder (readable over adb); empty if unavailable.
QString femmAndroidExternalDir();
#endif
