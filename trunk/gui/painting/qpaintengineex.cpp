/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpaintengineex_p.h"
#include "qpainter_p.h"
#include "qstroker_p.h"
#include <private/qpainterpath_p.h>

// #include <qvarlengtharray.h>
#include "core/qvarlengtharray.h"
// #include <qdebug.h>


QT_BEGIN_NAMESPACE

/*******************************************************************************
 *
 * class QVectorPath
 *
 */

const QRealRect &QVectorPath::controlPointRect() const
{
    if (m_hints & ControlPointRect)
        return m_cp_rect;

    if (m_count == 0) {
        m_cp_rect.x1 = m_cp_rect.x2 = m_cp_rect.y1 = m_cp_rect.y2 = 0;
        m_hints |= ControlPointRect;
        return m_cp_rect;
    }
    Q_ASSERT(m_points && m_count > 0);

    const qreal *pts = m_points;
    m_cp_rect.x1 = m_cp_rect.x2 = *pts;
    ++pts;
    m_cp_rect.y1 = m_cp_rect.y2 = *pts;
    ++pts;

    const qreal *epts = m_points + (m_count << 1);
    while (pts < epts) {
        qreal x = *pts;
        if (x < m_cp_rect.x1) m_cp_rect.x1 = x;
        else if (x > m_cp_rect.x2) m_cp_rect.x2 = x;
        ++pts;

        qreal y = *pts;
        if (y < m_cp_rect.y1) m_cp_rect.y1 = y;
        else if (y > m_cp_rect.y2) m_cp_rect.y2 = y;
        ++pts;
    }

    m_hints |= ControlPointRect;
    return m_cp_rect;
}

