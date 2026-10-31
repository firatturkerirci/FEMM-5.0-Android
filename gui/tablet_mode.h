// SPDX-License-Identifier: MIT
// Copyright (C) 2026 David Meeker and contributors
//
// tablet_mode.h - whether to use the touch-friendly layout: always on Android;
// on the desktop when FEMM_TABLET is set (for trying the layout out).
#pragma once

#include <QMessageBox>
#include <QWidget>
#include <QtGlobal>

inline bool femmTabletMode()
{
#ifdef __ANDROID__
    return true;
#else
    static const bool on = qEnvironmentVariableIsSet("FEMM_TABLET");
    return on;
#endif
}

// Shown when Plot X-Y or a line integral is asked for before a contour exists.
// A status-bar line is easy to miss, and on a tablet there is nothing else to
// say what to do next.
inline void femmContourHint(QWidget *parent)
{
    QMessageBox::information(parent, QObject::tr("Define a contour first"),
        QObject::tr("Plot X-Y and line integrals work along a contour (a path "
                    "through the results).\n\n"
                    "1. Choose Contour mode.\n"
                    "2. Tap or click points on the results view. A tap or left "
                    "click snaps to the nearest mesh node; a right click or the "
                    "pen's side button puts the point exactly where you press.\n"
                    "3. Press Plot X-Y or Integrate again."));
}
