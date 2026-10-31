// SPDX-License-Identifier: MIT
// Copyright (C) 2026 David Meeker and contributors
//
// tablet_mode.h - whether to use the touch-friendly layout: always on Android;
// on the desktop when FEMM_TABLET is set (for trying the layout out).
#pragma once

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
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

// The two-tone tablet icon for a classic icon name (gui/icons/tablet/, made
// by tools/gen_tablet_icons.py), in the set that matches the current theme.
// A null QIcon when there is none, or when Qt cannot draw SVG here.
inline QIcon femmTabletIcon(const QString &name)
{
    const bool dark = qApp && qApp->palette().color(QPalette::Window).lightness() < 128;
    const QString path = QString(":/tablet/%1/%2.svg").arg(dark ? "dark" : "light", name);
    if (!QFile::exists(path)) return QIcon();
    QIcon ic(path);
    if (ic.pixmap(QSize(32, 32)).isNull()) return QIcon();   // no SVG icon engine
    return ic;
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