const QVectorPath &qtVectorPathForPath(const QPainterPath &path)
{
    Q_ASSERT(path.d_func());
    return path.d_func()->vectorPath();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug Q_GUI_EXPORT &operator<<(QDebug &s, const QVectorPath &path)
{
    QRealRect vectorPathBounds = path.controlPointRect();
    QRectF rf(vectorPathBounds.x1, vectorPathBounds.y1,
              vectorPathBounds.x2 - vectorPathBounds.x1, vectorPathBounds.y2 - vectorPathBounds.y1);
    s << "QVectorPath(size:" << path.elementCount()
      << " hints:" << hex << path.hints()
      << rf << ")";
    return s;
}
#endif

/*******************************************************************************
 *
 * class QPaintEngineExPrivate:
 *
 */


struct StrokeHandler {
    QDataBuffer<qreal> pts;
    QDataBuffer<QPainterPath::ElementType> types;
};


QPaintEngineExPrivate::QPaintEngineExPrivate()
    : dasher(&stroker),
      strokeHandler(0),
      activeStroker(0),
      strokerPen(Qt::NoPen)
{
}


QPaintEngineExPrivate::~QPaintEngineExPrivate()
{
    delete strokeHandler;
}



/*******************************************************************************
 *
 * class QPaintEngineEx:
 *
 */

static QPainterPath::ElementType qpaintengineex_ellipse_types[] = {
    QPainterPath::MoveToElement,
    QPainterPath::CurveToElement,
    QPainterPath::CurveToDataElement,
    QPainterPath::CurveToDataElement,

    QPainterPath::CurveToElement,
    QPainterPath::CurveToDataElement,
    QPainterPath::CurveToDataElement,

    QPainterPath::CurveToElement,
    QPainterPath::CurveToDataElement,
    QPainterPath::CurveToDataElement,

    QPainterPath::CurveToElement,
    QPainterPath::CurveToDataElement,
    QPainterPath::CurveToDataElement
};

static QPainterPath::ElementType qpaintengineex_line_types_16[] = {
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement,
    QPainterPath::MoveToElement, QPainterPath::LineToElement
};

static QPainterPath::ElementType qpaintengineex_rect4_types_32[] = {
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 1
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 2
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 3
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 4
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 5
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 6
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 7
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 8
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 9
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 10
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 11
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 12
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 13
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 14
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 15
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 16
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 17
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 18
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 19
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 20
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 21
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 22
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 23
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 24
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 25
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 26
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 27
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 28
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 29
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 30
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 31
    QPainterPath::MoveToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, QPainterPath::LineToElement, // 32
};

static void qpaintengineex_moveTo(qreal x, qreal y, void *data) {
    ((StrokeHandler *) data)->pts.add(x);
    ((StrokeHandler *) data)->pts.add(y);
    ((StrokeHandler *) data)->types.add(QPainterPath::MoveToElement);
}

static void qpaintengineex_lineTo(qreal x, qreal y, void *data) {
    ((StrokeHandler *) data)->pts.add(x);
    ((StrokeHandler *) data)->pts.add(y);
    ((StrokeHandler *) data)->types.add(QPainterPath::LineToElement);
}

static void qpaintengineex_cubicTo(qreal c1x, qreal c1y, qreal c2x, qreal c2y, qreal ex, qreal ey, void *data) {
    ((StrokeHandler *) data)->pts.add(c1x);
    ((StrokeHandler *) data)->pts.add(c1y);
    ((StrokeHandler *) data)->types.add(QPainterPath::CurveToElement);

    ((StrokeHandler *) data)->pts.add(c2x);
    ((StrokeHandler *) data)->pts.add(c2y);
    ((StrokeHandler *) data)->types.add(QPainterPath::CurveToDataElement);

    ((StrokeHandler *) data)->pts.add(ex);
    ((StrokeHandler *) data)->pts.add(ey);
    ((StrokeHandler *) data)->types.add(QPainterPath::CurveToDataElement);
}

QPaintEngineEx::QPaintEngineEx(QPaintEngineExPrivate &data)
    : QPaintEngine(data, AllFeatures)
{
    extended = true;
}


QPainterState *QPaintEngineEx::createState(QPainterState *orig) const
{
    if (!orig)
        return new QPainterState;
    return new QPainterState(orig);
}

extern bool qt_scaleForTransform(const QTransform &transform, qreal *scale); // qtransform.cpp

void QPaintEngineEx::stroke(const QVectorPath &path, const QPen &pen)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << "QPaintEngineEx::stroke()" << pen;
#endif

    Q_D(QPaintEngineEx);

    if (path.isEmpty())
        return;

    if (!d->strokeHandler) {
        d->strokeHandler = new StrokeHandler;
        d->stroker.setMoveToHook(qpaintengineex_moveTo);
        d->stroker.setLineToHook(qpaintengineex_lineTo);
        d->stroker.setCubicToHook(qpaintengineex_cubicTo);
    }

    if (!qpen_fast_equals(pen, d->strokerPen)) {
        d->strokerPen = pen;
        d->stroker.setJoinStyle(pen.joinStyle());
        d->stroker.setCapStyle(pen.capStyle());
        d->stroker.setMiterLimit(pen.miterLimit());
        qreal penWidth = pen.widthF();
        if (penWidth == 0)
            d->stroker.setStrokeWidth(1);
        else
            d->stroker.setStrokeWidth(penWidth);

        Qt::PenStyle style = pen.style();
        if (style == Qt::SolidLine) {
            d->activeStroker = &d->stroker;
        } else if (style == Qt::NoPen) {
            d->activeStroker = 0;
        } else {
            // ### re-enable...
//             if (pen.isCosmetic()) {
//                 d->dashStroker->setClipRect(d->deviceRect);
//             } else {
//                 QRectF clipRect = s->matrix.inverted().mapRect(QRectF(d->deviceRect));
//                 d->dashStroker->setClipRect(clipRect);
//             }
            d->dasher.setDashPattern(pen.dashPattern());
            d->dasher.setDashOffset(pen.dashOffset());
            d->activeStroker = &d->dasher;
        }
    }

    if (!d->activeStroker) {
        return;
    }

    const QPainterPath::ElementType *types = path.elements();
    const qreal *points = path.points();
    int pointCount = path.elementCount();

    const qreal *lastPoint = points + (pointCount<<1);

    d->activeStroker->begin(d->strokeHandler);
    d->strokeHandler->types.reset();
    d->strokeHandler->pts.reset();

    // Some engines might decide to optimize for the non-shape hint later on...
    uint flags = QVectorPath::WindingFill;
    if (d->stroker.capStyle() == Qt::RoundCap || d->stroker.joinStyle() == Qt::RoundJoin)
        flags |= QVectorPath::CurvedShapeHint;

    // ### Perspective Xforms are currently not supported...
    qreal txscale = 1;
    if (!(pen.isCosmetic() || (qt_scaleForTransform(state()->matrix, &txscale) && txscale != 1))) {
        // We include cosmetic pens in this case to avoid having to
        // change the current transform. Normal transformed,
        // non-cosmetic pens will be transformed as part of fill
        // later, so they are also covered here..
        if (types) {
            while (points < lastPoint) {
                switch (*types) {
                case QPainterPath::MoveToElement:
                    d->activeStroker->moveTo(points[0], points[1]);
                    points += 2;
                    ++types;
                    break;
                case QPainterPath::LineToElement:
                    d->activeStroker->lineTo(points[0], points[1]);
                    points += 2;
                    ++types;
                    break;
                case QPainterPath::CurveToElement:
                    d->activeStroker->cubicTo(points[0], points[1],
                                              points[2], points[3],
                                              points[4], points[5]);
                    points += 6;
                    types += 3;
                    flags |= QVectorPath::CurvedShapeHint;
                    break;
                default:
                    break;
                }
            }
            if (path.hasImplicitClose())
                d->activeStroker->lineTo(path.points()[0], path.points()[1]);

        } else {
            d->activeStroker->moveTo(points[0], points[1]);
            points += 2;
            ++types;
            while (points < lastPoint) {
                d->activeStroker->lineTo(points[0], points[1]);
                points += 2;
                ++types;
            }
            if (path.hasImplicitClose())
                d->activeStroker->lineTo(path.points()[0], path.points()[1]);
        }
        d->activeStroker->end();

        if (!d->strokeHandler->types.size()) // an empty path...
            return;

        QVectorPath strokePath(d->strokeHandler->pts.data(),
                               d->strokeHandler->types.size(),
                               d->strokeHandler->types.data(),
                               QVectorPath::WindingFill);
        fill(strokePath, pen.brush());
    } else {
        const qreal strokeWidth = d->stroker.strokeWidth();
        d->stroker.setStrokeWidth(strokeWidth * txscale);
        // For cosmetic pens we need a bit of trickery... We to process xform the input points
        if (types) {
            while (points < lastPoint) {
                switch (*types) {
                case QPainterPath::MoveToElement: {
                    QPointF pt = (*(QPointF *) points) * state()->matrix;
                    d->activeStroker->moveTo(pt.x(), pt.y());
                    points += 2;
                    ++types;
                    break;
                }
                case QPainterPath::LineToElement: {
                    QPointF pt = (*(QPointF *) points) * state()->matrix;
                    d->activeStroker->lineTo(pt.x(), pt.y());
                    points += 2;
                    ++types;
                    break;
                }
                case QPainterPath::CurveToElement: {
                    QPointF c1 = ((QPointF *) points)[0] * state()->matrix;
                    QPointF c2 = ((QPointF *) points)[1] * state()->matrix;
                    QPointF e =  ((QPointF *) points)[2] * state()->matrix;
                    d->activeStroker->cubicTo(c1.x(), c1.y(), c2.x(), c2.y(), e.x(), e.y());
                    points += 6;
                    types += 3;
                    flags |= QVectorPath::CurvedShapeHint;
                    break;
                }
                default:
                    break;
                }
            }
            if (path.hasImplicitClose()) {
                QPointF pt = * ((QPointF *) path.points()) * state()->matrix;
                d->activeStroker->lineTo(pt.x(), pt.y());
            }

        } else {
            QPointF p = ((QPointF *)points)[0] * state()->matrix;
            d->activeStroker->moveTo(p.x(), p.y());
            points += 2;
            ++types;
            while (points < lastPoint) {
                QPointF p = ((QPointF *)points)[0] * state()->matrix;
                d->activeStroker->lineTo(p.x(), p.y());
                points += 2;
                ++types;
            }
            if (path.hasImplicitClose())
                d->activeStroker->lineTo(p.x(), p.y());
        }

        d->activeStroker->end();
        d->stroker.setStrokeWidth(strokeWidth);
        QVectorPath strokePath(d->strokeHandler->pts.data(),
                               d->strokeHandler->types.size(),
                               d->strokeHandler->types.data(),
                               QVectorPath::WindingFill);

        QTransform xform = state()->matrix;
        state()->matrix = QTransform();
        transformChanged();

        QBrush brush = pen.brush();
        if (qbrush_style(brush) != Qt::SolidPattern)
            brush.setTransform(brush.transform() * xform);

        fill(strokePath, brush);

        state()->matrix = xform;
        transformChanged();
    }
}

