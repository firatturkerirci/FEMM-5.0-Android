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
#include <QPainter>
#include <QProxyStyle>
#include <QStyleOption>
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

// Touch sizes, given through a proxy style rather than a style sheet: an
// application-wide style sheet makes Qt re-style widgets on every state change
// (each toolbar click, each dialog), which was slow on a tablet.
class TabletStyle : public QProxyStyle
{
public:
    explicit TabletStyle(QStyle *base) : QProxyStyle(base) {}

    int styleHint(StyleHint h, const QStyleOption *opt, const QWidget *w,
                  QStyleHintReturn *ret) const override
    {
        switch (h) {
        // A combo box opens a scrolling list right under itself, not a
        // menu-like popup over the whole screen, and the highlight does not
        // follow a finger dragged across the list.
        case SH_ComboBox_Popup:              return 0;
        case SH_ComboBox_ListMouseTracking:  return 0;
        case SH_Menu_MouseTracking:          return 0;
        default: return QProxyStyle::styleHint(h, opt, w, ret);
        }
    }

    int pixelMetric(PixelMetric m, const QStyleOption *opt, const QWidget *w) const override
    {
        switch (m) {
        case PM_ButtonMargin:              return 12;
        case PM_IndicatorWidth:
        case PM_IndicatorHeight:
        case PM_ExclusiveIndicatorWidth:
        case PM_ExclusiveIndicatorHeight:  return 22;
        case PM_ScrollBarExtent:           return 14;
        case PM_SplitterWidth:
        case PM_DockWidgetSeparatorExtent: return 10;
        case PM_ToolBarItemSpacing:        return 4;
        case PM_MenuHMargin:
        case PM_MenuVMargin:               return 6;
        case PM_SmallIconSize:             return 20;
        default: return QProxyStyle::pixelMetric(m, opt, w);
        }
    }

    static bool onQuickBar(const QWidget *w)
    {
        return w && w->parentWidget() && w->parentWidget()->objectName() == QLatin1String("QuickBar");
    }

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w) const override
    {
        if (pe == PE_PanelButtonTool) {
            const bool on = opt->state & (State_On | State_Sunken);
            if (onQuickBar(w) || on) {
                p->save();
                p->setRenderHint(QPainter::Antialiasing);
                const QRectF r = QRectF(opt->rect).adjusted(1.5, 1.5, -1.5, -1.5);
                if (on) {
                    p->setPen(Qt::NoPen);
                    p->setBrush(opt->palette.highlight());
                } else {
                    p->setPen(QPen(opt->palette.mid().color(), 1));
                    p->setBrush(opt->palette.button());
                }
                p->drawRoundedRect(r, 7, 7);
                p->restore();
                return;
            }
        }
        QProxyStyle::drawPrimitive(pe, opt, p, w);
    }

    QSize sizeFromContents(ContentsType t, const QStyleOption *opt, const QSize &cs,
                           const QWidget *w) const override
    {
        QSize r = QProxyStyle::sizeFromContents(t, opt, cs, w);
        if (t == CT_ToolButton && onQuickBar(w)) {
            r.rwidth() += 20;
            r.setHeight(qMax(r.height(), 44));
            return r;
        }
        auto atLeast = [&r](int h) { r.setHeight(qMax(r.height(), h)); };
        switch (t) {
        case CT_PushButton:   atLeast(40); r.setWidth(qMax(r.width(), 88)); break;
        case CT_LineEdit:
        case CT_ComboBox:
        case CT_SpinBox:      atLeast(38); break;
        case CT_CheckBox:
        case CT_RadioButton:  atLeast(34); break;
        case CT_TabBarTab:    atLeast(38); r.rwidth() += 12; break;
        case CT_MenuBarItem:  atLeast(40); r.rwidth() += 16; break;
        case CT_ItemViewItem: atLeast(34); break;
        case CT_MenuItem: {
            const auto *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
            if (!mi || mi->menuItemType != QStyleOptionMenuItem::Separator) atLeast(40);
            break;
        }
        default: break;
        }
        return r;
    }
};

// Dialogs used to open small: give them a tablet-friendly minimum width
// before they are shown. Qt centres a dialog over its parent by itself.
// (Resizing or moving one after it appears could leave it undrawn on Android
// until the app was sent to the background and back.)
class DialogWidener : public QObject
{
protected:
    bool eventFilter(QObject *obj, QEvent *e) override
    {
        if (e->type() == QEvent::Polish) {
            auto *dlg = qobject_cast<QDialog *>(obj);
            if (dlg && !qobject_cast<QFileDialog *>(dlg)) {
                const QScreen *sc = dlg->screen();
                const int maxW = sc ? sc->availableGeometry().width() * 9 / 10 : 520;
                dlg->setMinimumWidth(qMin(520, maxW));
            }
        }
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
    app.setStyle(new TabletStyle(QStyleFactory::create("Fusion")));
    app.setPalette(makePalette(dark ? kDark : kLight));
    app.installEventFilter(new DialogWidener);
}
