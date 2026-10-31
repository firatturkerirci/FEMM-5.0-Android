// SPDX-License-Identifier: MIT
// Copyright (C) 2026 David Meeker and contributors
//
// dock_title.h - a touch-friendly title bar for the script panel and the
// Python console in tablet mode: the panel's name and four large buttons to
// put it on the left, right or bottom, or close it. Dragging a dock with a
// finger is fiddly, and a floating dock is a separate window on Android, so
// floating is turned off and the buttons do the moving.
#pragma once

class QMainWindow;
class QDockWidget;

void femmInstallTabletDockTitle(QMainWindow *mw, QDockWidget *dock);
