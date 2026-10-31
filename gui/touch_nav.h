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

#include <QElapsedTimer>
#include <QEvent>
#include <QGestureEvent>
#include <QObject>
#include <QPainter>
#include <QPinchGesture>
#include <QPixmap>
#include <QTimer>
#include <QWidget>
#include <functional>

#include "tablet_mode.h"

class TouchNav : public QObject
{
public:
    // fn(anchor, factor, moved): zoom by `factor` about widget point `anchor`,
    // then shift the view so the anchor follows the fingers by `moved` pixels.
    using Fn = std::function<void(const QPointF &, double, const QPointF &)>;

    static void attach(QWidget *w, Fn fn);

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

// While a view is being resized (a panel dragged, opened or closed beside it)
// it would redraw the whole model for every step. Instead the first step
// takes one picture, the following steps show that picture, and the real
// redraw happens once, shortly after the resizing stops. Tablet mode only.
class ResizeSmoother : public QObject
{
public:
    explicit ResizeSmoother(QWidget *w) : QObject(w), m_w(w)
    {
        m_timer.setSingleShot(true);
        m_timer.setInterval(160);
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_active = false;
            m_snapshot = QPixmap();
            m_w->update();
        });
        w->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *obj, QEvent *e) override
    {
        if (obj != m_w) return false;
        if (e->type() == QEvent::Resize && m_w->isVisible()) {
            // A single resize (a tab switch, a panel opened) just repaints
            // normally: taking a picture first would cost a second full
            // drawing. Only a second resize close behind the first (a drag)
            // switches to the picture.
            const qint64 now = m_clock.isValid() ? m_clock.elapsed() : -1;
            if (!m_clock.isValid()) m_clock.start();
            const bool burst = now >= 0 && now - m_lastResize < 250;
            m_lastResize = m_clock.elapsed();
            if (!burst && !m_active) return false;
            if (!m_active) {
                m_active = true;
                m_snapshot = m_w->grab();
                m_snapshot.setDevicePixelRatio(m_w->devicePixelRatioF());
                const QImage corner = m_snapshot.copy(0, 0, 1, 1).toImage();
                m_background = corner.isNull() ? m_w->palette().color(QPalette::Base)
                                               : corner.pixelColor(0, 0);
            }
            m_timer.start();
            return false;
        }
        if (e->type() == QEvent::Paint && m_active && !m_snapshot.isNull()) {
            QPainter p(m_w);
            p.fillRect(m_w->rect(), m_background);
            p.drawPixmap(QPointF(0, 0), m_snapshot);
            return true;   // the real paint comes when the timer fires
        }
        return false;
    }

private:
    QWidget *m_w;
    QTimer m_timer;
    QPixmap m_snapshot;
    QColor m_background;
    bool m_active = false;
    QElapsedTimer m_clock;
    qint64 m_lastResize = -1000;
};

inline void TouchNav::attach(QWidget *w, Fn fn)
{
#ifdef __ANDROID__
    new TouchNav(w, std::move(fn));
#else
    Q_UNUSED(fn);
#endif
    if (femmTabletMode()) new ResizeSmoother(w);
}
