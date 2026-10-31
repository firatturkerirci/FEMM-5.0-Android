// SPDX-License-Identifier: MIT
// Copyright (C) 2026 David Meeker and contributors
//
// dock_title.cpp - see dock_title.h.

#include "dock_title.h"
#include "tablet_mode.h"

#include <QDockWidget>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QToolButton>

namespace {

class TabletDockTitle : public QWidget
{
public:
    TabletDockTitle(QMainWindow *mw, QDockWidget *dock) : QWidget(dock), m_mw(mw), m_dock(dock)
    {
        setObjectName("TabletDockTitle");
        auto *h = new QHBoxLayout(this);
        h->setContentsMargins(10, 4, 4, 4);
        h->setSpacing(4);
        m_label = new QLabel(dock->windowTitle(), this);
        QFont f = m_label->font();
        f.setBold(true);
        m_label->setFont(f);
        h->addWidget(m_label, 1);
        m_left   = button("dockleft",   tr("Move to the left"));
        m_right  = button("dockright",  tr("Move to the right"));
        m_bottom = button("dockbottom", tr("Move to the bottom"));
        auto *close = button("close", tr("Close (reopen it from the quick bar)"));
        for (auto *b : {m_left, m_right, m_bottom, close}) h->addWidget(b);
        connect(m_left,   &QToolButton::clicked, this, [this]() { moveTo(Qt::LeftDockWidgetArea); });
        connect(m_right,  &QToolButton::clicked, this, [this]() { moveTo(Qt::RightDockWidgetArea); });
        connect(m_bottom, &QToolButton::clicked, this, [this]() { moveTo(Qt::BottomDockWidgetArea); });
        connect(close, &QToolButton::clicked, dock, &QDockWidget::close);
        connect(dock, &QDockWidget::dockLocationChanged, this, [this]() { syncButtons(); });
        connect(dock, &QDockWidget::windowTitleChanged, m_label, &QLabel::setText);
        syncButtons();
    }

private:
    QToolButton *button(const char *icon, const QString &tip)
    {
        auto *b = new QToolButton(this);
        b->setIcon(femmTabletIcon(QString::fromLatin1(icon)));
        b->setIconSize(QSize(24, 24));
        b->setFixedSize(40, 40);
        b->setToolTip(tip);
        b->setAutoRaise(true);
        if (b->icon().isNull()) b->setText(tip.left(1));
        return b;
    }

    void moveTo(Qt::DockWidgetArea area)
    {
        m_mw->addDockWidget(area, m_dock);   // re-adding moves it
        m_dock->show();
        m_dock->raise();
        // A useful size for the new place: a column beside the model, or a
        // strip under it.
        if (area == Qt::BottomDockWidgetArea)
            m_mw->resizeDocks({m_dock}, {m_mw->height() * 35 / 100}, Qt::Vertical);
        else
            m_mw->resizeDocks({m_dock}, {m_mw->width() * 40 / 100}, Qt::Horizontal);
        syncButtons();
    }

    void syncButtons()
    {
        const Qt::DockWidgetArea a = m_mw->dockWidgetArea(m_dock);
        m_left->setEnabled(a != Qt::LeftDockWidgetArea);
        m_right->setEnabled(a != Qt::RightDockWidgetArea);
        m_bottom->setEnabled(a != Qt::BottomDockWidgetArea);
    }

    QMainWindow *m_mw;
    QDockWidget *m_dock;
    QLabel *m_label;
    QToolButton *m_left, *m_right, *m_bottom;
};

} // namespace

void femmInstallTabletDockTitle(QMainWindow *mw, QDockWidget *dock)
{
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    dock->setTitleBarWidget(new TabletDockTitle(mw, dock));
}
