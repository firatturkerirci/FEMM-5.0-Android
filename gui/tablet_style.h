// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Fırat Türkerirci and contributors
//
// tablet_style.h - the tablet look: Fusion style, a palette that follows the
// system's light or dark mode, touch-sized controls, and dialogs sized and
// centred for a tablet screen. Applied in tablet mode only (tablet_mode.h).
#pragma once

class QApplication;

// Call right after QApplication is constructed, before any window exists.
// dark < 0: follow the system; 0: light; 1: dark.
void femmApplyTabletStyle(QApplication &app, int dark = -1);
