// SPDX-License-Identifier: MIT
// Copyright (C) 2026 David Meeker and contributors
//
// touch_nav.h - two-finger navigation for the drawing and results views on a
// touch screen: pinch to zoom about the fingers, drag with two fingers to pan.
// One-finger touches are left alone, so Qt turns them into the mouse clicks
// the views already understand. Active on Android only.
//
// Redrawing a solved model (thousands of shaded triangles, flux lines) for
// every finger movement made pinching stutter on a tablet. So the view is
// drawn once when the gesture starts; while the fingers move, only that
// picture is scaled and moved, and the view is redrawn for real once, when
// the fingers lift.
#pragma once

#include <QEvent>
#include <QGestureEvent>
#include <QObject>
#include <QPainter>
#include <QPinchGesture>
#include <QPixmap>
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

        switch (p->state()) {
        case Qt::GestureStarted:
            begin();
            break;
        case Qt::GestureUpdated: {
            const QPointF c  = m_w->mapFromGlobal(p->centerPoint());
            const QPointF lc = m_w->mapFromGlobal(p->lastCenterPoint());
            double f = p->scaleFactor();
            if (!(f > 0.2 && f < 5.0)) f = 1.0;   // ignore a wild sample
            // Screen point q goes to lc + (c - lc) + f*(q - lc); compose it
            // onto the transform accumulated so far (q -> s*q + t).
            const QPointF b = c - f * lc;
            m_s *= f;
            m_t = f * m_t + b;
            if (m_overlay) m_overlay->update();
            break;
        }
        case Qt::GestureFinished:
        case Qt::GestureCanceled:
            end();
            break;
        default:
            break;
        }
        ge->accept(p);
        return true;
    }

private:
    // Paints the snapshot under the accumulated transform.
    class Overlay : public QWidget
    {
    public:
        Overlay(TouchNav *nav, QWidget *parent) : QWidget(parent), m_nav(nav)
        {
            setAttribute(Qt::WA_TransparentForMouseEvents);
            setAttribute(Qt::WA_OpaquePaintEvent);
        }
    protected:
        void paintEvent(QPaintEvent *) override
        {
            QPainter p(this);
            p.fillRect(rect(), m_nav->m_background);
            p.setRenderHint(QPainter::SmoothPixmapTransform, false);
            p.translate(m_nav->m_t);
            p.scale(m_nav->m_s, m_nav->m_s);
            p.drawPixmap(QPointF(0, 0), m_nav->m_snapshot);
        }
    private:
        TouchNav *m_nav;
    };

    TouchNav(QWidget *w, Fn fn) : QObject(w), m_w(w), m_fn(std::move(fn))
    {
        w->setAttribute(Qt::WA_AcceptTouchEvents);
        w->grabGesture(Qt::PinchGesture);
        w->installEventFilter(this);
    }

    void begin()
    {
        m_s = 1.0;
        m_t = QPointF();
        m_snapshot = m_w->grab();
        // Drawn device-independent: grab() returns a high-DPI pixmap.
        m_snapshot.setDevicePixelRatio(m_w->devicePixelRatioF());
        const QImage corner = m_snapshot.copy(0, 0, 1, 1).toImage();
        m_background = corner.isNull() ? m_w->palette().color(QPalette::Base)
                                       : corner.pixelColor(0, 0);
        if (!m_overlay) m_overlay = new Overlay(this, m_w);
        m_overlay->setGeometry(m_w->rect());
        m_overlay->show();
        m_overlay->raise();
        m_active = true;
    }

    void end()
    {
        if (!m_active) return;
        m_active = false;
        if (m_overlay) m_overlay->hide();
        m_snapshot = QPixmap();
        // The whole gesture as one step: q -> s*q + t is a zoom by s about the
        // widget origin followed by a move of t.
        if (m_s != 1.0 || !m_t.isNull())
            m_fn(QPointF(0, 0), m_s, m_t);
    }

    QWidget *m_w;
    Fn m_fn;
    Overlay *m_overlay = nullptr;
    QPixmap m_snapshot;
    QColor m_background;
    double m_s = 1.0;
    QPointF m_t;
    bool m_active = false;
};