void QPaintEngineEx::draw(const QVectorPath &path)
{
    fill(path, state()->brush);
    stroke(path, state()->pen);
}


void QPaintEngineEx::clip(const QRect &r, Qt::ClipOperation op)
{
    qreal right = r.x() + r.width();
    qreal bottom = r.y() + r.height();
    qreal pts[] = { r.x(), r.y(),
                    right, r.y(),
                    right, bottom,
                    r.x(), bottom,
                    r.x(), r.y() };
    QVectorPath vp(pts, 5, 0, QVectorPath::RectangleHint);
    clip(vp, op);
}

void QPaintEngineEx::clip(const QRegion &region, Qt::ClipOperation op)
{
    QVector<QRect> rects = region.rects();
    if (rects.size() <= 32) {
        qreal pts[2*32*4];
        int pos = 0;
        for (QVector<QRect>::const_iterator i = rects.constBegin(); i != rects.constEnd(); ++i) {
            qreal x1 = i->x();
            qreal y1 = i->y();
            qreal x2 = i->x() + i->width();
            qreal y2 = i->y() + i->height();

            pts[pos++] = x1;
            pts[pos++] = y1;

            pts[pos++] = x2;
            pts[pos++] = y1;

            pts[pos++] = x2;
            pts[pos++] = y2;

            pts[pos++] = x1;
            pts[pos++] = y2;
        }
        QVectorPath vp(pts, rects.size() * 4, qpaintengineex_rect4_types_32);
        clip(vp, op);
    } else {
        QVarLengthArray<qreal> pts(rects.size() * 2 * 4);
        QVarLengthArray<QPainterPath::ElementType> types(rects.size() * 4);
        int ppos = 0;
        int tpos = 0;

        for (QVector<QRect>::const_iterator i = rects.constBegin(); i != rects.constEnd(); ++i) {
            qreal x1 = i->x();
            qreal y1 = i->y();
            qreal x2 = i->x() + i->width();
            qreal y2 = i->y() + i->height();

            pts[ppos++] = x1;
            pts[ppos++] = y1;

            pts[ppos++] = x2;
            pts[ppos++] = y1;

            pts[ppos++] = x2;
            pts[ppos++] = y2;

            pts[ppos++] = x1;
            pts[ppos++] = y2;

            types[tpos++] = QPainterPath::MoveToElement;
            types[tpos++] = QPainterPath::LineToElement;
            types[tpos++] = QPainterPath::LineToElement;
            types[tpos++] = QPainterPath::LineToElement;
        }

        QVectorPath vp(pts.data(), rects.size() * 4, types.data());
        clip(vp, op);
    }

}

