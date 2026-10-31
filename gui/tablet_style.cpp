// SPDX-License-Identifier: MIT
// Copyright (C) 2026 David Meeker and contributors
//
// tablet_style.cpp - see tablet_style.h.

#include "tablet_style.h"

#include <QApplication>
#include <QDialog>
#include <QEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QPalette>
#include <QScreen>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTimer>
#include <QWidget>

namespace {

struct Tones {
    const char *window, *base, *alt, *text, *muted, *button, *mid, *accent, *onAccent;
};
// The same colours as the two-tone icons (tools/gen_tablet_icons.py).
const Tones kLight = {"#f4f6f7", "#ffffff", "#eef2f3", "#16232a", "#5b6b73",
                      "#e9eef0", "#c9d3d7", "#0b7f86", "#ffffff"};
const Tones kDark  = {"#0f1619", "#172126", "#1c282e", "#e3eaed", "#93a4ab",
                      "#213037", "#34464e", "#3cc4cb", "#0f1619"};

QPalette makePalette(const Tones &t)
{
    QPalette p;
    auto set = [&p](QPalette::ColorRole r, const char *c) { p.setColor(r, QColor(c)); };
    set(QPalette::Window, t.window);
    set(QPalette::WindowText, t.text);
    set(QPalette::Base, t.base);
    set(QPalette::AlternateBase, t.alt);
    set(QPalette::Text, t.text);
    set(QPalette::Button, t.button);
    set(QPalette::ButtonText, t.text);
    set(QPalette::Highlight, t.accent);
    set(QPalette::HighlightedText, t.onAccent);
    set(QPalette::ToolTipBase, t.text);
    set(QPalette::ToolTipText, t.base);
    set(QPalette::PlaceholderText, t.muted);
    set(QPalette::Mid, t.mid);
    set(QPalette::Midlight, t.alt);
    set(QPalette::Light, t.base);
    set(QPalette::Dark, t.mid);
    set(QPalette::Shadow, "#000000");
    set(QPalette::Link, t.accent);
    set(QPalette::BrightText, t.onAccent);
    for (auto r : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        p.setColor(QPalette::Disabled, r, QColor(t.muted));
    return p;
}

// Controls sized for a finger or a pen, rounded corners, flat surfaces.
const char *kStyleSheet = R"(
QPushButton {
    min-height: 34px; padding: 4px 18px;
    border: 1px solid palette(mid); border-radius: 8px;
    background: palette(button);
}
QPushButton:hover { border-color: palette(highlight); }
QPushButton:pressed, QPushButton:checked {
    background: palette(highlight); color: palette(highlighted-text); border-color: palette(highlight);
}
QPushButton:default { border: 2px solid palette(highlight); }
QPushButton:disabled { color: palette(placeholder-text); }

QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QPlainTextEdit, QTextEdit {
    border: 1px solid palette(mid); border-radius: 6px; background: palette(base);
    selection-background-color: palette(highlight); selection-color: palette(highlighted-text);
}
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { min-height: 32px; padding: 0 8px; }
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus,
QPlainTextEdit:focus, QTextEdit:focus { border: 2px solid palette(highlight); }
QComboBox::drop-down { width: 28px; border: 0; }
QComboBox QAbstractItemView { border: 1px solid palette(mid); border-radius: 6px; padding: 4px; }

QCheckBox, QRadioButton { spacing: 10px; min-height: 30px; }
QCheckBox::indicator, QRadioButton::indicator { width: 20px; height: 20px; }

QGroupBox {
    border: 1px solid palette(mid); border-radius: 10px;
    margin-top: 16px; padding: 12px 10px 10px 10px;
}
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: palette(placeholder-text); }

QTabWidget::pane { border: 1px solid palette(mid); border-radius: 8px; top: -1px; }
QTabBar::tab {
    min-height: 32px; padding: 4px 16px; margin: 2px;
    border: 1px solid transparent; border-radius: 8px; color: palette(placeholder-text);
}
QTabBar::tab:selected { background: palette(button); border-color: palette(mid); color: palette(text); }

QMenuBar { padding: 2px 4px; }
QMenuBar::item { padding: 8px 14px; border-radius: 6px; background: transparent; }
QMenuBar::item:selected, QMenuBar::item:pressed { background: palette(button); }
QMenu { padding: 6px; border: 1px solid palette(mid); border-radius: 10px; background: palette(base); }
QMenu::item { padding: 9px 28px 9px 14px; border-radius: 6px; }
QMenu::item:selected { background: palette(highlight); color: palette(highlighted-text); }
QMenu::separator { height: 1px; background: palette(mid); margin: 6px 8px; }

QMainWindow::separator { background: palette(mid); width: 8px; height: 8px; }
QMainWindow::separator:hover, QMainWindow::separator:pressed { background: palette(highlight); }
#TabletDockTitle { background: palette(window); border-bottom: 1px solid palette(mid); }
QToolTip { border: 0; border-radius: 6px; padding: 6px 10px; }
QStatusBar { border-top: 1px solid palette(mid); }
QDockWidget::title { padding: 8px 10px; background: palette(window); }

QHeaderView::section { padding: 6px 8px; border: 0; border-bottom: 1px solid palette(mid); background: palette(button); }
QTreeView, QListView, QTableView { border: 1px solid palette(mid); border-radius: 8px; }
QTreeView::item, QListView::item { padding: 6px 4px; }

QScrollBar:vertical { width: 10px; background: transparent; margin: 2px; }
QScrollBar:horizontal { height: 10px; background: transparent; margin: 2px; }
QScrollBar::handle { background: palette(mid); border-radius: 4px; }
QScrollBar::handle:vertical { min-height: 40px; }
QScrollBar::handle:horizontal { min-width: 40px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
QSlider::groove:horizontal { height: 6px; border-radius: 3px; background: palette(mid); }
QSlider::handle:horizontal { width: 22px; height: 22px; margin: -8px 0; border-radius: 11px; background: palette(highlight); }
)";

// Dialogs open small and in a corner on a tablet: give them room and centre
// them over the main window.
class DialogFitter : public QObject
{
protected:
    bool eventFilter(QObject *obj, QEvent *e) override
    {
        if (e->type() != QEvent::Show) return false;
        auto *dlg = qobject_cast<QDialog *>(obj);
        if (!dlg || qobject_cast<QFileDialog *>(dlg) || dlg->property("femmFitted").toBool())
            return false;
        dlg->setProperty("femmFitted", true);
        QTimer::singleShot(0, dlg, [dlg]() {
            QWidget *host = dlg->parentWidget() ? dlg->parentWidget()->window() : nullptr;
            const QRect area = host ? host->geometry()
                                    : (dlg->screen() ? dlg->screen()->availableGeometry() : QRect());
            if (area.isEmpty()) return;
            const int w = qMin(qMax(dlg->width(), 520), area.width() * 9 / 10);
            const int h = qMin(dlg->height(), area.height() * 9 / 10);
            dlg->resize(w, h);
            dlg->move(area.center() - QPoint(w / 2, h / 2));
        });
        return false;
    }
};

} // namespace

void femmApplyTabletStyle(QApplication &app, int dark)
{
    if (dark < 0) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark ? 1 : 0;
#else
        dark = app.palette().color(QPalette::Window).lightness() < 128 ? 1 : 0;
#endif
    }
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setPalette(makePalette(dark ? kDark : kLight));
    app.setStyleSheet(QString::fromLatin1(kStyleSheet));
    app.installEventFilter(new DialogFitter);
}
