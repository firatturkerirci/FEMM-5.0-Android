// SPDX-License-Identifier: MIT
// Copyright (C) 2026 David Meeker and contributors
//
// touch_nav.h - two-finger navigation for the drawing and results views on a
// touch screen: pinch to zoom about the fingers, drag with two fingers to pan.
// One-finger touches are left alone, so Qt turns them into the mouse clicks
// the views already understand. Active on Android only.
#pragma once

#include <QEvent>
#include <QGestureEvent>
#include <QObject>
#include <QPinchGesture>
#include <QWidget>
#include <functional>

class TouchNav : public QObject
{
public:
    // fn(anchor, factor, moved): zoom by `factor` about widget point `anchor`,
    // then shift the view so the anchor follows the fingers by `moved` pixels.
    using Fn = std::function<void(const QPointF &, double, const QPointF &)>;

    static void attach(QWidget *w, Fn fn)
    {
#ifdef __ANDROID__
        new TouchNav(w, std::move(fn));
#else
        Q_UNUSED(w);
        Q_UNUSED(fn);
#endif
    }

protected:
    bool eventFilter(QObject *obj, QEvent *e) override
    {
        if (obj != m_w || e->type() != QEvent::Gesture) return false;
        auto *ge = static_cast<QGestureEvent *>(e);
        auto *p = static_cast<QPinchGesture *>(ge->gesture(Qt::PinchGesture));
        if (!p) return false;
        const QPointF c  = m_w->mapFromGlobal(p->centerPoint());
        const QPointF lc = m_w->mapFromGlobal(p->lastCenterPoint());
        if (p->state() == Qt::GestureStarted) {
            m_fn(c, 1.0, QPointF());
        } else {
            double f = p->scaleFactor();
            if (!(f > 0.2 && f < 5.0)) f = 1.0;   // ignore a wild first sample
            m_fn(lc, f, c - lc);
        }
        ge->accept(p);
        return true;
    }

private:
    TouchNav(QWidget *w, Fn fn) : QObject(w), m_w(w), m_fn(std::move(fn))
    {
        w->setAttribute(Qt::WA_AcceptTouchEvents);
        w->grabGesture(Qt::PinchGesture);
        w->installEventFilter(this);
    }

    QWidget *m_w;
    Fn m_fn;
};