void QPaintEngineEx::clip(const QPainterPath &path, Qt::ClipOperation op)
{
    if (path.isEmpty()) {
        QVectorPath vp(0, 0);
        clip(vp, op);
    } else {
        clip(qtVectorPathForPath(path), op);
    }
}

void QPaintEngineEx::fillRect(const QRectF &r, const QBrush &brush)
{
    qreal pts[] = { r.x(), r.y(), r.x() + r.width(), r.y(),
                    r.x() + r.width(), r.y() + r.height(), r.x(), r.y() + r.height() };
    QVectorPath vp(pts, 4, 0, QVectorPath::RectangleHint);
    fill(vp, brush);
}

void QPaintEngineEx::fillRect(const QRectF &r, const QColor &color)
{
    fillRect(r, QBrush(color));
}

void QPaintEngineEx::drawRects(const QRect *rects, int rectCount)
{
    for (int i=0; i<rectCount; ++i) {
        const QRect &r = rects[i];
        // ### Is there a one off here?
        qreal right = r.x() + r.width();
        qreal bottom = r.y() + r.height();
        qreal pts[] = { r.x(), r.y(),
                        right, r.y(),
                        right, bottom,
                        r.x(), bottom,
                        r.x(), r.y() };
        QVectorPath vp(pts, 5, 0, QVectorPath::RectangleHint);
        draw(vp);
    }
}

void QPaintEngineEx::drawRects(const QRectF *rects, int rectCount)
{
    for (int i=0; i<rectCount; ++i) {
        const QRectF &r = rects[i];
        qreal right = r.x() + r.width();
        qreal bottom = r.y() + r.height();
        qreal pts[] = { r.x(), r.y(),
                        right, r.y(),
                        right, bottom,
                        r.x(), bottom,
                        r.x(), r.y() };
        QVectorPath vp(pts, 5, 0, QVectorPath::RectangleHint);
        draw(vp);
    }
}

void QPaintEngineEx::drawLines(const QLine *lines, int lineCount)
{
    int elementCount = lineCount << 1;
    while (elementCount > 0) {
        int count = qMin(elementCount, 32);

        qreal pts[64];
        int count2 = count<<1;
#ifdef Q_WS_MAC
        for (int i=0; i<count2; i+=2) {
            pts[i] = ((int *) lines)[i+1];
            pts[i+1] = ((int *) lines)[i];
        }
#else
        for (int i=0; i<count2; ++i)
            pts[i] = ((int *) lines)[i];
#endif

        QVectorPath path(pts, count, qpaintengineex_line_types_16, QVectorPath::LinesHint);
        stroke(path, state()->pen);

        elementCount -= 32;
        lines += 16;
    }
}

void QPaintEngineEx::drawLines(const QLineF *lines, int lineCount)
{
    int elementCount = lineCount << 1;
    while (elementCount > 0) {
        int count = qMin(elementCount, 32);

        QVectorPath path((qreal *) lines, count, qpaintengineex_line_types_16,
                         QVectorPath::LinesHint);
        stroke(path, state()->pen);

        elementCount -= 32;
        lines += 16;
    }
}

void QPaintEngineEx::drawEllipse(const QRectF &r)
{
    qreal pts[26]; // QPointF[13] without constructors...
    union {
        qreal *ptr;
        QPointF *points;
    } x;
    x.ptr = pts;

    int point_count = 0;
    x.points[0] = qt_curves_for_arc(r, 0, -360, x.points + 1, &point_count);
    QVectorPath vp((qreal *) pts, 13, qpaintengineex_ellipse_types, QVectorPath::EllipseHint);
    draw(vp);
}

void QPaintEngineEx::drawEllipse(const QRect &r)
{
    drawEllipse(QRectF(r));
}

void QPaintEngineEx::drawPath(const QPainterPath &path)
{
    if (!path.isEmpty())
        draw(qtVectorPathForPath(path));
}


void QPaintEngineEx::drawPoints(const QPointF *points, int pointCount)
{
    QPen pen = state()->pen;
    if (pen.capStyle() == Qt::FlatCap)
        pen.setCapStyle(Qt::SquareCap);

    if (pen.brush().isOpaque()) {
        while (pointCount > 0) {
            int count = qMin(pointCount, 16);
            qreal pts[64];
            int oset = -1;
            for (int i=0; i<count; ++i) {
                pts[++oset] = points[i].x();
                pts[++oset] = points[i].y();
                pts[++oset] = points[i].x() + 0.001;
                pts[++oset] = points[i].y();
            }
            QVectorPath path(pts, count * 2, qpaintengineex_line_types_16, QVectorPath::NonCurvedShapeHint);
            stroke(path, pen);
            pointCount -= 16;
            points += 16;
        }
    } else {
        for (int i=0; i<pointCount; ++i) {
            qreal pts[] = { points[i].x(), points[i].y(), points[i].x() + 0.001, points[i].y() };
            QVectorPath path(pts, 2, 0);
            stroke(path, pen);
        }
    }
}

void QPaintEngineEx::drawPoints(const QPoint *points, int pointCount)
{
    QPen pen = state()->pen;
    if (pen.capStyle() == Qt::FlatCap)
        pen.setCapStyle(Qt::SquareCap);

    if (pen.brush().isOpaque()) {
        while (pointCount > 0) {
            int count = qMin(pointCount, 16);
            qreal pts[64];
            int oset = -1;
            for (int i=0; i<count; ++i) {
                pts[++oset] = points[i].x();
                pts[++oset] = points[i].y();
                pts[++oset] = points[i].x() + 0.001;
                pts[++oset] = points[i].y();
            }
            QVectorPath path(pts, count * 2, qpaintengineex_line_types_16, QVectorPath::NonCurvedShapeHint);
            stroke(path, pen);
            pointCount -= 16;
            points += 16;
        }
    } else {
        for (int i=0; i<pointCount; ++i) {
            qreal pts[] = { points[i].x(), points[i].y(), points[i].x() + 0.001, points[i].y() };
            QVectorPath path(pts, 2, 0);
            stroke(path, pen);
        }
    }
}


void QPaintEngineEx::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    QVectorPath path((qreal *) points, pointCount, 0, QVectorPath::polygonFlags(mode));

    if (mode == PolylineMode)
        stroke(path, state()->pen);
    else
        draw(path);
}

void QPaintEngineEx::drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode)
{
    int count = pointCount<<1;
    QVarLengthArray<qreal> pts(count);

#ifdef Q_WS_MAC
    for (int i=0; i<count; i+=2) {
        pts[i] = ((int *) points)[i+1];
        pts[i+1] = ((int *) points)[i];
    }
#else
    for (int i=0; i<count; ++i)
        pts[i] = ((int *) points)[i];
#endif

    QVectorPath path(pts.data(), pointCount, 0, QVectorPath::polygonFlags(mode));

    if (mode == PolylineMode)
        stroke(path, state()->pen);
    else
        draw(path);

}

void QPaintEngineEx::drawPixmap(const QPointF &pos, const QPixmap &pm)
{
    drawPixmap(QRectF(pos, pm.size()), pm, pm.rect());
}

void QPaintEngineEx::drawImage(const QPointF &pos, const QImage &image)
{
    drawImage(QRectF(pos, image.size()), image, image.rect());
}

void QPaintEngineEx::drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s)
{
    QBrush brush(state()->pen.color(), pixmap);
    QTransform xform;
    xform.translate(-s.x(), -s.y());
    brush.setTransform(xform);

    qreal pts[] = { r.x(), r.y(),
                    r.x() + r.width(), r.y(),
                    r.x() + r.width(), r.y() + r.height(),
                    r.x(), r.y() + r.height() };

    QVectorPath path(pts, 4, 0, QVectorPath::RectangleHint);
    fill(path, brush);
}

void QPaintEngineEx::setState(QPainterState *s)
{
    QPaintEngine::state = s;
}


void QPaintEngineEx::updateState(const QPaintEngineState &)
{
    // do nothing...
}

QT_END_NAMESPACE
