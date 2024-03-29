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

// #include <QtCore/qglobal.h>
#include "core/qglobal.h"

#define QT_FT_BEGIN_HEADER
#define QT_FT_END_HEADER

#include <private/qrasterdefs_p.h>
#include <private/qgrayraster_p.h>

#include <qpainterpath.h>
// #include <qdebug.h>
// #include <qhash.h>
#include "core/qhash.h"
//modify #include <qlabel.h>
#include <qbitmap.h>
// #include <qmath.h>
#include "core/qmath.h"

#if defined (Q_WS_X11)
#  include <private/qfontengine_ft_p.h>
#endif

//   #include <private/qdatabuffer_p.h>
//   #include <private/qpainter_p.h>
#include <private/qmath_p.h>
// #include <private/qtextengine_p.h>
// #include <private/qfontengine_p.h>
#include <private/qpixmap_raster_p.h>
//   #include <private/qpolygonclipper_p.h>
//   #include <private/qrasterizer_p.h>
#include <private/qimage_p.h>

#include "qpaintengine_raster_p.h"
//   #include "qbezier_p.h"
#include "qoutlinemapper_p.h"

#if defined(Q_WS_WIN)
// #  include <qt_windows.h>
// #  include <qvarlengtharray.h>
#include "core/qt_windows.h"
#include "core/qvarlengtharray.h"
//#  include <private/qfontengine_p.h>
#  if defined(Q_OS_WINCE)
#    include "qguifunctions_wince.h"
#  endif
#elif defined(Q_WS_MAC)
#  include <private/qt_mac_p.h>
#  include <private/qpixmap_mac_p.h>
#  include <private/qpaintengine_mac_p.h>
#elif defined(Q_WS_QWS)
#  if !defined(QT_NO_FREETYPE)
#    include <private/qfontengine_ft_p.h>
#  endif
#  if !defined(QT_NO_QWS_QPF2)
#    include <private/qfontengine_qpf_p.h>
#  endif
#  include <private/qabstractfontengine_p.h>
#endif

#if defined(Q_WS_WIN64)
#  include <malloc.h>
#endif
#include <limits.h>

#if defined(QT_NO_FPU) || (_MSC_VER >= 1300 && _MSC_VER < 1400)
#  define FLOATING_POINT_BUGGY_OR_NO_FPU
#endif

QT_BEGIN_NAMESPACE

extern bool qt_scaleForTransform(const QTransform &transform, qreal *scale); // qtransform.cpp

#define qreal_to_fixed_26_6(f) (int(f * 64))
#define qt_swap_int(x, y) { int tmp = (x); (x) = (y); (y) = tmp; }
#define qt_swap_qreal(x, y) { qreal tmp = (x); (x) = (y); (y) = tmp; }

#ifdef Q_WS_WIN
static bool qt_enable_16bit_colors = false;
#endif

// #define QT_DEBUG_DRAW
#ifdef QT_DEBUG_DRAW
void dumpClip(int width, int height, QClipData *clip);
#endif

#define QT_FAST_SPANS


// A little helper macro to get a better approximation of dimensions.
// If we have a rect that starting at 0.5 of width 3.5 it should span
// 4 pixels.
#define int_dim(pos, dim) (int(pos+dim) - int(pos))

// use the same rounding as in qrasterizer.cpp (6 bit fixed point)
static const qreal aliasedCoordinateDelta = 0.5 - 0.015625;

#ifdef Q_WS_WIN
extern bool qt_cleartype_enabled;
#endif


/********************************************************************************
 * Span functions
 */
static void qt_span_fill_clipRect(int count, const QSpan *spans, void *userData);
static void qt_span_fill_clipRegion(int count, const QSpan *spans, void *userData);
static void qt_span_fill_clipped(int count, const QSpan *spans, void *userData);
static void qt_span_clip(int count, const QSpan *spans, void *userData);
static void qt_merge_clip(const QClipData *c1, const QClipData *c2, QClipData *result);

struct ClipData
{
    QClipData *oldClip;
    QClipData *newClip;
    Qt::ClipOperation operation;
};

enum LineDrawMode {
    LineDrawClipped,
    LineDrawNormal,
    LineDrawIncludeLastPixel
};

static void drawLine_midpoint_i(int x1, int y1, int x2, int y2, ProcessSpans span_func, QSpanData *data,
                                LineDrawMode style, const QIntRect &rect);
static void drawLine_midpoint_dashed_i(int x1, int y1, int x2, int y2,
                                       QPen *pen, ProcessSpans span_func, QSpanData *data,
                                       LineDrawMode style, const QIntRect &devRect,
                                       int *patternOffset);
// static void drawLine_midpoint_f(qreal x1, qreal y1, qreal x2, qreal y2,
//                                 ProcessSpans span_func, QSpanData *data,
//                                 LineDrawMode style, const QRect &devRect);

static void drawEllipse_midpoint_i(const QRect &rect, const QRect &clip,
                                   ProcessSpans pen_func, ProcessSpans brush_func,
                                   QSpanData *pen_data, QSpanData *brush_data);

struct QRasterFloatPoint {
    qreal x;
    qreal y;
};

#ifdef QT_DEBUG_DRAW
static const QRectF boundingRect(const QPointF *points, int pointCount)
{
    const QPointF *e = points;
    const QPointF *last = points + pointCount;
    qreal minx, maxx, miny, maxy;
    minx = maxx = e->x();
    miny = maxy = e->y();
    while (++e < last) {
        if (e->x() < minx)
            minx = e->x();
        else if (e->x() > maxx)
            maxx = e->x();
        if (e->y() < miny)
            miny = e->y();
        else if (e->y() > maxy)
            maxy = e->y();
    }
    return QRectF(QPointF(minx, miny), QPointF(maxx, maxy));
}
#endif

template <typename T> static inline bool isRect(const T *pts, int elementCount) {
    return (elementCount == 5 // 5-point polygon, check for closed rect
            && pts[0] == pts[8] && pts[1] == pts[9] // last point == first point
            && pts[0] == pts[6] && pts[2] == pts[4] // x values equal
            && pts[1] == pts[3] && pts[5] == pts[7] // y values equal...
            && pts[0] < pts[4] && pts[1] < pts[5]
            ) ||
           (elementCount == 4 // 4-point polygon, check for unclosed rect
            && pts[0] == pts[6] && pts[2] == pts[4] // x values equal
            && pts[1] == pts[3] && pts[5] == pts[7] // y values equal...
            && pts[0] < pts[4] && pts[1] < pts[5]
            );
}


static void qt_ft_outline_move_to(qfixed x, qfixed y, void *data)
{
    ((QOutlineMapper *) data)->moveTo(QPointF(qt_fixed_to_real(x), qt_fixed_to_real(y)));
}

static void qt_ft_outline_line_to(qfixed x, qfixed y, void *data)
{
    ((QOutlineMapper *) data)->lineTo(QPointF(qt_fixed_to_real(x), qt_fixed_to_real(y)));
}

static void qt_ft_outline_cubic_to(qfixed c1x, qfixed c1y,
                             qfixed c2x, qfixed c2y,
                             qfixed ex, qfixed ey,
                             void *data)
{
    ((QOutlineMapper *) data)->curveTo(QPointF(qt_fixed_to_real(c1x), qt_fixed_to_real(c1y)),
                                       QPointF(qt_fixed_to_real(c2x), qt_fixed_to_real(c2y)),
                                       QPointF(qt_fixed_to_real(ex), qt_fixed_to_real(ey)));
}


#if !defined(QT_NO_DEBUG) && 0
static void qt_debug_path(const QPainterPath &path)
{
    const char *names[] = {
        "MoveTo     ",
        "LineTo     ",
        "CurveTo    ",
        "CurveToData"
    };

    fprintf(stderr,"\nQPainterPath: elementCount=%d\n", path.elementCount());
    for (int i=0; i<path.elementCount(); ++i) {
        const QPainterPath::Element &e = path.elementAt(i);
        Q_ASSERT(e.type >= 0 && e.type <= QPainterPath::CurveToDataElement);
        fprintf(stderr," - %3d:: %s, (%.2f, %.2f)\n", i, names[e.type], e.x, e.y);
    }
}
#endif



/*!
    \class QRasterPaintEngine
    \preliminary
    \ingroup qws
    \since 4.2

    \brief The QRasterPaintEngine class enables hardware acceleration
    of painting operations in Qt for Embedded Linux.

    Note that this functionality is only available in
    \l{Qt for Embedded Linux}.

    In \l{Qt for Embedded Linux}, painting is a pure software
    implementation. But starting with Qt 4.2, it is
    possible to add an accelerated graphics driver to take advantage
    of available hardware resources.

    Hardware acceleration is accomplished by creating a custom screen
    driver, accelerating the copying from memory to the screen, and
    implementing a custom paint engine accelerating the various
    painting operations. Then a custom paint device (derived from the
    QCustomRasterPaintDevice class) and a custom window surface
    (derived from QWSWindowSurface) must be implemented to make
    \l{Qt for Embedded Linux} aware of the accelerated driver.

    \note The QRasterPaintEngine class does not support 8-bit images.
    Instead, they need to be converted to a supported format, such as
    QImage::Format_ARGB32_Premultiplied.

    See the \l {Adding an Accelerated Graphics Driver to Qt for Embedded Linux}
    documentation for details.

    \sa QCustomRasterPaintDevice, QPaintEngine
*/

/*!
    \fn Type QRasterPaintEngine::type() const
    \reimp
*/

/*!
    \typedef QSpan
    \relates QRasterPaintEngine

    A struct equivalent to QT_FT_Span, containing a position (x,
    y), the span's length in pixels and its color/coverage (a value
    ranging from 0 to 255).
*/

/*!
    \since 4.5

    Creates a raster based paint engine for operating on the given
    \a device, with the complete set of \l
    {QPaintEngine::PaintEngineFeature}{paint engine features and
    capabilities}.
*/
QRasterPaintEngine::QRasterPaintEngine(QPaintDevice *device)
    : QPaintEngineEx(*(new QRasterPaintEnginePrivate))
{
    d_func()->device = device;
    init();
}

/*!
    \internal
*/
QRasterPaintEngine::QRasterPaintEngine(QRasterPaintEnginePrivate &dd, QPaintDevice *device)
    : QPaintEngineEx(dd)
{
    d_func()->device = device;
    init();
}

void QRasterPaintEngine::init()
{
    Q_D(QRasterPaintEngine);


#ifdef Q_WS_WIN
    d->hdc = 0;
#endif

    d->rasterPoolSize = 8192;
    d->rasterPoolBase =
#if defined(Q_WS_WIN64)
        // We make use of setjmp and longjmp in qgrayraster.c which requires
        // 16-byte alignment, hence we hardcode this requirement here..
        (unsigned char *) _aligned_malloc(d->rasterPoolSize, sizeof(void*) * 2);
#else
        (unsigned char *) malloc(d->rasterPoolSize);
#endif

    // The antialiasing raster.
    d->grayRaster = new QT_FT_Raster;
    qt_ft_grays_raster.raster_new(0, d->grayRaster);
    qt_ft_grays_raster.raster_reset(*d->grayRaster, d->rasterPoolBase, d->rasterPoolSize);

    d->rasterizer = new QRasterizer;
    d->rasterBuffer = new QRasterBuffer();
    d->outlineMapper = new QOutlineMapper;
    d->outlinemapper_xform_dirty = true;

    d->basicStroker.setMoveToHook(qt_ft_outline_move_to);
    d->basicStroker.setLineToHook(qt_ft_outline_line_to);
    d->basicStroker.setCubicToHook(qt_ft_outline_cubic_to);
    d->dashStroker = 0;

    d->baseClip = 0;

    d->image_filler.init(d->rasterBuffer, this);
    d->image_filler.type = QSpanData::Texture;

    d->image_filler_xform.init(d->rasterBuffer, this);
    d->image_filler_xform.type = QSpanData::Texture;

    d->solid_color_filler.init(d->rasterBuffer, this);
    d->solid_color_filler.type = QSpanData::Solid;

    d->deviceDepth = d->device->depth();

    d->mono_surface = false;
    gccaps &= ~PorterDuff;

    QImage::Format format = QImage::Format_Invalid;

    switch (d->device->devType()) {
    case QInternal::Pixmap:
        qWarning("QRasterPaintEngine: unsupported for pixmaps...");
        break;
    case QInternal::Image:
        format = d->rasterBuffer->prepare(static_cast<QImage *>(d->device));
        break;
#ifdef Q_WS_QWS
    case QInternal::CustomRaster:
        d->rasterBuffer->prepare(static_cast<QCustomRasterPaintDevice*>(d->device));
        break;
#endif
    default:
        qWarning("QRasterPaintEngine: unsupported target device %d\n", d->device->devType());
        d->device = 0;
        return;
    }

    switch (format) {
    case QImage::Format_MonoLSB:
    case QImage::Format_Mono:
        d->mono_surface = true;
        break;
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB32:
        gccaps |= PorterDuff;
        break;
    case QImage::Format_RGB32:
    case QImage::Format_RGB444:
    case QImage::Format_RGB555:
    case QImage::Format_RGB666:
    case QImage::Format_RGB888:
    case QImage::Format_RGB16:
        break;
    default:
        break;
    }
}




/*!
    Destroys this paint engine.
*/
QRasterPaintEngine::~QRasterPaintEngine()
{
    Q_D(QRasterPaintEngine);

#if defined(Q_WS_WIN64)
    _aligned_free(d->rasterPoolBase);
#else
    free(d->rasterPoolBase);
#endif

    qt_ft_grays_raster.raster_done(*d->grayRaster);
    delete d->grayRaster;

    delete d->rasterBuffer;
    delete d->outlineMapper;
    delete d->rasterizer;
    delete d->dashStroker;
}

/*!
    \reimp
*/
bool QRasterPaintEngine::begin(QPaintDevice *device)
{
    Q_D(QRasterPaintEngine);

    if (device->devType() == QInternal::Pixmap) {
        QPixmap *pixmap = static_cast<QPixmap *>(device);
        if (pixmap->data->classId() == QPixmapData::RasterClass)
            d->device = pixmap->data->buffer();
    } else {
        d->device = device;
    }

    // Make sure QPaintEngine::paintDevice() returns the proper device.
    d->pdev = d->device;

    Q_ASSERT(d->device->devType() == QInternal::Image
             || d->device->devType() == QInternal::CustomRaster);

    d->systemStateChanged();

    QRasterPaintEngineState *s = state();
    ensureOutlineMapper();
    d->outlineMapper->m_clip_rect = d->deviceRect.adjusted(-10, -10, 10, 10);
    QRect bounds(-QT_RASTER_COORD_LIMIT, -QT_RASTER_COORD_LIMIT,
                 2*QT_RASTER_COORD_LIMIT, 2*QT_RASTER_COORD_LIMIT);
    d->outlineMapper->m_clip_rect = bounds.intersected(d->outlineMapper->m_clip_rect);


    d->rasterizer->setClipRect(d->deviceRect);

    s->penData.init(d->rasterBuffer, this);
    s->penData.setup(s->pen.brush(), s->intOpacity);
    s->stroker = &d->basicStroker;
    d->basicStroker.setClipRect(d->deviceRect);

    s->brushData.init(d->rasterBuffer, this);
    s->brushData.setup(s->brush, s->intOpacity);

    d->rasterBuffer->compositionMode = QPainter::CompositionMode_SourceOver;

    setDirty(DirtyBrushOrigin);

#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::begin(" << (void *) device
             << ") devType:" << device->devType()
             << "devRect:" << d->deviceRect;
    if (d->baseClip) {
        dumpClip(d->rasterBuffer->width(), d->rasterBuffer->height(), d->baseClip);
    }
#endif

#if defined(Q_WS_WIN)
    d->isPlain45DegreeRotation = true;
#endif

//     if (d->mono_surface)
//         d->glyphCacheType = QFontEngineGlyphCache::Raster_Mono;
// #ifdef Q_WS_WIN
//     else if (qt_cleartype_enabled) {
//         QImage::Format format = static_cast<QImage *>(d->device)->format();
//         if (format == QImage::Format_ARGB32_Premultiplied || format == QImage::Format_RGB32)
//             d->glyphCacheType = QFontEngineGlyphCache::Raster_RGBMask;
//         else
//             d->glyphCacheType = QFontEngineGlyphCache::Raster_A8;
//     }
// #endif
//     else
//         d->glyphCacheType = QFontEngineGlyphCache::Raster_A8;

    setActive(true);
    return true;
}

/*!
    \reimp
*/
bool QRasterPaintEngine::end()
{
    Q_D(QRasterPaintEngine);
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::end devRect:" << d->deviceRect;
    if (d->baseClip) {
        dumpClip(d->rasterBuffer->width(), d->rasterBuffer->height(), d->baseClip);
    }
#endif

    if (d->baseClip) {
        delete d->baseClip;
        d->baseClip = 0;
    }

    return true;
}

/*!
    \internal
*/
void QRasterPaintEngine::releaseBuffer()
{
    Q_D(QRasterPaintEngine);
    delete d->rasterBuffer;
    d->rasterBuffer = new QRasterBuffer;
}

/*!
    \internal
*/
QSize QRasterPaintEngine::size() const
{
    Q_D(const QRasterPaintEngine);
    return QSize(d->rasterBuffer->width(), d->rasterBuffer->height());
}

/*!
    \internal
*/
// #ifndef QT_NO_DEBUG
// void QRasterPaintEngine::saveBuffer(const QString &s) const
// {
//     Q_D(const QRasterPaintEngine);
//     d->rasterBuffer->bufferImage().save(s, "PNG");
// }
// #endif

/*!
    \internal
*/
void QRasterPaintEngine::updateMatrix(const QTransform &matrix)
{
    QRasterPaintEngineState *s = state();
    // FALCON: get rid of this line, see drawImage call below.
    s->matrix = matrix;
    QTransform::TransformationType txop = s->matrix.type();

    switch (txop) {

    case QTransform::TxNone:
        s->flags.int_xform = true;
        break;

    case QTransform::TxTranslate:
        s->flags.int_xform = qreal(int(s->matrix.dx())) == s->matrix.dx()
                            && qreal(int(s->matrix.dy())) == s->matrix.dy();
        break;

    case QTransform::TxScale:
        s->flags.int_xform = qreal(int(s->matrix.dx())) == s->matrix.dx()
                            && qreal(int(s->matrix.dy())) == s->matrix.dy()
                            && qreal(int(s->matrix.m11())) == s->matrix.m11()
                            && qreal(int(s->matrix.m22())) == s->matrix.m22();
        break;

    default: // shear / perspective...
        s->flags.int_xform = false;
        break;
    }

    s->flags.tx_noshear = qt_scaleForTransform(s->matrix, &s->txscale);

    ensureOutlineMapper();

#ifdef Q_WS_WIN
    Q_D(QRasterPaintEngine);
    d->isPlain45DegreeRotation = false;
    if (txop >= QTransform::TxRotate) {
        d->isPlain45DegreeRotation =
            (qFuzzyCompare(matrix.m11() + 1, qreal(1))
             && qFuzzyCompare(matrix.m12(), qreal(1))
             && qFuzzyCompare(matrix.m21(), qreal(-1))
             && qFuzzyCompare(matrix.m22() + 1, qreal(1))
                )
            ||
            (qFuzzyCompare(matrix.m11(), qreal(-1))
             && qFuzzyCompare(matrix.m12() + 1, qreal(1))
             && qFuzzyCompare(matrix.m21() + 1, qreal(1))
             && qFuzzyCompare(matrix.m22(), qreal(-1))
                )
            ||
            (qFuzzyCompare(matrix.m11() + 1, qreal(1))
             && qFuzzyCompare(matrix.m12(), qreal(-1))
             && qFuzzyCompare(matrix.m21(), qreal(1))
             && qFuzzyCompare(matrix.m22() + 1, qreal(1))
                )
            ;
    }
#endif

}



QRasterPaintEngineState::~QRasterPaintEngineState()
{
    if (flags.has_clip_ownership)
        delete clip;
}


QRasterPaintEngineState::QRasterPaintEngineState()
{
    stroker = 0;

    fillFlags = 0;
    strokeFlags = 0;
    pixmapFlags = 0;

    intOpacity = 256;

    txscale = 1.;

    flags.fast_pen = true;
    flags.antialiased = false;
    flags.bilinear = false;
    flags.fast_text = true;
    flags.int_xform = true;
    flags.tx_noshear = true;
    flags.fast_images = true;

    clip = 0;
    flags.has_clip_ownership = false;

    dirty = 0;
}

QRasterPaintEngineState::QRasterPaintEngineState(QRasterPaintEngineState &s)
    : QPainterState(s)
{
    stroker = s.stroker;

    lastBrush = s.lastBrush;
    brushData = s.brushData;
    brushData.tempImage = 0;

    lastPen = s.lastPen;
    penData = s.penData;
    penData.tempImage = 0;

    fillFlags = s.fillFlags;
    strokeFlags = s.strokeFlags;
    pixmapFlags = s.pixmapFlags;

    intOpacity = s.intOpacity;

    txscale = s.txscale;

    flag_bits = s.flag_bits;

    clip = s.clip;
    flags.has_clip_ownership = false;

    dirty = s.dirty;
}

/*!
    \internal
*/
QPainterState *QRasterPaintEngine::createState(QPainterState *orig) const
{
    QRasterPaintEngineState *s;
    if (!orig)
        s = new QRasterPaintEngineState();
    else
        s = new QRasterPaintEngineState(*static_cast<QRasterPaintEngineState *>(orig));

    return s;
}

/*!
    \internal
*/
void QRasterPaintEngine::setState(QPainterState *s)
{
    Q_D(QRasterPaintEngine);
    QPaintEngineEx::setState(s);
    d->rasterBuffer->compositionMode = s->composition_mode;
}

/*!
    \fn QRasterPaintEngineState *QRasterPaintEngine::state()
    \internal
*/

/*!
    \fn const QRasterPaintEngineState *QRasterPaintEngine::state() const
    \internal
*/

/*!
    \internal
*/
void QRasterPaintEngine::penChanged()
{
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::penChanged():" << state()->pen;
#endif
    QRasterPaintEngineState *s = state();
    s->strokeFlags |= DirtyPen;
    s->dirty |= DirtyPen;
}

/*!
    \internal
*/
void QRasterPaintEngine::updatePen(const QPen &pen)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::updatePen():" << s->pen;
#endif

    Qt::PenStyle pen_style = qpen_style(pen);

    s->lastPen = pen;
    s->strokeFlags = 0;

    s->penData.clip = d->clip();
    s->penData.setup(pen_style == Qt::NoPen ? QBrush() : pen.brush(), s->intOpacity);

    if (s->strokeFlags & QRasterPaintEngine::DirtyTransform
        || pen.brush().transform().type() >= QTransform::TxNone) {
        d->updateMatrixData(&s->penData, pen.brush(), s->matrix);
    }

    // Slightly ugly handling of an uncommon case... We need to change
    // the pen because it is reused in draw_midpoint to decide dashed
    // or non-dashed.
    if (pen_style == Qt::CustomDashLine && pen.dashPattern().size() == 0) {
        pen_style = Qt::SolidLine;
        s->lastPen.setStyle(Qt::SolidLine);
    }

    d->basicStroker.setJoinStyle(qpen_joinStyle(pen));
    d->basicStroker.setCapStyle(qpen_capStyle(pen));
    d->basicStroker.setMiterLimit(pen.miterLimit());

    qreal penWidth = qpen_widthf(pen);
    if (penWidth == 0)
        d->basicStroker.setStrokeWidth(1);
    else
        d->basicStroker.setStrokeWidth(penWidth);

    if(pen_style == Qt::SolidLine) {
        s->stroker = &d->basicStroker;
    } else if (pen_style != Qt::NoPen) {
        if (!d->dashStroker)
            d->dashStroker = new QDashStroker(&d->basicStroker);
        if (pen.isCosmetic()) {
            d->dashStroker->setClipRect(d->deviceRect);
        } else {
            // ### I've seen this inverted devrect multiple places now...
            QRectF clipRect = s->matrix.inverted().mapRect(QRectF(d->deviceRect));
            d->dashStroker->setClipRect(clipRect);
        }
        d->dashStroker->setDashPattern(pen.dashPattern());
        d->dashStroker->setDashOffset(pen.dashOffset());
        s->stroker = d->dashStroker;
    } else {
        s->stroker = 0;
    }

    s->flags.fast_pen = pen_style > Qt::NoPen
                  && s->penData.blend
                  && !s->flags.antialiased
                  && (penWidth == 0 || (penWidth <= 1
                                        && (s->matrix.type() <= QTransform::TxTranslate
                                            || pen.isCosmetic())));

    ensureState(); // needed because of tx_noshear...
    s->flags.non_complex_pen = qpen_capStyle(s->lastPen) <= Qt::SquareCap && s->flags.tx_noshear;

    s->strokeFlags = 0;
}



/*!
    \internal
*/
void QRasterPaintEngine::brushOriginChanged()
{
    QRasterPaintEngineState *s = state();
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::brushOriginChanged()" << s->brushOrigin;
#endif

    s->fillFlags |= DirtyBrushOrigin;
}


/*!
    \internal
*/
void QRasterPaintEngine::brushChanged()
{
    QRasterPaintEngineState *s = state();
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::brushChanged():" << s->brush;
#endif
    s->fillFlags |= DirtyBrush;
}




/*!
    \internal
*/
void QRasterPaintEngine::updateBrush(const QBrush &brush)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::updateBrush()" << brush;
#endif
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();
    // must set clip prior to setup, as setup uses it...
    s->brushData.clip = d->clip();
    s->brushData.setup(brush, s->intOpacity);
    if (s->fillFlags & DirtyTransform
        || brush.transform().type() >= QTransform::TxNone)
        d_func()->updateMatrixData(&s->brushData, brush, d->brushMatrix());
    s->lastBrush = brush;
    s->fillFlags = 0;
}

void QRasterPaintEngine::updateOutlineMapper()
{
    Q_D(QRasterPaintEngine);
    d->outlineMapper->setMatrix(state()->matrix);
}

void QRasterPaintEngine::updateState()
{
    QRasterPaintEngineState *s = state();

    if (s->dirty & DirtyTransform)
        updateMatrix(s->matrix);

    if (s->dirty & (DirtyPen|DirtyCompositionMode)) {
        const QPainter::CompositionMode mode = s->composition_mode;
        s->flags.fast_text = (s->penData.type == QSpanData::Solid)
                       && (mode == QPainter::CompositionMode_Source
                           || (mode == QPainter::CompositionMode_SourceOver
                               && qAlpha(s->penData.solid.color) == 255));
    }

    s->dirty = 0;
}


/*!
    \internal
*/
void QRasterPaintEngine::opacityChanged()
{
    QRasterPaintEngineState *s = state();

#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::opacityChanged()" << s->opacity;
#endif

    s->fillFlags |= DirtyOpacity;
    s->strokeFlags |= DirtyOpacity;
    s->pixmapFlags |= DirtyOpacity;
    s->intOpacity = (int) (s->opacity * 256);
}

/*!
    \internal
*/
void QRasterPaintEngine::compositionModeChanged()
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::compositionModeChanged()" << s->composition_mode;
#endif

    s->fillFlags |= DirtyCompositionMode;
    s->dirty |= DirtyCompositionMode;

    s->strokeFlags |= DirtyCompositionMode;
    d->rasterBuffer->compositionMode = s->composition_mode;

    d->recalculateFastImages();
}

/*!
    \internal
*/
void QRasterPaintEngine::renderHintsChanged()
{
    QRasterPaintEngineState *s = state();

#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::renderHintsChanged()" << hex << s->renderHints;
#endif

    bool was_aa = s->flags.antialiased;
    bool was_bilinear = s->flags.bilinear;

    s->flags.antialiased = bool(s->renderHints & QPainter::Antialiasing);
    s->flags.bilinear = bool(s->renderHints & QPainter::SmoothPixmapTransform);

    if (was_aa != s->flags.antialiased)
        s->strokeFlags |= DirtyHints;

    if (was_bilinear != s->flags.bilinear) {
        s->strokeFlags |= DirtyPen;
        s->fillFlags |= DirtyBrush;
    }

    Q_D(QRasterPaintEngine);
    d->recalculateFastImages();
}

/*!
    \internal
*/
void QRasterPaintEngine::transformChanged()
{
    QRasterPaintEngineState *s = state();

#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::transformChanged()" << s->matrix;
#endif

    s->fillFlags |= DirtyTransform;
    s->strokeFlags |= DirtyTransform;

    s->dirty |= DirtyTransform;

    Q_D(QRasterPaintEngine);
    d->recalculateFastImages();
}

/*!
    \internal
*/
void QRasterPaintEngine::clipEnabledChanged()
{
    QRasterPaintEngineState *s = state();

#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::clipEnabledChanged()" << s->clipEnabled;
#endif

    if (s->clip) {
        s->clip->enabled = s->clipEnabled;
        s->fillFlags |= DirtyClipEnabled;
        s->strokeFlags |= DirtyClipEnabled;
        s->pixmapFlags |= DirtyClipEnabled;
    }
}

#ifdef Q_WS_QWS
void QRasterPaintEnginePrivate::prepare(QCustomRasterPaintDevice *device)
{
    rasterBuffer->prepare(device);
}
#endif

void QRasterPaintEnginePrivate::drawImage(const QPointF &pt,
                                          const QImage &img,
                                          SrcOverBlendFunc func,
                                          const QRect &clip,
                                          int alpha,
                                          const QRect &sr)
{
    if (!clip.isValid())
        return;
    Q_ASSERT(img.depth() >= 8);

    int srcBPL = img.bytesPerLine();
    const uchar *srcBits = img.bits();
    int srcSize = img.depth() >> 3; // This is the part that is incompatible with lower than 8-bit..
    int iw = img.width();
    int ih = img.height();

    if (!sr.isEmpty()) {
        iw = sr.width();
        ih = sr.height();
        // Adjust the image according to the source offset...
        srcBits += ((sr.y() * srcBPL) + sr.x() * srcSize);
    }

    // adapt the x parameters
    int x = qRound(pt.x());
    int cx1 = clip.x();
    int cx2 = clip.x() + clip.width();
    if (x < cx1) {
        int d = cx1 - x;
        srcBits += srcSize * d;
        iw -= d;
        x = cx1;
    }
    if (x + iw > cx2) {
        int d = x + iw - cx2;
        iw -= d;
    }
    if (iw < 0)
        return;

    // adapt the y paremeters...
    int cy1 = clip.y();
    int cy2 = clip.y() + clip.height();
    int y = qRound(pt.y());
    if (y < cy1) {
        int d = cy1 - y;
        srcBits += srcBPL * d;
        ih -= d;
        y = cy1;
    }
    if (y + ih > cy2) {
        int d = y + ih - cy2;
        ih -= d;
    }
    if (ih < 0)
        return;

    // call the blend function...
    int dstSize = rasterBuffer->bytesPerPixel();
    int dstBPL = rasterBuffer->bytesPerLine();
    func(rasterBuffer->buffer() + x * dstSize + y * dstBPL, dstBPL,
         srcBits, srcBPL,
         iw, ih,
         alpha);
}


void QRasterPaintEnginePrivate::systemStateChanged()
{
    QRect clipRect(0, 0,
            qMin(QT_RASTER_COORD_LIMIT, device->width()),
            qMin(QT_RASTER_COORD_LIMIT, device->height()));

    if (!systemClip.isEmpty()) {
        QRegion clippedDeviceRgn = systemClip & clipRect;
        deviceRect = clippedDeviceRgn.boundingRect();
        delete baseClip;
        baseClip = new QClipData(device->height());
        baseClip->setClipRegion(clippedDeviceRgn);
    } else {
        deviceRect = clipRect;
    }
#ifdef QT_DEBUG_DRAW
    qDebug() << "systemStateChanged" << this << "deviceRect" << deviceRect << clipRect << systemClip;
#endif
    Q_Q(QRasterPaintEngine);
    q->state()->strokeFlags |= QPaintEngine::DirtyClipRegion;
    q->state()->fillFlags |= QPaintEngine::DirtyClipRegion;
    q->state()->pixmapFlags |= QPaintEngine::DirtyClipRegion;
}

void QRasterPaintEnginePrivate::updateMatrixData(QSpanData *spanData, const QBrush &b, const QTransform &m)
{
    if (b.d->style == Qt::NoBrush || b.d->style == Qt::SolidPattern)
        return;

    Q_Q(QRasterPaintEngine);
    bool bilinear = q->state()->flags.bilinear;

    if (b.d->transform.type() > QTransform::TxNone) { // FALCON: optimise
        spanData->setupMatrix(b.transform() * m, bilinear);
    } else {
        if (m.type() <= QTransform::TxTranslate) {
            // specialize setupMatrix for translation matrices
            // to avoid needless matrix inversion
            spanData->m11 = 1;
            spanData->m12 = 0;
            spanData->m13 = 0;
            spanData->m21 = 0;
            spanData->m22 = 1;
            spanData->m23 = 0;
            spanData->m33 = 1;
            spanData->dx = -m.dx();
            spanData->dy = -m.dy();
            spanData->txop = m.type();
            spanData->bilinear = bilinear;
            spanData->fast_matrix = qAbs(m.dx()) < 1e4 && qAbs(m.dy()) < 1e4;
            spanData->adjustSpanMethods();
        } else {
            spanData->setupMatrix(m, bilinear);
        }
    }
}


/*!
    \internal
*/
void QRasterPaintEngine::clip(const QVectorPath &path, Qt::ClipOperation op)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::clip(): " << path << op;

    if (path.elements()) {
        for (int i=0; i<path.elementCount(); ++i) {
            qDebug() << " - " << path.elements()[i]
                     << "(" << path.points()[i*2] << ", " << path.points()[i*2+1] << ")";
        }
    } else {
        for (int i=0; i<path.elementCount(); ++i) {
            qDebug() << " ---- "
                     << "(" << path.points()[i*2] << ", " << path.points()[i*2+1] << ")";
        }
    }
#endif

    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    const qreal *points = path.points();
    const QPainterPath::ElementType *types = path.elements();

    // There are some cases that are not supported by clip(QRect)
    if (op != Qt::UniteClip && (op != Qt::IntersectClip || !s->clip
                                || s->clip->hasRectClip || s->clip->hasRegionClip)) {
        if (s->matrix.type() <= QTransform::TxTranslate
            && ((path.shape() == QVectorPath::RectangleHint)
                || (isRect(points, path.elementCount())
                    && (!types || (types[0] == QPainterPath::MoveToElement
                                   && types[1] == QPainterPath::LineToElement
                                   && types[2] == QPainterPath::LineToElement
                                   && types[3] == QPainterPath::LineToElement))))) {
#ifdef QT_DEBUG_DRAW
            qDebug() << " --- optimizing vector clip to rect clip...";
#endif

            QRectF r(points[0], points[1], points[4]-points[0], points[5]-points[1]);
            clip(r.toRect(), op);
            return;
        }
    }

    if (op == Qt::NoClip) {
        if (s->flags.has_clip_ownership)
            delete s->clip;
        s->clip = 0;
        s->flags.has_clip_ownership = false;

    } else {
        QClipData *base = d->baseClip;

        // Intersect with current clip when available...
        if (op == Qt::IntersectClip && s->clip)
            base = s->clip;

        // We always intersect, except when there is nothing to
        // intersect with, in which case we simplify the operation to
        // a replace...
        Qt::ClipOperation isectOp = Qt::IntersectClip;
        if (base == 0)
            isectOp = Qt::ReplaceClip;

        QClipData *newClip = new QClipData(d->rasterBuffer->height());
        newClip->initialize();
        ClipData clipData = { base, newClip, isectOp };
        ensureOutlineMapper();
        d->rasterize(d->outlineMapper->convertPath(path), qt_span_clip, &clipData, 0);

        newClip->fixup();

        if (op == Qt::UniteClip) {
            // merge clips
            QClipData *result = new QClipData(d->rasterBuffer->height());
            QClipData *current = s->clip ? s->clip : new QClipData(d->rasterBuffer->height());
            qt_merge_clip(current, newClip, result);
            result->fixup();
            delete newClip;
            if (!s->clip)
                delete current;
            newClip = result;
        }

        if (s->flags.has_clip_ownership)
            delete s->clip;

        s->clip = newClip;
        s->flags.has_clip_ownership = true;
    }

    s->fillFlags |= DirtyClipPath;
    s->strokeFlags |= DirtyClipPath;
    s->pixmapFlags |= DirtyClipPath;

    d->solid_color_filler.clip = d->clip();
    d->solid_color_filler.adjustSpanMethods();
}



/*!
    \internal
*/
void QRasterPaintEngine::clip(const QRect &rect, Qt::ClipOperation op)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::clip(): " << rect << op;
#endif

    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    if (op == Qt::NoClip) {
        if (s->flags.has_clip_ownership)
            delete s->clip;
        s->clip = d->baseClip;
        s->flags.has_clip_ownership = false;

    } else if (op == Qt::UniteClip || s->matrix.type() > QTransform::TxScale) {
        QPaintEngineEx::clip(rect, op);
        return;

    } else if (op == Qt::ReplaceClip || s->clip == 0) {

        // No current clip, hence we intersect with sysclip and be
        // done with it...
        QRect clipRect = s->matrix.mapRect(rect) & d->deviceRect;
        QRegion clipRegion = systemClip();
        QClipData *clip = new QClipData(d->rasterBuffer->height());

        if (clipRegion.isEmpty())
            clip->setClipRect(clipRect);
        else
            clip->setClipRegion(clipRegion & clipRect);

        if (s->flags.has_clip_ownership)
            delete s->clip;

        s->clip = clip;
        s->flags.has_clip_ownership = true;

    } else { // intersect clip with current clip
        QClipData *base = s->clip;

        Q_ASSERT(base);
        if (base->hasRectClip || base->hasRegionClip) {
            QRect clipRect = s->matrix.mapRect(rect) & d->deviceRect;
            if (!s->flags.has_clip_ownership) {
                s->clip = new QClipData(d->rasterBuffer->height());
                s->flags.has_clip_ownership = true;
            }
            if (base->hasRectClip)
                s->clip->setClipRect(base->clipRect & clipRect);
            else
                s->clip->setClipRegion(base->clipRegion & clipRect);
        } else {
            QPaintEngineEx::clip(rect, op);
            return;
        }
    }

    s->brushData.clip = d->clip();
    s->penData.clip = d->clip();

    s->fillFlags |= DirtyClipPath;
    s->strokeFlags |= DirtyClipPath;
    s->pixmapFlags |= DirtyClipPath;

    d->solid_color_filler.clip = d->clip();
    d->solid_color_filler.adjustSpanMethods();
}

/*!
    \internal
*/
void QRasterPaintEngine::clip(const QRegion &region, Qt::ClipOperation op)
{
    QPaintEngineEx::clip(region, op);
}

/*!
    \internal
*/
void QRasterPaintEngine::clip(const QPainterPath &path, Qt::ClipOperation op)
{
    QPaintEngineEx::clip(path, op);
}

/*!
    \internal
*/
void QRasterPaintEngine::fillPath(const QPainterPath &path, QSpanData *fillData)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " --- fillPath, bounds=" << path.boundingRect();
#endif

    if (!fillData->blend)
        return;

    Q_D(QRasterPaintEngine);

    const QRectF controlPointRect = path.controlPointRect();

    QRasterPaintEngineState *s = state();
    const QRect deviceRect = s->matrix.mapRect(controlPointRect).toRect();
    ProcessSpans blend = d->getBrushFunc(deviceRect, fillData);
    const bool do_clip = (deviceRect.left() < -QT_RASTER_COORD_LIMIT
                          || deviceRect.right() > QT_RASTER_COORD_LIMIT
                          || deviceRect.top() < -QT_RASTER_COORD_LIMIT
                          || deviceRect.bottom() > QT_RASTER_COORD_LIMIT);

    if (!s->flags.antialiased && !do_clip) {
        d->initializeRasterizer(fillData);
        d->rasterizer->rasterize(path * s->matrix, path.fillRule());
        return;
    }

    ensureOutlineMapper();
    d->rasterize(d->outlineMapper->convertPath(path), blend, fillData, d->rasterBuffer);
}

static void fillRect_normalized(const QRect &r, QSpanData *data,
                                QRasterPaintEnginePrivate *pe)
{
    int x1, x2, y1, y2;

    bool rectClipped = false;

    if (data->clip) {
        x1 = qMax(r.x(), data->clip->xmin);
        x2 = qMin(r.x() + r.width(), data->clip->xmax);
        y1 = qMax(r.y(), data->clip->ymin);
        y2 = qMin(r.y() + r.height(), data->clip->ymax);
        rectClipped = data->clip->hasRectClip;

    } else if (pe) {
        x1 = qMax(r.x(), pe->deviceRect.x());
        x2 = qMin(r.x() + r.width(), pe->deviceRect.x() + pe->deviceRect.width());
        y1 = qMax(r.y(), pe->deviceRect.y());
        y2 = qMin(r.y() + r.height(), pe->deviceRect.y() + pe->deviceRect.height());
    } else {
        x1 = qMax(r.x(), 0);
        x2 = qMin(r.x() + r.width(), data->rasterBuffer->width());
        y1 = qMax(r.y(), 0);
        y2 = qMin(r.y() + r.height(), data->rasterBuffer->height());
    }

    if (x2 <= x1 || y2 <= y1)
        return;

    const int width = x2 - x1;
    const int height = y2 - y1;

    bool isUnclipped = rectClipped
                       || (pe && pe->isUnclipped_normalized(QRect(x1, y1, width, height)));

    if (pe && isUnclipped) {
        const QPainter::CompositionMode mode = pe->rasterBuffer->compositionMode;

        if (data->fillRect && (mode == QPainter::CompositionMode_Source
                               || (mode == QPainter::CompositionMode_SourceOver
                                   && qAlpha(data->solid.color) == 255)))
        {
            data->fillRect(data->rasterBuffer, x1, y1, width, height,
                           data->solid.color);
            return;
        }
    }

    ProcessSpans blend = isUnclipped ? data->unclipped_blend : data->blend;

    const int nspans = 256;
    QT_FT_Span spans[nspans];

    Q_ASSERT(data->blend);
    int y = y1;
    while (y < y2) {
        int n = qMin(nspans, y2 - y);
        int i = 0;
        while (i < n) {
            spans[i].x = x1;
            spans[i].len = width;
            spans[i].y = y + i;
            spans[i].coverage = 255;
            ++i;
        }

        blend(n, spans, data);
        y += n;
    }
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawRects(const QRect *rects, int rectCount)
{
#ifdef QT_DEBUG_DRAW
    qDebug(" - QRasterPaintEngine::drawRect(), rectCount=%d", rectCount);
#endif
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    // Fill
    ensureBrush();
    if (s->brushData.blend) {
        if (!s->flags.antialiased && s->matrix.type() <= QTransform::TxTranslate) {
            const QRect *r = rects;
            const QRect *lastRect = rects + rectCount;

            int offset_x = int(s->matrix.dx());
            int offset_y = int(s->matrix.dy());
            while (r < lastRect) {
                QRect rect = r->normalized();
                QRect rr = rect.translated(offset_x, offset_y);
                fillRect_normalized(rr, &s->brushData, d);
                ++r;
            }
        } else {
            QRectVectorPath path;
            for (int i=0; i<rectCount; ++i) {
                path.set(rects[i]);
                fill(path, s->brush);
            }
        }
    }

    ensurePen();
    if (s->penData.blend) {
        if (s->flags.fast_pen && s->lastPen.brush().isOpaque()) {
            const QRect *r = rects;
            const QRect *lastRect = rects + rectCount;
            while (r < lastRect) {
                int left = r->x();
                int right = r->x() + r->width();
                int top = r->y();
                int bottom = r->y() + r->height();

#ifdef Q_WS_MAC
                int pts[] = { top, left,
                              top, right,
                              bottom, right,
                              bottom, left };
#else
                int pts[] = { left, top,
                              right, top,
                              right, bottom,
                              left, bottom };
#endif

                strokePolygonCosmetic((QPoint *) pts, 4, WindingMode);
                ++r;
            }
        } else {
            QRectVectorPath path;
            for (int i = 0; i < rectCount; ++i) {
                path.set(rects[i]);
                stroke(path, s->pen);
            }
        }
    }
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
#ifdef QT_DEBUG_DRAW
    qDebug(" - QRasterPaintEngine::drawRect(), rectCount=%d", rectCount);
#endif
#ifdef QT_FAST_SPANS
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    ensureState();

    if (s->flags.tx_noshear) {
        ensureBrush();
        if (s->brushData.blend) {
            d->initializeRasterizer(&s->brushData);
            for (int i = 0; i < rectCount; ++i) {
                const QRectF &rect = rects[i].normalized();
                if (rect.isEmpty())
                    continue;
                const QPointF a = s->matrix.map((rect.topLeft() + rect.bottomLeft()) * 0.5f);
                const QPointF b = s->matrix.map((rect.topRight() + rect.bottomRight()) * 0.5f);
                d->rasterizer->rasterizeLine(a, b, rect.height() / rect.width());
            }
        }

        ensurePen();
        if (s->penData.blend) {
            qreal width = s->pen.isCosmetic()
                          ? (s->lastPen.widthF() == 0 ? 1 : s->lastPen.widthF())
                          : s->lastPen.widthF() * s->txscale;

            if (s->flags.fast_pen && s->lastPen.brush().isOpaque()) {
                for (int i = 0; i < rectCount; ++i) {
                    const QRectF &r = rects[i];
                    qreal left = r.x();
                    qreal right = r.x() + r.width();
                    qreal top = r.y();
                    qreal bottom = r.y() + r.height();
                    qreal pts[] = { left, top,
                                    right, top,
                                    right, bottom,
                                    left, bottom };
                    strokePolygonCosmetic((QPointF *) pts, 4, WindingMode);
                }
            } else if (width <= 1 && qpen_style(s->lastPen) == Qt::SolidLine) {
                d->initializeRasterizer(&s->penData);

                for (int i = 0; i < rectCount; ++i) {
                    const QRectF &rect = rects[i].normalized();
                    if (rect.isEmpty()) {
                        qreal pts[] = { rect.left(), rect.top(), rect.right(), rect.bottom() };
                        QVectorPath vp(pts, 2, 0, QVectorPath::LinesHint);
                        QPaintEngineEx::stroke(vp, s->lastPen);
                    } else {
                        const QPointF tl = s->matrix.map(rect.topLeft());
                        const QPointF tr = s->matrix.map(rect.topRight());
                        const QPointF bl = s->matrix.map(rect.bottomLeft());
                        const QPointF br = s->matrix.map(rect.bottomRight());
                        const qreal w = width / (rect.width() * s->txscale);
                        const qreal h = width / (rect.height() * s->txscale);
                        d->rasterizer->rasterizeLine(tl, tr, w); // top
                        d->rasterizer->rasterizeLine(bl, br, w); // bottom
                        d->rasterizer->rasterizeLine(bl, tl, h); // left
                        d->rasterizer->rasterizeLine(br, tr, h); // right
                    }
                }
            } else {
                for (int i = 0; i < rectCount; ++i) {
                    const QRectF &r = rects[i];
                    qreal left = r.x();
                    qreal right = r.x() + r.width();
                    qreal top = r.y();
                    qreal bottom = r.y() + r.height();
                    qreal pts[] = { left, top,
                                    right, top,
                                    right, bottom,
                                    left, bottom,
                                    left, top };
                    QVectorPath vp(pts, 5, 0, QVectorPath::RectangleHint);
                    QPaintEngineEx::stroke(vp, s->lastPen);
                }
            }
        }

        return;
    }
#endif // QT_FAST_SPANS
    QPaintEngineEx::drawRects(rects, rectCount);
}

void QRasterPaintEnginePrivate::strokeProjective(const QPainterPath &path)
{
    Q_Q(QRasterPaintEngine);
    QRasterPaintEngineState *s = q->state();

    const QPen &pen = s->lastPen;
    QPainterPathStroker pathStroker;
    pathStroker.setWidth(pen.width() == 0 ? qreal(1) : pen.width());
    pathStroker.setCapStyle(pen.capStyle());
    pathStroker.setJoinStyle(pen.joinStyle());
    pathStroker.setMiterLimit(pen.miterLimit());
    pathStroker.setDashOffset(pen.dashOffset());

    if (qpen_style(pen) == Qt::CustomDashLine)
        pathStroker.setDashPattern(pen.dashPattern());
    else
        pathStroker.setDashPattern(qpen_style(pen));

    outlineMapper->setMatrix(QTransform());
    const QPainterPath stroke = pen.isCosmetic()
        ? pathStroker.createStroke(s->matrix.map(path))
        : s->matrix.map(pathStroker.createStroke(path));

    rasterize(outlineMapper->convertPath(stroke), s->penData.blend, &s->penData, rasterBuffer);
    outlinemapper_xform_dirty = true;
}



/*!
    \internal
*/
void QRasterPaintEngine::stroke(const QVectorPath &path, const QPen &pen)
{
    QRasterPaintEngineState *s = state();
    ensurePen(pen);
    if (!s->penData.blend)
        return;

    if (s->flags.fast_pen && path.shape() <= QVectorPath::NonCurvedShapeHint && s->lastPen.brush().isOpaque()) {
        strokePolygonCosmetic((QPointF *) path.points(), path.elementCount(),
                              path.hasImplicitClose()
                              ? WindingMode
                              : PolylineMode);

    } else if (s->flags.non_complex_pen && path.shape() == QVectorPath::LinesHint) {
        qreal width = s->lastPen.isCosmetic()
                      ? (qpen_widthf(s->lastPen) == 0 ? 1 : qpen_widthf(s->lastPen))
                      : qpen_widthf(s->lastPen) * s->txscale;
        int dashIndex = 0;
        qreal dashOffset = s->lastPen.dashOffset();
        bool inDash = true;
        qreal patternLength = 0;
        const QVector<qreal> pattern = s->lastPen.dashPattern();
        for (int i = 0; i < pattern.size(); ++i)
            patternLength += pattern.at(i);

        if (patternLength > 0) {
            int n = qFloor(dashOffset / patternLength);
            dashOffset -= n * patternLength;
            while (dashOffset > pattern.at(dashIndex)) {
                dashOffset -= pattern.at(dashIndex);
                dashIndex = (dashIndex + 1) % pattern.size();
                inDash = !inDash;
            }
        }

        Q_D(QRasterPaintEngine);
        d->initializeRasterizer(&s->penData);
        int lineCount = path.elementCount() / 2;
        const QLineF *lines = reinterpret_cast<const QLineF *>(path.points());

        for (int i = 0; i < lineCount; ++i) {
            if (lines[i].p1() == lines[i].p2()) {
                if (s->lastPen.capStyle() != Qt::FlatCap) {
                    QPointF p = lines[i].p1();
                    QLineF line = s->matrix.map(QLineF(QPointF(p.x() - width*0.5, p.y()),
                                                       QPointF(p.x() + width*0.5, p.y())));
                    d->rasterizer->rasterizeLine(line.p1(), line.p2(), 1);
                }
                continue;
            }

            const QLineF line = s->matrix.map(lines[i]);
            if (qpen_style(s->lastPen) == Qt::SolidLine) {
                d->rasterizer->rasterizeLine(line.p1(), line.p2(),
                                            width / line.length(),
                                            s->lastPen.capStyle() == Qt::SquareCap);
            } else {
                d->rasterizeLine_dashed(line, width,
                                        &dashIndex, &dashOffset, &inDash);
            }
        }
    }
    else
        QPaintEngineEx::stroke(path, pen);
}

static inline QRect toNormalizedFillRect(const QRectF &rect)
{
    const int x1 = qRound(rect.x() + aliasedCoordinateDelta);
    const int y1 = qRound(rect.y() + aliasedCoordinateDelta);
    const int x2 = qRound(rect.right() + aliasedCoordinateDelta);
    const int y2 = qRound(rect.bottom() + aliasedCoordinateDelta);

    return QRect(x1, y1, x2 - x1, y2 - y1).normalized();
}

/*!
    \internal
*/
void QRasterPaintEngine::fill(const QVectorPath &path, const QBrush &brush)
{
    if (path.isEmpty())
        return;
#ifdef QT_DEBUG_DRAW
    QRealRect vectorPathBounds = path.controlPointRect();
    QRectF rf(vectorPathBounds.x1, vectorPathBounds.y1,
              vectorPathBounds.x2 - vectorPathBounds.x1, vectorPathBounds.y2 - vectorPathBounds.y1);
    qDebug() << "QRasterPaintEngine::fill(): "
             << "size=" << path.elementCount()
             << ", hints=" << hex << path.hints()
             << rf << brush;
#endif

    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    ensureBrush(brush);
    if (!s->brushData.blend)
        return;

    if (path.shape() == QVectorPath::RectangleHint) {
        if (!s->flags.antialiased && s->matrix.type() <= QTransform::TxScale) {
            const qreal *p = path.points();
            QPointF tl = QPointF(p[0], p[1]) * s->matrix;
            QPointF br = QPointF(p[4], p[5]) * s->matrix;
            fillRect_normalized(toNormalizedFillRect(QRectF(tl, br)), &s->brushData, d);
            return;
        }
        ensureState();
        if (s->flags.tx_noshear) {
            d->initializeRasterizer(&s->brushData);
            // ### Is normalizing really nessesary here?
            const qreal *p = path.points();
            QRectF r = QRectF(p[0], p[1], p[2] - p[0], p[7] - p[1]).normalized();
            if (!r.isEmpty()) {
                const QPointF a = s->matrix.map((r.topLeft() + r.bottomLeft()) * 0.5f);
                const QPointF b = s->matrix.map((r.topRight() + r.bottomRight()) * 0.5f);
                d->rasterizer->rasterizeLine(a, b, r.height() / r.width());
            }
            return;
        }
    }

    if (path.shape() == QVectorPath::EllipseHint) {
        if (!s->flags.antialiased && s->matrix.type() <= QTransform::TxScale) {
            const qreal *p = path.points();
            QPointF tl = QPointF(p[0], p[1]) * s->matrix;
            QPointF br = QPointF(p[4], p[5]) * s->matrix;
            QRectF r = s->matrix.mapRect(QRectF(tl, br));

            ProcessSpans penBlend = d->getPenFunc(r, &s->penData);
            ProcessSpans brushBlend = d->getBrushFunc(r, &s->brushData);
            const QRect brect = QRect(int(r.x()), int(r.y()),
                                      int_dim(r.x(), r.width()),
                                      int_dim(r.y(), r.height()));
            if (brect == r) {
                drawEllipse_midpoint_i(brect, d->deviceRect, penBlend, brushBlend,
                                       &s->penData, &s->brushData);
                return;
            }
        }
    }

    // ### Optimize for non transformed ellipses and rectangles...
    QRealRect r = path.controlPointRect();
    QRectF cpRect(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1);
    const QRect deviceRect = s->matrix.mapRect(cpRect).toRect();
    ProcessSpans blend = d->getBrushFunc(deviceRect, &s->brushData);

        // ### Falcon
//         const bool do_clip = (deviceRect.left() < -QT_RASTER_COORD_LIMIT
//                               || deviceRect.right() > QT_RASTER_COORD_LIMIT
//                               || deviceRect.top() < -QT_RASTER_COORD_LIMIT
//                               || deviceRect.bottom() > QT_RASTER_COORD_LIMIT);

        // ### Falonc: implement....
//         if (!s->flags.antialiased && !do_clip) {
//             d->initializeRasterizer(&s->brushData);
//             d->rasterizer->rasterize(path * d->matrix, path.fillRule());
//             return;
//         }

    ensureOutlineMapper();
    d->rasterize(d->outlineMapper->convertPath(path), blend, &s->brushData, d->rasterBuffer);
}

void QRasterPaintEngine::fillRect(const QRectF &r, QSpanData *data)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    if (!s->flags.antialiased) {
        uint txop = s->matrix.type();
        if (txop == QTransform::TxNone) {
            fillRect_normalized(toNormalizedFillRect(r), data, d);
            return;
        } else if (txop == QTransform::TxTranslate) {
            const QRect rr = toNormalizedFillRect(r.translated(s->matrix.dx(), s->matrix.dy()));
            fillRect_normalized(rr, data, d);
            return;
        } else if (txop == QTransform::TxScale) {
            const QRect rr = toNormalizedFillRect(s->matrix.mapRect(r));
            fillRect_normalized(rr, data, d);
            return;
        }
    }
    ensureState();
    if (s->flags.tx_noshear) {
        d->initializeRasterizer(data);
        QRectF nr = r.normalized();
        if (!nr.isEmpty()) {
            const QPointF a = s->matrix.map((nr.topLeft() + nr.bottomLeft()) * 0.5f);
            const QPointF b = s->matrix.map((nr.topRight() + nr.bottomRight()) * 0.5f);
            d->rasterizer->rasterizeLine(a, b, nr.height() / nr.width());
        }
        return;
    }

    QPainterPath path;
    path.addRect(r);
    ensureOutlineMapper();
    fillPath(path, data);
}

/*!
    \reimp
*/
void QRasterPaintEngine::fillRect(const QRectF &r, const QBrush &brush)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::fillRecct(): " << r << brush;
#endif
    QRasterPaintEngineState *s = state();

    ensureBrush(brush);
    if (!s->brushData.blend)
        return;

    fillRect(r, &s->brushData);
}

/*!
    \reimp
*/
void QRasterPaintEngine::fillRect(const QRectF &r, const QColor &color)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << "QRasterPaintEngine::fillRect(): " << r << color;
#endif
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    d->solid_color_filler.solid.color = PREMUL(ARGB_COMBINE_ALPHA(color.rgba(), s->intOpacity));
    d->solid_color_filler.clip = d->clip();
    d->solid_color_filler.adjustSpanMethods();

    fillRect(r, &d->solid_color_filler);
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawPath(const QPainterPath &path)
{
#ifdef QT_DEBUG_DRAW
    QRectF bounds = path.boundingRect();
    qDebug(" - QRasterPaintEngine::drawPath(), [%.2f, %.2f, %.2f, %.2f]",
           bounds.x(), bounds.y(), bounds.width(), bounds.height());
#endif

    if (path.isEmpty())
        return;

    // Filling..,
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    ensureBrush();
    if (s->brushData.blend) {
        ensureOutlineMapper();
        fillPath(path, &s->brushData);
    }

    // Stroking...
    ensurePen();
    if (!s->penData.blend)
        return;
    {
        if (s->matrix.type() >= QTransform::TxProject) {
            d->strokeProjective(path);
        } else {
            Q_ASSERT(s->stroker);
            d->outlineMapper->beginOutline(Qt::WindingFill);
            qreal txscale = 1;
            if (s->pen.isCosmetic() || (qt_scaleForTransform(s->matrix, &txscale) && txscale != 1)) {
                const qreal strokeWidth = d->basicStroker.strokeWidth();
                const QRectF clipRect = d->dashStroker ? d->dashStroker->clipRect() : QRectF();
                if (d->dashStroker)
                    d->dashStroker->setClipRect(d->deviceRect);
                d->basicStroker.setStrokeWidth(strokeWidth * txscale);
                d->outlineMapper->setMatrix(QTransform());
                s->stroker->strokePath(path, d->outlineMapper, s->matrix);
                d->outlinemapper_xform_dirty = true;
                d->basicStroker.setStrokeWidth(strokeWidth);
                if (d->dashStroker)
                    d->dashStroker->setClipRect(clipRect);
            } else {
                ensureOutlineMapper();
                s->stroker->strokePath(path, d->outlineMapper, QTransform());
            }
            d->outlineMapper->endOutline();

            ProcessSpans blend = d->getPenFunc(d->outlineMapper->controlPointRect,
                    &s->penData);
            d->rasterize(d->outlineMapper->outline(), blend, &s->penData, d->rasterBuffer);
        }
    }

}

static inline bool isAbove(const QPointF *a, const QPointF *b)
{
    return a->y() < b->y();
}

static bool splitPolygon(const QPointF *points, int pointCount, QVector<QPointF> *upper, QVector<QPointF> *lower)
{
    Q_ASSERT(upper);
    Q_ASSERT(lower);

    Q_ASSERT(pointCount >= 2);

    QVector<const QPointF *> sorted;
    sorted.reserve(pointCount);

    upper->reserve(pointCount * 3 / 4);
    lower->reserve(pointCount * 3 / 4);

    for (int i = 0; i < pointCount; ++i)
        sorted << points + i;

    qSort(sorted.begin(), sorted.end(), isAbove);

    qreal splitY = sorted.at(sorted.size() / 2)->y();

    const QPointF *end = points + pointCount;
    const QPointF *last = end - 1;

    QVector<QPointF> *bin[2] = { upper, lower };

    for (const QPointF *p = points; p < end; ++p) {
        int side = p->y() < splitY;
        int lastSide = last->y() < splitY;

        if (side != lastSide) {
            if (qFuzzyCompare(p->y(), splitY)) {
                bin[!side]->append(*p);
            } else if (qFuzzyCompare(last->y(), splitY)) {
                bin[side]->append(*last);
            } else {
                QPointF delta = *p - *last;
                QPointF intersection(p->x() + delta.x() * (splitY - p->y()) / delta.y(), splitY);

                bin[0]->append(intersection);
                bin[1]->append(intersection);
            }
        }

        bin[side]->append(*p);

        last = p;
    }

    // give up if we couldn't reduce the point count
    return upper->size() < pointCount && lower->size() < pointCount;
}

/*!
  \internal
 */
void QRasterPaintEngine::fillPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    const int maxPoints = 0xffff;

    // max amount of points that raster engine can reliably handle
    if (pointCount > maxPoints) {
        QVector<QPointF> upper, lower;

        if (splitPolygon(points, pointCount, &upper, &lower)) {
            fillPolygon(upper.constData(), upper.size(), mode);
            fillPolygon(lower.constData(), lower.size(), mode);
        } else
            qWarning("Polygon too complex for filling.");

        return;
    }

    // Compose polygon fill..,
    QVectorPath vp((qreal *) points, pointCount, 0, QVectorPath::polygonFlags(mode));
    ensureOutlineMapper();
    QT_FT_Outline *outline = d->outlineMapper->convertPath(vp);

    // scanconvert.
    ProcessSpans brushBlend = d->getBrushFunc(d->outlineMapper->controlPointRect,
                                              &s->brushData);
    d->rasterize(outline, brushBlend, &s->brushData, d->rasterBuffer);
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

#ifdef QT_DEBUG_DRAW
    qDebug(" - QRasterPaintEngine::drawPolygon(F), pointCount=%d", pointCount);
    for (int i=0; i<pointCount; ++i)
        qDebug() << "   - " << points[i];
#endif
    Q_ASSERT(pointCount >= 2);

    if (mode != PolylineMode && isRect((qreal *) points, pointCount)) {
        QRectF r(points[0], points[2]);
        drawRects(&r, 1);
        return;
    }

    ensurePen();
    ensureBrush();
    if (mode != PolylineMode) {
        // Do the fill...
        if (s->brushData.blend) {
            d->outlineMapper->setCoordinateRounding(s->penData.blend && s->flags.fast_pen && s->lastPen.brush().isOpaque());
            fillPolygon(points, pointCount, mode);
            d->outlineMapper->setCoordinateRounding(false);
        }
    }

    // Do the outline...
    if (s->penData.blend) {
        if (s->flags.fast_pen && s->lastPen.brush().isOpaque())
            strokePolygonCosmetic(points, pointCount, mode);
        else {
            QVectorPath vp((qreal *) points, pointCount, 0, QVectorPath::polygonFlags(mode));
            QPaintEngineEx::stroke(vp, s->lastPen);
        }
    }
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

#ifdef QT_DEBUG_DRAW
    qDebug(" - QRasterPaintEngine::drawPolygon(I), pointCount=%d", pointCount);
    for (int i=0; i<pointCount; ++i)
        qDebug() << "   - " << points[i];
#endif
    Q_ASSERT(pointCount >= 2);
    if (mode != PolylineMode && isRect((int *) points, pointCount)) {
        QRect r(points[0].x(),
                points[0].y(),
                points[2].x() - points[0].x(),
                points[2].y() - points[0].y());
        drawRects(&r, 1);
        return;
    }

    ensureState();
    ensurePen();
    if (!(s->flags.int_xform && s->flags.fast_pen && (!s->penData.blend || s->pen.brush().isOpaque()))) {
        // this calls the float version
        QPaintEngineEx::drawPolygon(points, pointCount, mode);
        return;
    }

    // Do the fill
    if (mode != PolylineMode) {
        ensureBrush();
        if (s->brushData.blend) {
            // Compose polygon fill..,
            ensureOutlineMapper();
            d->outlineMapper->setCoordinateRounding(s->penData.blend != 0);
            d->outlineMapper->beginOutline(mode == WindingMode ? Qt::WindingFill : Qt::OddEvenFill);
            d->outlineMapper->moveTo(*points);
            const QPoint *p = points;
            const QPoint *ep = points + pointCount - 1;
            do {
                d->outlineMapper->lineTo(*(++p));
            } while (p < ep);
            d->outlineMapper->endOutline();

            // scanconvert.
            ProcessSpans brushBlend = d->getBrushFunc(d->outlineMapper->controlPointRect,
                                                      &s->brushData);
            d->rasterize(d->outlineMapper->outline(), brushBlend, &s->brushData, d->rasterBuffer);
            d->outlineMapper->setCoordinateRounding(false);
        }
    }

    // Do the outline...
    if (s->penData.blend) {
        if (s->flags.fast_pen && s->lastPen.brush().isOpaque())
            strokePolygonCosmetic(points, pointCount, mode);
        else {
            int count = pointCount * 2;
            QVarLengthArray<qreal> fpoints(count);
#ifdef Q_WS_MAC
            for (int i=0; i<count; i+=2) {
                fpoints[i] = ((int *) points)[i+1];
                fpoints[i+1] = ((int *) points)[i];
            }
#else
            for (int i=0; i<count; ++i)
                fpoints[i] = ((int *) points)[i];
#endif
            QVectorPath vp((qreal *) fpoints.data(), pointCount, 0, QVectorPath::polygonFlags(mode));
            QPaintEngineEx::stroke(vp, s->lastPen);
        }
    }
}

/*!
    \internal
*/
void QRasterPaintEngine::strokePolygonCosmetic(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    Q_ASSERT(s->penData.blend);
    Q_ASSERT(s->flags.fast_pen);

    bool needs_closing = mode != PolylineMode && points[0] != points[pointCount-1];

    // Use fast path for 0 width /  trivial pens.
    QIntRect devRect;
    devRect.set(d->deviceRect);

    LineDrawMode mode_for_last = (s->lastPen.capStyle() != Qt::FlatCap
                                  ? LineDrawIncludeLastPixel
                                  : LineDrawNormal);
    int dashOffset = int(s->lastPen.dashOffset());

    const QPointF offs(aliasedCoordinateDelta, aliasedCoordinateDelta);

    // Draw all the line segments.
    for (int i=1; i<pointCount; ++i) {

        QPointF lp1 = points[i-1] * s->matrix + offs;
        QPointF lp2 = points[i] * s->matrix + offs;

        const QRectF brect(lp1, lp2);
        ProcessSpans penBlend = d->getPenFunc(brect, &s->penData);
        if (qpen_style(s->lastPen) == Qt::SolidLine) {
            drawLine_midpoint_i(qFloor(lp1.x()), qFloor(lp1.y()),
                                qFloor(lp2.x()), qFloor(lp2.y()),
                                penBlend, &s->penData,
                                i == pointCount - 1 ? mode_for_last : LineDrawIncludeLastPixel,
                                devRect);
        } else {
            drawLine_midpoint_dashed_i(qFloor(lp1.x()), qFloor(lp1.y()),
                                       qFloor(lp2.x()), qFloor(lp2.y()),
                                       &s->lastPen,
                                       penBlend, &s->penData,
                                       i == pointCount - 1 ? mode_for_last : LineDrawIncludeLastPixel,
                                       devRect, &dashOffset);
        }
    }

    // Polygons are implicitly closed.
    if (needs_closing) {
        QPointF lp1 = points[pointCount-1] * s->matrix + offs;
        QPointF lp2 = points[0] * s->matrix + offs;

        const QRectF brect(lp1, lp2);
        ProcessSpans penBlend = d->getPenFunc(brect, &s->penData);
        if (qpen_style(s->lastPen) == Qt::SolidLine) {
            drawLine_midpoint_i(qFloor(lp1.x()), qFloor(lp1.y()),
                                qFloor(lp2.x()), qFloor(lp2.y()),
                                penBlend, &s->penData,
                                LineDrawIncludeLastPixel,
                                devRect);
        } else {
            drawLine_midpoint_dashed_i(qFloor(lp1.x()), qFloor(lp1.y()),
                                       qFloor(lp2.x()), qFloor(lp2.y()),
                                       &s->lastPen,
                                       penBlend, &s->penData,
                                       LineDrawIncludeLastPixel,
                                       devRect, &dashOffset);
        }
    }

}

/*!
    \internal
*/
void QRasterPaintEngine::strokePolygonCosmetic(const QPoint *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    // We assert here because this function is called from drawRects
    // and drawPolygon and they already do ensurePen(), so we skip that
    // here to avoid duplicate checks..
    Q_ASSERT(s->penData.blend);

    bool needs_closing = mode != PolylineMode && points[0] != points[pointCount-1];

    QIntRect devRect;
    devRect.set(d->deviceRect);

    LineDrawMode mode_for_last = (s->lastPen.capStyle() != Qt::FlatCap
                                  ? LineDrawIncludeLastPixel
                                  : LineDrawNormal);

    int m11 = int(s->matrix.m11());
    int m22 = int(s->matrix.m22());
    int dx = int(s->matrix.dx());
    int dy = int(s->matrix.dy());
    int m13 = int(s->matrix.m13());
    int m23 = int(s->matrix.m23());
    bool affine = !m13 && !m23;

    int dashOffset = int(s->lastPen.dashOffset());

    if (affine) {
        // Draw all the line segments.
        for (int i=1; i<pointCount; ++i) {
            const QPoint lp1 = points[i-1] * s->matrix;
            const QPoint lp2 = points[i] * s->matrix;
            const QRect brect(lp1, lp2);
            ProcessSpans penBlend = d->getPenFunc(brect, &s->penData);

            if (qpen_style(s->lastPen) == Qt::SolidLine)
                drawLine_midpoint_i(lp1.x(), lp1.y(),
                                    lp2.x(), lp2.y(),
                                    penBlend, &s->penData,
                                    i == pointCount - 1 ? mode_for_last : LineDrawIncludeLastPixel,
                                    devRect);
            else
                drawLine_midpoint_dashed_i(lp1.x(), lp1.y(),
                                           lp2.x(), lp2.y(),
                                           &s->lastPen,
                                           penBlend, &s->penData,
                                           i == pointCount - 1 ? mode_for_last : LineDrawIncludeLastPixel,
                                           devRect, &dashOffset);

        }

        // Polygons are implicitly closed.
        if (needs_closing) {
            const QPoint lp1 = points[pointCount - 1] * s->matrix;
            const QPoint lp2 = points[0] * s->matrix;
            const QRect brect(lp1, lp2);
            ProcessSpans penBlend = d->getPenFunc(brect, &s->penData);

            if (qpen_style(s->lastPen) == Qt::SolidLine)
                drawLine_midpoint_i(lp1.x(), lp1.y(),
                                    lp2.x(), lp2.y(),
                                    penBlend, &s->penData, LineDrawIncludeLastPixel,
                                    devRect);
            else
                drawLine_midpoint_dashed_i(lp1.x(), lp1.y(),
                                           lp2.x(), lp2.y(),
                                           &s->lastPen,
                                           penBlend, &s->penData, LineDrawIncludeLastPixel,
                                           devRect, &dashOffset);
        }
    } else {
        // Draw all the line segments.
        for (int i=1; i<pointCount; ++i) {
            int x1 = points[i-1].x() * m11 + dx;
            int y1 = points[i-1].y() * m22 + dy;
            qreal w = m13*points[i-1].x() + m23*points[i-1].y() + 1.;
            w = 1/w;
            x1 = int(x1*w);
            y1 = int(y1*w);
            int x2 = points[i].x() * m11 + dx;
            int y2 = points[i].y() * m22 + dy;
            w = m13*points[i].x() + m23*points[i].y() + 1.;
            w = 1/w;
            x2 = int(x2*w);
            y2 = int(y2*w);

            const QRect brect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
            ProcessSpans penBlend = d->getPenFunc(brect, &s->penData);
            if (qpen_style(s->lastPen) == Qt::SolidLine)
                drawLine_midpoint_i(x1, y1, x2, y2,
                                    penBlend, &s->penData,
                                    i == pointCount - 1 ? mode_for_last : LineDrawIncludeLastPixel,
                                    devRect);
            else
                drawLine_midpoint_dashed_i(x1, y1, x2, y2,
                                           &s->lastPen,
                                           penBlend, &s->penData,
                                           i == pointCount - 1 ? mode_for_last : LineDrawIncludeLastPixel,
                                           devRect, &dashOffset);

        }

        int x1 = points[pointCount-1].x() * m11 + dx;
        int y1 = points[pointCount-1].y() * m22 + dy;
        qreal w = m13*points[pointCount-1].x() + m23*points[pointCount-1].y() + 1.;
        w = 1/w;
        x1 = int(x1*w);
        y1 = int(y1*w);
        int x2 = points[0].x() * m11 + dx;
        int y2 = points[0].y() * m22 + dy;
        w = m13*points[0].x() + m23*points[0].y() + 1.;
        w = 1/w;
        x2 = int(x2 * w);
        y2 = int(y2 * w);
        // Polygons are implicitly closed.

        if (needs_closing) {
            const QRect brect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
            ProcessSpans penBlend = d->getPenFunc(brect, &s->penData);
            if (qpen_style(s->lastPen) == Qt::SolidLine)
                drawLine_midpoint_i(x1, y1, x2, y2,
                                    penBlend, &s->penData, LineDrawIncludeLastPixel,
                                    devRect);
            else
                drawLine_midpoint_dashed_i(x1, y1, x2, y2,
                                           &s->lastPen,
                                           penBlend, &s->penData, LineDrawIncludeLastPixel,
                                           devRect, &dashOffset);
        }
    }
}

#define IMAGE_FROM_PIXMAP(pixmap) \
    pixmap.data->classId() == QPixmapData::RasterClass        \
        ? ((QRasterPixmapData *) pixmap.data)->image          \
        : pixmap.toImage()

/*!
    \internal
*/
void QRasterPaintEngine::drawPixmap(const QPointF &pos, const QPixmap &pixmap)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " - QRasterPaintEngine::drawPixmap(), pos=" << pos << " pixmap=" << pixmap.size() << "depth=" << pixmap.depth();
#endif
    if (pixmap.depth() == 1) {
        Q_D(QRasterPaintEngine);
        QRasterPaintEngineState *s = state();
        if (s->matrix.type() <= QTransform::TxTranslate) {
            drawBitmap(pos + QPointF(s->matrix.dx(), s->matrix.dy()), pixmap, &s->penData);
        } else {
            drawImage(pos, d->rasterBuffer->colorizeBitmap(IMAGE_FROM_PIXMAP(pixmap), s->pen.color()));
        }
    } else {
        QRasterPaintEngine::drawImage(pos, IMAGE_FROM_PIXMAP(pixmap));
    }
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pixmap, const QRectF &sr)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " - QRasterPaintEngine::drawPixmap(), r=" << r << " sr=" << sr << " pixmap=" << pixmap.size() << "depth=" << pixmap.depth();
#endif

    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    if (pixmap.depth() == 1) {
        if (s->matrix.type() <= QTransform::TxTranslate
            && r.size() == sr.size()
            && r.size() == pixmap.size()) {
            ensurePen();
            drawBitmap(r.topLeft() + QPointF(s->matrix.dx(), s->matrix.dy()), pixmap, &s->penData);
            return;
        } else {
            drawImage(r, d->rasterBuffer->colorizeBitmap(IMAGE_FROM_PIXMAP(pixmap),
                                                         s->pen.color()), sr);
        }
    } else {
        drawImage(r, IMAGE_FROM_PIXMAP(pixmap), sr);
    }
}

// assumes that rect has positive width and height
static inline const QRect toRect_normalized(const QRectF &rect)
{
    const int x = qRound(rect.x());
    const int y = qRound(rect.y());
    const int w = int(rect.width() + qreal(0.5));
    const int h = int(rect.height() + qreal(0.5));

    return QRect(x, y, w, h);
}

static inline int fast_ceil_positive(const qreal &v)
{
    const int iv = int(v);
    if (v - iv == 0)
        return iv;
    else
        return iv + 1;
}

static inline const QRect toAlignedRect_positive(const QRectF &rect)
{
    const int xmin = int(rect.x());
    const int xmax = int(fast_ceil_positive(rect.right()));
    const int ymin = int(rect.y());
    const int ymax = int(fast_ceil_positive(rect.bottom()));
    return QRect(xmin, ymin, xmax - xmin, ymax - ymin);
}

/*!
    \internal
*/
void QRasterPaintEngine::drawImage(const QPointF &p, const QImage &img)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " - QRasterPaintEngine::drawImage(), p=" <<  p << " image=" << img.size() << "depth=" << img.depth();
#endif

    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    if (s->matrix.type() > QTransform::TxTranslate) {
        drawImage(QRectF(p.x(), p.y(), img.width(), img.height()),
                  img,
                  QRectF(0, 0, img.width(), img.height()));
    } else {

        const QClipData *clip = d->clip();
        QPointF pt(p.x() + s->matrix.dx(), p.y() + s->matrix.dy());

        // ### TODO: remove this eventually...
        static bool NO_BLEND_FUNC = !qgetenv("QT_NO_BLEND_FUNCTIONS").isNull();

        if (s->flags.fast_images && !NO_BLEND_FUNC) {
            SrcOverBlendFunc func = qBlendFunctions[d->rasterBuffer->format][img.format()];
            if (func) {
                if (!clip) {
                    d->drawImage(pt, img, func, d->deviceRect, s->intOpacity);
                    return;
                } else if (clip->hasRectClip) {
                    d->drawImage(pt, img, func, clip->clipRect, s->intOpacity);
                    return;
                }
            }
        }

        d->image_filler.clip = clip;
        d->image_filler.initTexture(&img, s->intOpacity, QTextureData::Plain, img.rect());
        if (!d->image_filler.blend)
            return;
        d->image_filler.dx = -pt.x();
        d->image_filler.dy = -pt.y();
        QRect rr = img.rect().translated(qRound(pt.x()), qRound(pt.y()));

        fillRect_normalized(rr, &d->image_filler, d);
    }

}

QRectF qt_mapRect_non_normalizing(const QRectF &r, const QTransform &t)
{
    return QRectF(r.topLeft() * t, r.bottomRight() * t);
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawImage(const QRectF &r, const QImage &img, const QRectF &sr,
                                   Qt::ImageConversionFlags)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " - QRasterPaintEngine::drawImage(), r=" << r << " sr=" << sr << " image=" << img.size() << "depth=" << img.depth();
#endif

    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();
    const bool aa = s->flags.antialiased || s->flags.bilinear;
    if (!aa && sr.size() == QSize(1, 1)) {
        fillRect(r, QColor::fromRgba(img.pixel(sr.x(), sr.y())));
        return;
    }

    bool stretch_sr = r.width() != sr.width() || r.height() != sr.height();

    const QClipData *clip = d->clip();

    if (s->matrix.type() > QTransform::TxTranslate || stretch_sr) {

        if (s->flags.fast_images) {
            SrcOverScaleFunc func = qScaleFunctions[d->rasterBuffer->format][img.format()];
            if (func && (!clip || clip->hasRectClip)) {
                func(d->rasterBuffer->buffer(), d->rasterBuffer->bytesPerLine(),
                     img.bits(), img.bytesPerLine(),
                     qt_mapRect_non_normalizing(r, s->matrix), sr,
                     !clip ? d->deviceRect : clip->clipRect,
                     s->intOpacity);
                return;
            }
        }

        QTransform copy = s->matrix;
        copy.translate(r.x(), r.y());
        if (stretch_sr)
            copy.scale(r.width() / sr.width(), r.height() / sr.height());
        copy.translate(-sr.x(), -sr.y());

        d->image_filler_xform.clip = clip;
        d->image_filler_xform.initTexture(&img, s->intOpacity, QTextureData::Plain, toAlignedRect_positive(sr));
        if (!d->image_filler_xform.blend)
            return;
        d->image_filler_xform.setupMatrix(copy, s->flags.bilinear);

#ifdef QT_FAST_SPANS
        ensureState();
        if (s->flags.tx_noshear || s->matrix.type() == QTransform::TxScale) {
            d->initializeRasterizer(&d->image_filler_xform);
            d->rasterizer->setAntialiased(aa);

            const QPointF offs = aa ? QPointF() : QPointF(aliasedCoordinateDelta, aliasedCoordinateDelta);

            const QRectF &rect = r.normalized();
            const QPointF a = s->matrix.map((rect.topLeft() + rect.bottomLeft()) * 0.5f) - offs;
            const QPointF b = s->matrix.map((rect.topRight() + rect.bottomRight()) * 0.5f) - offs;

            if (s->flags.tx_noshear)
                d->rasterizer->rasterizeLine(a, b, rect.height() / rect.width());
            else
                d->rasterizer->rasterizeLine(a, b, qAbs((s->matrix.m22() * rect.height()) / (s->matrix.m11() * rect.width())));
            return;
        }
#endif
        bool wasAntialiased = s->flags.antialiased;
        if (!s->flags.antialiased)
            s->flags.antialiased = s->flags.bilinear;
        const qreal offs = s->flags.antialiased ? qreal(0) : aliasedCoordinateDelta;
        QPainterPath path;
        path.addRect(r);
        QTransform m = s->matrix;
        s->matrix = QTransform(m.m11(), m.m12(), m.m13(),
                               m.m21(), m.m22(), m.m23(),
                               m.m31() - offs, m.m32() - offs, m.m33());
        fillPath(path, &d->image_filler_xform);
        s->matrix = m;
        s->flags.antialiased = wasAntialiased;
    } else {

        if (s->flags.fast_images) {
            SrcOverBlendFunc func = qBlendFunctions[d->rasterBuffer->format][img.format()];
            if (func) {
                QPointF pt(r.x() + s->matrix.dx(), r.y() + s->matrix.dy());
                if (!clip) {
                    d->drawImage(pt, img, func, d->deviceRect, s->intOpacity, sr.toRect());
                    return;
                } else if (clip->hasRectClip) {
                    d->drawImage(pt, img, func, clip->clipRect, s->intOpacity, sr.toRect());
                    return;
                }
            }
        }

        d->image_filler.clip = clip;
        d->image_filler.initTexture(&img, s->intOpacity, QTextureData::Plain, toAlignedRect_positive(sr));
        if (!d->image_filler.blend)
            return;
        d->image_filler.dx = -(r.x() + s->matrix.dx()) + sr.x();
        d->image_filler.dy = -(r.y() + s->matrix.dy()) + sr.y();

        QRectF rr = r;
        rr.translate(s->matrix.dx(), s->matrix.dy());
        fillRect_normalized(toRect_normalized(rr), &d->image_filler, d);
    }
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &sr)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " - QRasterPaintEngine::drawTiledPixmap(), r=" << r << "pixmap=" << pixmap.size();
#endif
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    QImage image;
    if (pixmap.depth() == 1)
        image = d->rasterBuffer->colorizeBitmap(IMAGE_FROM_PIXMAP(pixmap), s->pen.color());
    else
        image = IMAGE_FROM_PIXMAP(pixmap);

    if (s->matrix.type() > QTransform::TxTranslate) {
        QTransform copy = s->matrix;
        copy.translate(r.x(), r.y());
        copy.translate(-sr.x(), -sr.y());
        d->image_filler_xform.clip = d->clip();
        d->image_filler_xform.initTexture(&image, s->intOpacity, QTextureData::Tiled);
        if (!d->image_filler_xform.blend)
            return;
        d->image_filler_xform.setupMatrix(copy, s->flags.bilinear);

#ifdef QT_FAST_SPANS
        ensureState();
        if (s->flags.tx_noshear || s->matrix.type() == QTransform::TxScale) {
            d->initializeRasterizer(&d->image_filler_xform);
            d->rasterizer->setAntialiased(s->flags.antialiased || s->flags.bilinear);

            const QRectF &rect = r.normalized();
            const QPointF a = s->matrix.map((rect.topLeft() + rect.bottomLeft()) * 0.5f);
            const QPointF b = s->matrix.map((rect.topRight() + rect.bottomRight()) * 0.5f);
            if (s->flags.tx_noshear)
                d->rasterizer->rasterizeLine(a, b, rect.height() / rect.width());
            else
                d->rasterizer->rasterizeLine(a, b, qAbs((s->matrix.m22() * rect.height()) / (s->matrix.m11() * rect.width())));
            return;
        }
#endif
        bool wasAntialiased = s->flags.antialiased;
        if (!s->flags.antialiased)
            s->flags.antialiased = s->flags.bilinear;
        QPainterPath path;
        path.addRect(r);
        fillPath(path, &d->image_filler_xform);
        s->flags.antialiased = wasAntialiased;
    } else {
        d->image_filler.clip = d->clip();

        d->image_filler.initTexture(&image, s->intOpacity, QTextureData::Tiled);
        if (!d->image_filler.blend)
            return;
        d->image_filler.dx = -(r.x() + s->matrix.dx()) + sr.x();
        d->image_filler.dy = -(r.y() + s->matrix.dy()) + sr.y();

        QRectF rr = r;
        rr.translate(s->matrix.dx(), s->matrix.dy());
        fillRect_normalized(rr.toRect().normalized(), &d->image_filler, d);
    }
}


//QWS hack
static inline bool monoVal(const uchar* s, int x)
{
    return  (s[x>>3] << (x&7)) & 0x80;
}

/*!
    \internal
*/
void QRasterPaintEngine::alphaPenBlt(const void* src, int bpl, int depth, int rx,int ry,int w,int h)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    if (!s->penData.blend)
        return;

    QRasterBuffer *rb = d->rasterBuffer;

    const QRect rect(rx, ry, w, h);
    const QClipData *clip = d->clip();
    bool unclipped = false;
    if (clip) {
        // inlined QRect::intersects
        const bool intersects = qMax(clip->xmin, rect.left()) <= qMin(clip->xmax - 1, rect.right())
                                && qMax(clip->ymin, rect.top()) <= qMin(clip->ymax - 1, rect.bottom());

        if (clip->hasRectClip) {
            unclipped = rx > clip->xmin
                        && rx + w < clip->xmax
                        && ry > clip->ymin
                        && ry + h < clip->ymax;
        }

        if (!intersects)
            return;
    } else {
        // inlined QRect::intersects
        const bool intersects = qMax(0, rect.left()) <= qMin(rb->width() - 1, rect.right())
                                && qMax(0, rect.top()) <= qMin(rb->height() - 1, rect.bottom());
        if (!intersects)
            return;

        // inlined QRect::contains
        const bool contains = rect.left() >= 0 && rect.right() < rb->width()
                              && rect.top() >= 0 && rect.bottom() < rb->height();

        unclipped = contains && d->isUnclipped_normalized(rect);
    }

    ProcessSpans blend = unclipped ? s->penData.unclipped_blend : s->penData.blend;
    const uchar * scanline = static_cast<const uchar *>(src);

    if (s->flags.fast_text) {
        if (unclipped) {
            if (depth == 1) {
                if (s->penData.bitmapBlit) {
                    s->penData.bitmapBlit(rb, rx, ry, s->penData.solid.color,
                                          scanline, w, h, bpl);
                    return;
                }
            } else if (depth == 8) {
                if (s->penData.alphamapBlit) {
                    s->penData.alphamapBlit(rb, rx, ry, s->penData.solid.color,
                                            scanline, w, h, bpl, 0);
                    return;
                }
            } else if (depth == 32) {
                // (A)RGB Alpha mask where the alpha component is not used.
                if (s->penData.alphaRGBBlit) {
                    s->penData.alphaRGBBlit(rb, rx, ry, s->penData.solid.color,
                                            (const uint *) scanline, w, h, bpl / 4, 0);
                    return;
                }
            }
        } else if (d->deviceDepth == 32 && (depth == 8 || depth == 32)) {
            // (A)RGB Alpha mask where the alpha component is not used.
            if (!clip) {
                int nx = qMax(0, rx);
                int ny = qMax(0, ry);

                // Move scanline pointer to compensate for moved x and y
                int xdiff = nx - rx;
                int ydiff = ny - ry;
                scanline += ydiff * bpl;
                scanline += xdiff * (depth == 32 ? 4 : 1);

                w -= xdiff;
                h -= ydiff;

                if (nx + w > d->rasterBuffer->width())
                    w = d->rasterBuffer->width() - nx;
                if (ny + h > d->rasterBuffer->height())
                    h = d->rasterBuffer->height() - ny;

                rx = nx;
                ry = ny;
            }
            if (depth == 8 && s->penData.alphamapBlit) {
                s->penData.alphamapBlit(rb, rx, ry, s->penData.solid.color,
                                        scanline, w, h, bpl, clip);
            } else if (depth == 32 && s->penData.alphaRGBBlit) {
                s->penData.alphaRGBBlit(rb, rx, ry, s->penData.solid.color,
                                        (const uint *) scanline, w, h, bpl / 4, clip);
            }
            return;
        }
    }

    int x0 = 0;
    if (rx < 0) {
        x0 = -rx;
        w -= x0;
    }

    int y0 = 0;
    if (ry < 0) {
        y0 = -ry;
        scanline += bpl * y0;
        h -= y0;
    }

    w = qMin(w, rb->width() - qMax(0, rx));
    h = qMin(h, rb->height() - qMax(0, ry));

    if (w <= 0 || h <= 0)
        return;

    const int NSPANS = 256;
    QSpan spans[NSPANS];
    int current = 0;

    const int x1 = x0 + w;
    const int y1 = y0 + h;

    if (depth == 1) {
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ) {
                if (!monoVal(scanline, x)) {
                    ++x;
                    continue;
                }

                if (current == NSPANS) {
                    blend(current, spans, &s->penData);
                    current = 0;
                }
                spans[current].x = x + rx;
                spans[current].y = y + ry;
                spans[current].coverage = 255;
                int len = 1;
                ++x;
                // extend span until we find a different one.
                while (x < x1 && monoVal(scanline, x)) {
                    ++x;
                    ++len;
                }
                spans[current].len = len;
                ++current;
            }
            scanline += bpl;
        }
    } else if (depth == 8) {
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ) {
                // Skip those with 0 coverage
                if (scanline[x] == 0) {
                    ++x;
                    continue;
                }

                if (current == NSPANS) {
                    blend(current, spans, &s->penData);
                    current = 0;
                }
                int coverage = scanline[x];
                spans[current].x = x + rx;
                spans[current].y = y + ry;
                spans[current].coverage = coverage;
                int len = 1;
                ++x;

                // extend span until we find a different one.
                while (x < x1 && scanline[x] == coverage) {
                    ++x;
                    ++len;
                }
                spans[current].len = len;
                ++current;
            }
            scanline += bpl;
        }
    } else { // 32-bit alpha...
        uint *sl = (uint *) src;
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ) {
                // Skip those with 0 coverage
                if ((sl[x] & 0x00ffffff) == 0) {
                    ++x;
                    continue;
                }

                if (current == NSPANS) {
                    blend(current, spans, &s->penData);
                    current = 0;
                }
                uint rgbCoverage = sl[x];
                int coverage = qGreen(rgbCoverage);
                spans[current].x = x + rx;
                spans[current].y = y + ry;
                spans[current].coverage = coverage;
                int len = 1;
                ++x;

                // extend span until we find a different one.
                while (x < x1 && sl[x] == rgbCoverage) {
                    ++x;
                    ++len;
                }
                spans[current].len = len;
                ++current;
            }
            sl += bpl / sizeof(uint);
        }
    }
//     qDebug() << "alphaPenBlt: num spans=" << current
//              << "span:" << spans->x << spans->y << spans->len << spans->coverage;
        // Call span func for current set of spans.
    if (current != 0)
        blend(current, spans, &s->penData);
}

// void QRasterPaintEngine::drawCachedGlyphs(const QPointF &p, const QTextItemInt &ti)
// {
//     Q_D(QRasterPaintEngine);
//     QRasterPaintEngineState *s = state();
// 
//     QVarLengthArray<QFixedPoint> positions;
//     QVarLengthArray<glyph_t> glyphs;
//     QTransform matrix = s->matrix;
//     matrix.translate(p.x(), p.y());
//     ti.fontEngine->getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
// 
//     QFontEngineGlyphCache::Type glyphType = ti.fontEngine->glyphFormat >= 0 ? QFontEngineGlyphCache::Type(ti.fontEngine->glyphFormat) : d->glyphCacheType;
// 
//     QImageTextureGlyphCache *cache =
//         (QImageTextureGlyphCache *) ti.fontEngine->glyphCache(glyphType, s->matrix);
//     if (!cache) {
//         cache = new QImageTextureGlyphCache(glyphType, s->matrix);
//         ti.fontEngine->setGlyphCache(glyphType, cache);
//     }
// 
//     cache->populate(ti, glyphs, positions);
// 
//     const QImage &image = cache->image();
//     int bpl = image.bytesPerLine();
// 
//     int depth = image.depth();
//     int rightShift = 0;
//     int leftShift = 0;
//     if (depth == 32)
//         leftShift = 2; // multiply by 4
//     else if (depth == 1)
//         rightShift = 3; // divide by 8
// 
//     int margin = cache->glyphMargin();
// 
//     const QFixed offs = QFixed::fromReal(aliasedCoordinateDelta);
// 
//     const uchar *bits = image.bits();
//     for (int i=0; i<glyphs.size(); ++i) {
//         const QTextureGlyphCache::Coord &c = cache->coords.value(glyphs[i]);
//         int x = qFloor(positions[i].x + offs) + c.baseLineX - margin;
//         int y = qFloor(positions[i].y + offs) - c.baseLineY - margin;
// 
// //         printf("drawing [%d %d %d %d] baseline [%d %d], glyph: %d, to: %d %d, pos: %d %d\n",
// //                c.x, c.y,
// //                c.w, c.h,
// //                c.baseLineX, c.baseLineY,
// //                glyphs[i],
// //                x, y,
// //                positions[i].x.toInt(), positions[i].y.toInt());
// 
//         alphaPenBlt(bits + ((c.x << leftShift) >> rightShift) + c.y * bpl, bpl, depth, x, y, c.w, c.h);
//     }
// 
//     return;
// }
// 
// 
// 
/*!
 * Returns true if the rectangle is completly within the current clip
 * state of the paint engine.
 */
bool QRasterPaintEnginePrivate::isUnclipped_normalized(const QRect &r) const
{
    const QClipData *cl = clip();
    if (!cl) {
        // inline contains() for performance (we know the rects are normalized)
        const QRect &r1 = deviceRect;
        return (r.left() >= r1.left() && r.right() <= r1.right()
                && r.top() >= r1.top() && r.bottom() <= r1.bottom());
    }


    // currently all painting functions clips to deviceRect internally
    if (cl->clipRect == deviceRect)
        return true;

    if (cl->hasRegionClip) {
        // inline contains() for performance (we know the rects are normalized)
        const QRect &r1 = cl->clipRect;
        return (r.left() >= r1.left() && r.right() <= r1.right()
                && r.top() >= r1.top() && r.bottom() <= r1.bottom());
    } else {
        return qt_region_strictContains(cl->clipRegion, r);
    }
}

bool QRasterPaintEnginePrivate::isUnclipped(const QRect &rect,
                                            int penWidth) const
{
    Q_Q(const QRasterPaintEngine);
    const QRasterPaintEngineState *s = q->state();
    const QClipData *cl = clip();
    if (!cl) {
        QRect r = rect.normalized();
        // inline contains() for performance (we know the rects are normalized)
        const QRect &r1 = deviceRect;
        return (r.left() >= r1.left() && r.right() <= r1.right()
                && r.top() >= r1.top() && r.bottom() <= r1.bottom());
    }


    // currently all painting functions that call this function clip to deviceRect internally
    if (cl->clipRect == deviceRect)
        return true;

    if (s->flags.antialiased)
        ++penWidth;

    QRect r = rect.normalized();
    if (penWidth > 0) {
        r.setX(r.x() - penWidth);
        r.setY(r.y() - penWidth);
        r.setWidth(r.width() + 2 * penWidth);
        r.setHeight(r.height() + 2 * penWidth);
    }

    if (!cl->clipRect.isEmpty()) {
        // inline contains() for performance (we know the rects are normalized)
        const QRect &r1 = cl->clipRect;
        return (r.left() >= r1.left() && r.right() <= r1.right()
                && r.top() >= r1.top() && r.bottom() <= r1.bottom());
    } else {
        return qt_region_strictContains(cl->clipRegion, r);
    }
}

inline bool QRasterPaintEnginePrivate::isUnclipped(const QRectF &rect,
                                                   int penWidth) const
{
    return isUnclipped(rect.normalized().toAlignedRect(), penWidth);
}

inline ProcessSpans
QRasterPaintEnginePrivate::getBrushFunc(const QRect &rect,
                                        const QSpanData *data) const
{
    return isUnclipped(rect, 0) ? data->unclipped_blend : data->blend;
}

inline ProcessSpans
QRasterPaintEnginePrivate::getBrushFunc(const QRectF &rect,
                                        const QSpanData *data) const
{
    return isUnclipped(rect, 0) ? data->unclipped_blend : data->blend;
}

inline ProcessSpans
QRasterPaintEnginePrivate::getPenFunc(const QRect &rect,
                                      const QSpanData *data) const
{
    Q_Q(const QRasterPaintEngine);
    const QRasterPaintEngineState *s = q->state();

    if (!s->flags.fast_pen && s->matrix.type() > QTransform::TxTranslate)
        return data->blend;
    const int penWidth = s->flags.fast_pen ? 1 : qCeil(s->pen.widthF());
    return isUnclipped(rect, penWidth) ? data->unclipped_blend : data->blend;
}

inline ProcessSpans
QRasterPaintEnginePrivate::getPenFunc(const QRectF &rect,
                                      const QSpanData *data) const
{
    Q_Q(const QRasterPaintEngine);
    const QRasterPaintEngineState *s = q->state();

    if (!s->flags.fast_pen && s->matrix.type() > QTransform::TxTranslate)
        return data->blend;
    const int penWidth = s->flags.fast_pen ? 1 : qCeil(s->lastPen.widthF());
    return isUnclipped(rect, penWidth) ? data->unclipped_blend : data->blend;
}

/*!
    \reimp
*/
// void QRasterPaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
// {
//     const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);
//     QRasterPaintEngineState *s = state();
// 
// #ifdef QT_DEBUG_DRAW
//     Q_D(QRasterPaintEngine);
//     fprintf(stderr," - QRasterPaintEngine::drawTextItem(), (%.2f,%.2f), string=%s ct=%d\n",
//            p.x(), p.y(), QString::fromRawData(ti.chars, ti.num_chars).toLatin1().data(),
//            d->glyphCacheType);
// #endif
// 
//     ensurePen();
//     ensureState();
// 
// #if defined (Q_WS_WIN) || defined(Q_WS_MAC)
// 
//     bool drawCached = true;
// 
//     if (s->matrix.type() >= QTransform::TxProject)
//         drawCached = false;
// 
//     // don't try to cache huge fonts
//     if (ti.fontEngine->fontDef.pixelSize * qSqrt(s->matrix.determinant()) >= 64)
//         drawCached = false;
// 
//     // ### Remove the TestFontEngine and Box engine crap, in these
//     // ### cases we should delegate painting to the font engine
//     // ### directly...
// 
// #if defined(Q_WS_WIN) && !defined(Q_OS_WINCE)
//     QFontEngine::Type fontEngineType = ti.fontEngine->type();
//     // qDebug() << "type" << fontEngineType << s->matrix.type();
//     if ((fontEngineType == QFontEngine::Win && !((QFontEngineWin *) ti.fontEngine)->ttf && s->matrix.type() > QTransform::TxTranslate)
//         || (s->matrix.type() <= QTransform::TxTranslate
//             && (fontEngineType == QFontEngine::TestFontEngine
//                 || fontEngineType == QFontEngine::Box))) {
//             drawCached = false;
//     }
// #else
//     if (s->matrix.type() > QTransform::TxTranslate)
//         drawCached = false;
// #endif
//     if (drawCached) {
//         drawCachedGlyphs(p, ti);
//         return;
//     }
// 
// #else // Q_WS_WIN || Q_WS_MAC
// 
//     QFontEngine *fontEngine = ti.fontEngine;
// 
// #if defined(Q_WS_QWS)
//     if (fontEngine->type() == QFontEngine::Box) {
//         fontEngine->draw(this, qFloor(p.x() + aliasedCoordinateDelta), qFloor(p.y() + aliasedCoordinateDelta), ti);
//         return;
//     }
// 
//     if (s->matrix.type() < QTransform::TxScale
//         && (fontEngine->type() == QFontEngine::QPF1 || fontEngine->type() == QFontEngine::QPF2
//             || (fontEngine->type() == QFontEngine::Proxy
//                 && !(static_cast<QProxyFontEngine *>(fontEngine)->drawAsOutline()))
//             )) {
//         fontEngine->draw(this, qFloor(p.x() + aliasedCoordinateDelta), qFloor(p.y() + aliasedCoordinateDelta), ti);
//         return;
//     }
// #endif // Q_WS_QWS
// 
// #if (defined(Q_WS_X11) || defined(Q_WS_QWS)) && !defined(QT_NO_FREETYPE)
// 
// #if defined(Q_WS_QWS) && !defined(QT_NO_QWS_QPF2)
//     if (fontEngine->type() == QFontEngine::QPF2) {
//         QFontEngine *renderingEngine = static_cast<QFontEngineQPF *>(fontEngine)->renderingEngine();
//         if (renderingEngine)
//             fontEngine = renderingEngine;
//     }
// #endif
// 
//     if (fontEngine->type() != QFontEngine::Freetype) {
//         QPaintEngineEx::drawTextItem(p, ti);
//         return;
//     }
// 
//     QFontEngineFT *fe = static_cast<QFontEngineFT *>(fontEngine);
// 
//     QTransform matrix = s->matrix;
//     matrix.translate(p.x(), p.y());
// 
//     QVarLengthArray<QFixedPoint> positions;
//     QVarLengthArray<glyph_t> glyphs;
//     fe->getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
//     if (glyphs.size() == 0)
//         return;
// 
//     // only use subpixel antialiasing when drawing to widgets
//     QFontEngineFT::GlyphFormat neededFormat =
//         painter()->device()->devType() == QInternal::Widget
//         ? fe->defaultGlyphFormat()
//         : QFontEngineFT::Format_A8;
// 
//     if (d_func()->mono_surface
//         || fe->isBitmapFont() // alphaPenBlt can handle mono, too
//         )
//         neededFormat = QFontEngineFT::Format_Mono;
// 
//     if (neededFormat == QFontEngineFT::Format_None)
//         neededFormat = QFontEngineFT::Format_A8;
// 
//     QFontEngineFT::QGlyphSet *gset = fe->defaultGlyphs();
//     if (s->matrix.type() >= QTransform::TxScale) {
//         if (s->matrix.isAffine())
//             gset = fe->loadTransformedGlyphSet(s->matrix);
//         else
//             gset = 0;
// 
//     }
// 
//     if (!gset || gset->outline_drawing
//         || !fe->loadGlyphs(gset, glyphs.data(), glyphs.size(), neededFormat))
//     {
//         QPaintEngine::drawTextItem(p, ti);
//         return;
//     }
// 
//     QFixed offs = QFixed::fromReal(aliasedCoordinateDelta);
//     FT_Face lockedFace = 0;
// 
//     int depth;
//     switch (neededFormat) {
//     case QFontEngineFT::Format_Mono:
//         depth = 1;
//         break;
//     case QFontEngineFT::Format_A8:
//         depth = 8;
//         break;
//     case QFontEngineFT::Format_A32:
//         depth = 32;
//         break;
//     default:
//         Q_ASSERT(false);
//     };
// 
//     for(int i = 0; i < glyphs.size(); i++) {
//         QFontEngineFT::Glyph *glyph = gset->glyph_data.value(glyphs[i]);
// 
//         if (!glyph || glyph->format != neededFormat) {
//             if (!lockedFace)
//                 lockedFace = fe->lockFace();
//             glyph = fe->loadGlyph(gset, glyphs[i], neededFormat);
//         }
// 
//         if (!glyph || !glyph->data)
//             continue;
// 
//         int pitch;
//         switch (neededFormat) {
//         case QFontEngineFT::Format_Mono:
//             pitch = ((glyph->width + 31) & ~31) >> 3;
//             break;
//         case QFontEngineFT::Format_A8:
//             pitch = (glyph->width + 3) & ~3;
//             break;
//         case QFontEngineFT::Format_A32:
//             pitch = glyph->width * 4;
//             break;
//         default:
//             Q_ASSERT(false);
//         };
// 
//         alphaPenBlt(glyph->data, pitch, depth,
//                     qFloor(positions[i].x + offs) + glyph->x,
//                     qFloor(positions[i].y + offs) - glyph->y,
//                     glyph->width, glyph->height);
//     }
//     if (lockedFace)
//         fe->unlockFace();
//     return;
// 
// #endif
// #endif
// 
//     QPaintEngineEx::drawTextItem(p, ti);
// }
// 
/*!
    \reimp
*/
void QRasterPaintEngine::drawPoints(const QPointF *points, int pointCount)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    ensurePen();
    qreal pw = s->lastPen.widthF();
    if (!s->flags.fast_pen && (s->matrix.type() > QTransform::TxTranslate || pw > 1)) {
        QPaintEngineEx::drawPoints(points, pointCount);

    } else {
        if (!s->penData.blend)
            return;

        QVarLengthArray<QT_FT_Span, 4096> array(pointCount);
        QT_FT_Span span = { 0, 1, 0, 255 };
        const QPointF *end = points + pointCount;
        qreal trans_x, trans_y;
        int x, y;
        int left = d->deviceRect.x();
        int right = left + d->deviceRect.width();
        int top = d->deviceRect.y();
        int bottom = top + d->deviceRect.height();
        int count = 0;
        while (points < end) {
            s->matrix.map(points->x(), points->y(), &trans_x, &trans_y);
            x = qFloor(trans_x);
            y = qFloor(trans_y);
            if (x >= left && x < right && y >= top && y < bottom) {
                if (count > 0) {
                    const QT_FT_Span &last = array[count - 1];
                    // spans must be sorted on y (primary) and x (secondary)
                    if (y < last.y || (y == last.y && x < last.x)) {
                        s->penData.blend(count, array.constData(), &s->penData);
                        count = 0;
                    }
                }

                span.x = x;
                span.y = y;
                array[count++] = span;
            }
            ++points;
        }

        if (count > 0)
            s->penData.blend(count, array.constData(), &s->penData);
    }
}


void QRasterPaintEngine::drawPoints(const QPoint *points, int pointCount)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    ensurePen();
    double pw = s->lastPen.widthF();
    if (!s->flags.fast_pen && (s->matrix.type() > QTransform::TxTranslate || pw > 1)) {
        QPaintEngineEx::drawPoints(points, pointCount);

    } else {
        if (!s->penData.blend)
            return;

        QVarLengthArray<QT_FT_Span, 4096> array(pointCount);
        QT_FT_Span span = { 0, 1, 0, 255 };
        const QPoint *end = points + pointCount;
        qreal trans_x, trans_y;
        int x, y;
        int left = d->deviceRect.x();
        int right = left + d->deviceRect.width();
        int top = d->deviceRect.y();
        int bottom = top + d->deviceRect.height();
        int count = 0;
        while (points < end) {
            s->matrix.map(points->x(), points->y(), &trans_x, &trans_y);
            x = qFloor(trans_x);
            y = qFloor(trans_y);
            if (x >= left && x < right && y >= top && y < bottom) {
                if (count > 0) {
                    const QT_FT_Span &last = array[count - 1];
                    // spans must be sorted on y (primary) and x (secondary)
                    if (y < last.y || (y == last.y && x < last.x)) {
                        s->penData.blend(count, array.constData(), &s->penData);
                        count = 0;
                    }
                }

                span.x = x;
                span.y = y;
                array[count++] = span;
            }
            ++points;
        }

        if (count > 0)
            s->penData.blend(count, array.constData(), &s->penData);
    }
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawLines(const QLine *lines, int lineCount)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " - QRasterPaintEngine::drawLine()";
#endif
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    ensurePen();
    if (s->flags.fast_pen) {
        QIntRect bounds; bounds.set(d->deviceRect);
        LineDrawMode mode = s->lastPen.capStyle() == Qt::FlatCap
                            ? LineDrawNormal
                            : LineDrawIncludeLastPixel;

        int m11 = int(s->matrix.m11());
        int m22 = int(s->matrix.m22());
        int dx = qFloor(s->matrix.dx() + aliasedCoordinateDelta);
        int dy = qFloor(s->matrix.dy() + aliasedCoordinateDelta);
        int dashOffset = int(s->lastPen.dashOffset());
        for (int i=0; i<lineCount; ++i) {
            if (s->flags.int_xform) {
                const QLine &l = lines[i];
                int x1 = l.x1() * m11 + dx;
                int y1 = l.y1() * m22 + dy;
                int x2 = l.x2() * m11 + dx;
                int y2 = l.y2() * m22 + dy;

                const QRect brect(QPoint(x1, y1), QPoint(x2, y2));
                ProcessSpans penBlend = d->getPenFunc(brect, &s->penData);
                if (qpen_style(s->lastPen) == Qt::SolidLine)
                    drawLine_midpoint_i(x1, y1, x2, y2,
                                        penBlend, &s->penData, mode, bounds);
                else
                    drawLine_midpoint_dashed_i(x1, y1, x2, y2,
                                               &s->lastPen, penBlend,
                                               &s->penData, mode, bounds,
                                               &dashOffset);
            } else {
                QLineF line = lines[i] * s->matrix;
                const QRectF brect(QPointF(line.x1(), line.y1()),
                                   QPointF(line.x2(), line.y2()));
                ProcessSpans penBlend = d->getPenFunc(brect, &s->penData);
                if (qpen_style(s->lastPen) == Qt::SolidLine)
                    drawLine_midpoint_i(int(line.x1()), int(line.y1()),
                                        int(line.x2()), int(line.y2()),
                                        penBlend, &s->penData, mode, bounds);
                else
                    drawLine_midpoint_dashed_i(int(line.x1()), int(line.y1()),
                                               int(line.x2()), int(line.y2()),
                                               &s->lastPen, penBlend,
                                               &s->penData, mode, bounds,
                                               &dashOffset);
            }
        }
    } else if (s->penData.blend) {
        QPaintEngineEx::drawLines(lines, lineCount);
    }
}

void QRasterPaintEnginePrivate::rasterizeLine_dashed(QLineF line,
                                                     qreal width,
                                                     int *dashIndex,
                                                     qreal *dashOffset,
                                                     bool *inDash)
{
    Q_Q(QRasterPaintEngine);
    QRasterPaintEngineState *s = q->state();

    const QPen &pen = s->lastPen;
    const bool squareCap = (pen.capStyle() == Qt::SquareCap);
    const QVector<qreal> pattern = pen.dashPattern();

    qreal length = line.length();
    Q_ASSERT(length > 0);
    while (length > 0) {
        const bool rasterize = *inDash;
        qreal dash = (pattern.at(*dashIndex) - *dashOffset) * width;
        QLineF l = line;

        if (dash >= length) {
            dash = length;
            *dashOffset += dash;
            length = 0;
        } else {
            *dashOffset = 0;
            *inDash = !(*inDash);
            *dashIndex = (*dashIndex + 1) % pattern.size();
            length -= dash;
            l.setLength(dash);
            line.setP1(l.p2());
        }

        if (rasterize && dash != 0)
            rasterizer->rasterizeLine(l.p1(), l.p2(), width / dash, squareCap);
    }
}

/*!
    \reimp
*/
void QRasterPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " - QRasterPaintEngine::drawLine()";
#endif
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    ensurePen();
    if (!s->penData.blend)
        return;
    if (s->flags.fast_pen) {
        QIntRect bounds;
        bounds.set(d->deviceRect);
        LineDrawMode mode = s->lastPen.capStyle() == Qt::FlatCap
                            ? LineDrawNormal
                            : LineDrawIncludeLastPixel;

        int dashOffset = int(s->lastPen.dashOffset());
        for (int i=0; i<lineCount; ++i) {
            QLineF line = (lines[i] * s->matrix).translated(aliasedCoordinateDelta, aliasedCoordinateDelta);
            const QRectF brect(QPointF(line.x1(), line.y1()),
                               QPointF(line.x2(), line.y2()));
            ProcessSpans penBlend = d->getPenFunc(brect, &s->penData);
            if (qpen_style(s->lastPen) == Qt::SolidLine)
                drawLine_midpoint_i(int(line.x1()), int(line.y1()),
                                    int(line.x2()), int(line.y2()),
                                    penBlend, &s->penData, mode, bounds);
            else
                drawLine_midpoint_dashed_i(int(line.x1()), int(line.y1()),
                                           int(line.x2()), int(line.y2()),
                                           &s->lastPen,
                                           penBlend, &s->penData, mode,
                                           bounds, &dashOffset);
        }
    } else {
        QPaintEngineEx::drawLines(lines, lineCount);
    }
}


/*!
    \reimp
*/
void QRasterPaintEngine::drawEllipse(const QRectF &rect)
{
    Q_D(QRasterPaintEngine);
    QRasterPaintEngineState *s = state();

    ensurePen();
    if (((qpen_style(s->lastPen) == Qt::SolidLine && s->flags.fast_pen)
         || (qpen_style(s->lastPen) == Qt::NoPen && !s->flags.antialiased))
#ifdef FLOATING_POINT_BUGGY_OR_NO_FPU
        && qMax(rect.width(), rect.height()) < 128 // integer math breakdown
#endif
        && s->matrix.type() <= QTransform::TxScale) // no shear
    {
        ensureBrush();
        const QRectF r = s->matrix.mapRect(rect);
        ProcessSpans penBlend = d->getPenFunc(r, &s->penData);
        ProcessSpans brushBlend = d->getBrushFunc(r, &s->brushData);
        const QRect brect = QRect(int(r.x()), int(r.y()),
                                  int_dim(r.x(), r.width()),
                                  int_dim(r.y(), r.height()));
        if (brect == r) {
            drawEllipse_midpoint_i(brect, d->deviceRect, penBlend, brushBlend,
                                   &s->penData, &s->brushData);
            return;
        }
    }
    QPaintEngineEx::drawEllipse(rect);
}

/*!
    \internal
*/
#ifdef Q_WS_MAC
void QRasterPaintEngine::setCGContext(CGContextRef ctx)
{
    Q_D(QRasterPaintEngine);
    d->cgContext = ctx;
}

/*!
    \internal
*/
CGContextRef QRasterPaintEngine::getCGContext() const
{
    Q_D(const QRasterPaintEngine);
    return d->cgContext;
}
#endif

#ifdef Q_WS_WIN
/*!
    \internal
*/
void QRasterPaintEngine::setDC(HDC hdc) {
    Q_D(QRasterPaintEngine);
    d->hdc = hdc;
}

/*!
    \internal
*/
HDC QRasterPaintEngine::getDC() const
{
    Q_D(const QRasterPaintEngine);
    return d->hdc;
}

/*!
    \internal
*/
void QRasterPaintEngine::releaseDC(HDC) const
{
}

#endif

/*!
    \internal
*/
QPoint QRasterPaintEngine::coordinateOffset() const
{
    return QPoint(0, 0);
}

/*!
    Draws the given color \a spans with the specified \a color. The \a
    count parameter specifies the number of spans.

    The default implementation does nothing; reimplement this function
    to draw the given color \a spans with the specified \a color. Note
    that this function \e must be reimplemented if the framebuffer is
    not memory-mapped.

    \sa drawBufferSpan()
*/
#if defined(Q_WS_QWS) && !defined(QT_NO_RASTERCALLBACKS)
void QRasterPaintEngine::drawColorSpans(const QSpan *spans, int count, uint color)
{
    Q_UNUSED(spans);
    Q_UNUSED(count);
    Q_UNUSED(color);
    qFatal("QRasterPaintEngine::drawColorSpans must be reimplemented on "
           "a non memory-mapped device");
}

/*!
    \fn void QRasterPaintEngine::drawBufferSpan(const uint *buffer, int size, int x, int y, int length, uint alpha)

    Draws the given \a buffer.

    The default implementation does nothing; reimplement this function
    to draw a buffer that contains more than one color. Note that this
    function \e must be reimplemented if the framebuffer is not
    memory-mapped.

    The \a size parameter specifies the total size of the given \a
    buffer, while the \a length parameter specifies the number of
    pixels to draw. The buffer's position is given by (\a x, \a
    y). The provided \a alpha value is added to each pixel in the
    buffer when drawing.

    \sa drawColorSpans()
*/
void QRasterPaintEngine::drawBufferSpan(const uint *buffer, int bufsize,
                                        int x, int y, int length, uint const_alpha)
{
    Q_UNUSED(buffer);
    Q_UNUSED(bufsize);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(length);
    Q_UNUSED(const_alpha);
    qFatal("QRasterPaintEngine::drawBufferSpan must be reimplemented on "
           "a non memory-mapped device");
}
#endif // Q_WS_QWS

void QRasterPaintEngine::drawBitmap(const QPointF &pos, const QPixmap &pm, QSpanData *fg)
{
    Q_ASSERT(fg);
    if (!fg->blend)
        return;
    Q_D(QRasterPaintEngine);

    const QImage image = IMAGE_FROM_PIXMAP(pm);
    Q_ASSERT(image.depth() == 1);

    const int spanCount = 256;
    QT_FT_Span spans[spanCount];
    int n = 0;

    // Boundaries
    int w = pm.width();
    int h = pm.height();
    int ymax = qMin(qRound(pos.y() + h), d->rasterBuffer->height());
    int ymin = qMax(qRound(pos.y()), 0);
    int xmax = qMin(qRound(pos.x() + w), d->rasterBuffer->width());
    int xmin = qMax(qRound(pos.x()), 0);

    int x_offset = xmin - qRound(pos.x());

    QImage::Format format = image.format();
    for (int y = ymin; y < ymax; ++y) {
        const uchar *src = image.scanLine(y - qRound(pos.y()));
        if (format == QImage::Format_MonoLSB) {
            for (int x = 0; x < xmax - xmin; ++x) {
                int src_x = x + x_offset;
                uchar pixel = src[src_x >> 3];
                if (!pixel) {
                    x += 7 - (src_x%8);
                    continue;
                }
                if (pixel & (0x1 << (src_x & 7))) {
                    spans[n].x = xmin + x;
                    spans[n].y = y;
                    spans[n].coverage = 255;
                    int len = 1;
                    while (src_x < w-1 && src[(src_x+1) >> 3] & (0x1 << ((src_x+1) & 7))) {
                        ++src_x;
                        ++len;
                    }
                    spans[n].len = ((len + spans[n].x) > xmax) ? (xmax - spans[n].x) : len;
                    x += len;
                    ++n;
                    if (n == spanCount) {
                        fg->blend(n, spans, fg);
                        n = 0;
                    }
                }
            }
        } else {
            for (int x = 0; x < xmax - xmin; ++x) {
                int src_x = x + x_offset;
                uchar pixel = src[src_x >> 3];
                if (!pixel) {
                    x += 7 - (src_x%8);
                    continue;
                }
                if (pixel & (0x80 >> (x & 7))) {
                    spans[n].x = xmin + x;
                    spans[n].y = y;
                    spans[n].coverage = 255;
                    int len = 1;
                    while (src_x < w-1 && src[(src_x+1) >> 3] & (0x80 >> ((src_x+1) & 7))) {
                        ++src_x;
                        ++len;
                    }
                    spans[n].len = ((len + spans[n].x) > xmax) ? (xmax - spans[n].x) : len;
                    x += len;
                    ++n;
                    if (n == spanCount) {
                        fg->blend(n, spans, fg);
                        n = 0;
                    }
                }
            }
        }
    }
    if (n) {
        fg->blend(n, spans, fg);
        n = 0;
    }
}

/*!
    \enum QRasterPaintEngine::ClipType
    \internal

    \value RectClip Indicates that the currently set clip is a single rectangle.
    \value ComplexClip Indicates that the currently set clip is a combination of several shapes.
*/

/*!
    \internal
    Returns the type of the clip currently set.
*/
QRasterPaintEngine::ClipType QRasterPaintEngine::clipType() const
{
    Q_D(const QRasterPaintEngine);

    const QClipData *clip = d->clip();
    if (!clip || clip->hasRectClip)
        return RectClip;
    else
        return ComplexClip;
}

/*!
    \internal
    Returns the bounding rect of the currently set clip.
*/
QRect QRasterPaintEngine::clipBoundingRect() const
{
    Q_D(const QRasterPaintEngine);

    const QClipData *clip = d->clip();

    if (!clip)
        return d->deviceRect;

    if (clip->hasRectClip)
        return clip->clipRect;

    return QRect(clip->xmin, clip->ymin, clip->xmax - clip->xmin, clip->ymax - clip->ymin);
}

static void qt_merge_clip(const QClipData *c1, const QClipData *c2, QClipData *result)
{
    Q_ASSERT(c1->clipSpanHeight == c2->clipSpanHeight && c1->clipSpanHeight == result->clipSpanHeight);

    // ### buffer overflow possible
    const int BUFFER_SIZE = 4096;
    int buffer[BUFFER_SIZE];
    int *b = buffer;
    int bsize = BUFFER_SIZE;

    QClipData::ClipLine *c1ClipLines = const_cast<QClipData *>(c1)->clipLines();
    QClipData::ClipLine *c2ClipLines = const_cast<QClipData *>(c2)->clipLines();
    result->initialize();

    for (int y = 0; y < c1->clipSpanHeight; ++y) {
        const QSpan *c1_spans = c1ClipLines[y].spans;
        int c1_count = c1ClipLines[y].count;
        const QSpan *c2_spans = c2ClipLines[y].spans;
        int c2_count = c2ClipLines[y].count;

        if (c1_count == 0 && c2_count == 0)
            continue;
        if (c1_count == 0) {
            result->appendSpans(c2_spans, c2_count);
            continue;
        } else if (c2_count == 0) {
            result->appendSpans(c1_spans, c1_count);
            continue;
        }

        // we need to merge the two

        // find required length
        int max = qMax(c1_spans[c1_count - 1].x + c1_spans[c1_count - 1].len,
                       c2_spans[c2_count - 1].x + c2_spans[c2_count - 1].len);
        if (max > bsize) {
            b = (int *)realloc(bsize == BUFFER_SIZE ? 0 : b, max*sizeof(int));
            bsize = max;
        }
        memset(buffer, 0, BUFFER_SIZE * sizeof(int));

        // Fill with old spans.
        for (int i = 0; i < c1_count; ++i) {
            const QSpan *cs = c1_spans + i;
            for (int j=cs->x; j<cs->x + cs->len; ++j)
                buffer[j] = cs->coverage;
        }

        // Fill with new spans
        for (int i = 0; i < c2_count; ++i) {
            const QSpan *cs = c2_spans + i;
            for (int j = cs->x; j < cs->x + cs->len; ++j) {
                buffer[j] += cs->coverage;
                if (buffer[j] > 255)
                    buffer[j] = 255;
            }
        }

        int x = 0;
        while (x<max) {

            // Skip to next span
            while (x < max && buffer[x] == 0) ++x;
            if (x >= max) break;

            int sx = x;
            int coverage = buffer[x];

            // Find length of span
            while (x < max && buffer[x] == coverage)
                ++x;

            result->appendSpan(sx, x - sx, y, coverage);
        }
    }
    if (b != buffer)
        free(b);
}

void QRasterPaintEnginePrivate::initializeRasterizer(QSpanData *data)
{
    Q_Q(QRasterPaintEngine);
    QRasterPaintEngineState *s = q->state();

    rasterizer->setAntialiased(s->flags.antialiased);

    QRect clipRect(deviceRect);
    ProcessSpans blend;
    // ### get from optimized rectbased QClipData

    const QClipData *c = clip();
    if (c) {
        const QRect r(QPoint(c->xmin, c->ymin),
                      QPoint(c->xmax, c->ymax));
        clipRect = clipRect.intersected(r);
        blend = data->blend;
    } else {
        blend = data->unclipped_blend;
    }

    rasterizer->setClipRect(clipRect);
    rasterizer->initialize(blend, data);
}

void QRasterPaintEnginePrivate::rasterize(QT_FT_Outline *outline,
                                          ProcessSpans callback,
                                          QSpanData *spanData, QRasterBuffer *rasterBuffer)
{
    if (!callback || !outline)
        return;

    Q_Q(QRasterPaintEngine);
    QRasterPaintEngineState *s = q->state();

    if (!s->flags.antialiased) {
        initializeRasterizer(spanData);

        const Qt::FillRule fillRule = outline->flags == QT_FT_OUTLINE_NONE
                                      ? Qt::WindingFill
                                      : Qt::OddEvenFill;

        rasterizer->rasterize(outline, fillRule);
        return;
    }

    rasterize(outline, callback, (void *)spanData, rasterBuffer);
}

void QRasterPaintEnginePrivate::rasterize(QT_FT_Outline *outline,
                                          ProcessSpans callback,
                                          void *userData, QRasterBuffer *)
{
    if (!callback || !outline)
        return;

    Q_Q(QRasterPaintEngine);
    QRasterPaintEngineState *s = q->state();

    if (!s->flags.antialiased) {
        rasterizer->setAntialiased(s->flags.antialiased);
        rasterizer->setClipRect(deviceRect);
        rasterizer->initialize(callback, userData);

        const Qt::FillRule fillRule = outline->flags == QT_FT_OUTLINE_NONE
                                      ? Qt::WindingFill
                                      : Qt::OddEvenFill;

        rasterizer->rasterize(outline, fillRule);
        return;
    }

    void *data = userData;

    QT_FT_BBox clip_box = { deviceRect.x(),
                            deviceRect.y(),
                            deviceRect.x() + deviceRect.width(),
                            deviceRect.y() + deviceRect.height() };

    QT_FT_Raster_Params rasterParams;
    rasterParams.target = 0;
    rasterParams.source = outline;
    rasterParams.flags = QT_FT_RASTER_FLAG_CLIP;
    rasterParams.gray_spans = 0;
    rasterParams.black_spans = 0;
    rasterParams.bit_test = 0;
    rasterParams.bit_set = 0;
    rasterParams.user = data;
    rasterParams.clip_box = clip_box;

    bool done = false;
    int error;

    while (!done) {

        rasterParams.flags |= (QT_FT_RASTER_FLAG_AA | QT_FT_RASTER_FLAG_DIRECT);
        rasterParams.gray_spans = callback;
        error = qt_ft_grays_raster.raster_render(*grayRaster, &rasterParams);

        // Out of memory, reallocate some more and try again...
        if (error == -6) { // -6 is Result_err_OutOfMemory
            int new_size = rasterPoolSize * 2;
            if (new_size > 1024 * 1024) {
                qWarning("QPainter: Rasterization of primitive failed");
                return;
            }

#if defined(Q_WS_WIN64)
            _aligned_free(rasterPoolBase);
#else
            free(rasterPoolBase);
#endif

            rasterPoolSize = new_size;
            rasterPoolBase =
#if defined(Q_WS_WIN64)
                // We make use of setjmp and longjmp in qgrayraster.c which requires
                // 16-byte alignment, hence we hardcode this requirement here..
                (unsigned char *) _aligned_malloc(rasterPoolSize, sizeof(void*) * 2);
#else
                (unsigned char *) malloc(rasterPoolSize);
#endif

            qt_ft_grays_raster.raster_done(*grayRaster);
            qt_ft_grays_raster.raster_new(0, grayRaster);
            qt_ft_grays_raster.raster_reset(*grayRaster, rasterPoolBase, rasterPoolSize);
        } else {
            done = true;
        }
    }
}

void QRasterPaintEnginePrivate::recalculateFastImages()
{
    Q_Q(QRasterPaintEngine);
    QRasterPaintEngineState *s = q->state();

    s->flags.fast_images = !(s->renderHints & QPainter::SmoothPixmapTransform)
                           && rasterBuffer->compositionMode == QPainter::CompositionMode_SourceOver
                           && s->matrix.type() <= QTransform::TxScale;
}



QImage QRasterBuffer::colorizeBitmap(const QImage &image, const QColor &color)
{
    Q_ASSERT(image.depth() == 1);

    QImage sourceImage = image.convertToFormat(QImage::Format_MonoLSB);
    QImage dest = QImage(sourceImage.size(), QImage::Format_ARGB32_Premultiplied);

    QRgb fg = PREMUL(color.rgba());
    QRgb bg = 0;

    int height = sourceImage.height();
    int width = sourceImage.width();
    for (int y=0; y<height; ++y) {
        uchar *source = sourceImage.scanLine(y);
        QRgb *target = reinterpret_cast<QRgb *>(dest.scanLine(y));
        for (int x=0; x < width; ++x)
            target[x] = (source[x>>3] >> (x&7)) & 1 ? fg : bg;
    }
    return dest;
}

QRasterBuffer::~QRasterBuffer()
{
}

void QRasterBuffer::init()
{
    compositionMode = QPainter::CompositionMode_SourceOver;
    monoDestinationWithClut = false;
    destColor0 = 0;
    destColor1 = 0;
}

QImage::Format QRasterBuffer::prepare(QImage *image)
{
    m_buffer = (uchar *)image->bits();
    m_width = qMin(QT_RASTER_COORD_LIMIT, image->width());
    m_height = qMin(QT_RASTER_COORD_LIMIT, image->height());
    bytes_per_pixel = image->depth()/8;
    bytes_per_line = image->bytesPerLine();

    format = image->format();
    drawHelper = qDrawHelper + format;
    if (image->depth() == 1 && image->colorTable().size() == 2) {
        monoDestinationWithClut = true;
        destColor0 = PREMUL(image->colorTable()[0]);
        destColor1 = PREMUL(image->colorTable()[1]);
    }

    return format;
}

void QRasterBuffer::resetBuffer(int val)
{
    memset(m_buffer, val, m_height*bytes_per_line);
}


#if defined(Q_WS_QWS)
void QRasterBuffer::prepare(QCustomRasterPaintDevice *device)
{
    m_buffer = reinterpret_cast<uchar*>(device->memory());
    m_width = qMin(QT_RASTER_COORD_LIMIT, device->width());
    m_height = qMin(QT_RASTER_COORD_LIMIT, device->height());
    bytes_per_pixel = device->depth() / 8;
    bytes_per_line = device->bytesPerLine();
    format = device->format();
#ifndef QT_NO_RASTERCALLBACKS
    if (!m_buffer)
        drawHelper = qDrawHelperCallback + format;
    else
#endif
        drawHelper = qDrawHelper + format;
}

class MetricAccessor : public QWidget {
public:
    int metric(PaintDeviceMetric m) {
        return QWidget::metric(m);
    }
};

int QCustomRasterPaintDevice::metric(PaintDeviceMetric m) const
{
    switch (m) {
    case PdmWidth:
        return widget->frameGeometry().width();
    case PdmHeight:
        return widget->frameGeometry().height();
    default:
        break;
    }

    return (static_cast<MetricAccessor*>(widget)->metric(m));
}

int QCustomRasterPaintDevice::bytesPerLine() const
{
    return (width() * depth() + 7) / 8;
}
#endif // Q_WS_QWS


/*!
    \class QCustomRasterPaintDevice
    \preliminary
    \ingroup qws
    \since 4.2

    \brief The QCustomRasterPaintDevice class is provided to activate
    hardware accelerated paint engines in Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    In \l{Qt for Embedded Linux}, painting is a pure software
    implementation. But starting with Qt 4.2, it is
    possible to add an accelerated graphics driver to take advantage
    of available hardware resources.

    Hardware acceleration is accomplished by creating a custom screen
    driver, accelerating the copying from memory to the screen, and
    implementing a custom paint engine accelerating the various
    painting operations. Then a custom paint device (derived from the
    QCustomRasterPaintDevice class) and a custom window surface
    (derived from QWSWindowSurface) must be implemented to make
    \l{Qt for Embedded Linux} aware of the accelerated driver.

    See the \l {Adding an Accelerated Graphics Driver to Qt for Embedded Linux}
    documentation for details.

    \sa QRasterPaintEngine, QPaintDevice
*/

/*!
    \fn QCustomRasterPaintDevice::QCustomRasterPaintDevice(QWidget *widget)

    Constructs a custom raster based paint device for the given
    top-level \a widget.
*/

/*!
    \fn int QCustomRasterPaintDevice::bytesPerLine() const

    Returns the number of bytes per line in the framebuffer. Note that
    this number might be larger than the framebuffer width.
*/

/*!
    \fn int QCustomRasterPaintDevice::devType() const
    \internal
*/

/*!
    \fn QImage::Format QCustomRasterPaintDevice::format() const

    Returns the format of the device's memory buffet.

    The default format is QImage::Format_ARGB32_Premultiplied. The
    only other valid format is QImage::Format_RGB16.
*/

/*!
    \fn void * QCustomRasterPaintDevice::memory () const

    Returns a pointer to the paint device's memory buffer, or 0 if no
    such buffer exists.
*/

/*!
    \fn int QCustomRasterPaintDevice::metric ( PaintDeviceMetric m ) const
    \reimp
*/

/*!
    \fn QSize QCustomRasterPaintDevice::size () const
    \internal
*/


QClipData::QClipData(int height)
{
    clipSpanHeight = height;
    m_clipLines = 0;

    allocated = height;
    m_spans = 0;
    xmin = xmax = ymin = ymax = 0;
    count = 0;

    enabled = true;
    hasRectClip = hasRegionClip = false;
}

QClipData::~QClipData()
{
    if (m_clipLines)
        free(m_clipLines);
    if (m_spans)
        free(m_spans);
}

void QClipData::initialize()
{
    if (m_spans)
        return;

    m_clipLines = (ClipLine *)calloc(sizeof(ClipLine), clipSpanHeight);
    m_spans = (QSpan *)malloc(clipSpanHeight*sizeof(QSpan));

    if (hasRectClip) {
        int y = 0;
        while (y < ymin) {
            m_clipLines[y].spans = 0;
            m_clipLines[y].count = 0;
            ++y;
        }

        const int len = clipRect.width();
        count = 0;
        while (y < ymax) {
            QSpan *span = m_spans + count;
            span->x = xmin;
            span->len = len;
            span->y = y;
            span->coverage = 255;
            ++count;

            m_clipLines[y].spans = span;
            m_clipLines[y].count = 1;
            ++y;
        }

        while (y < clipSpanHeight) {
            m_clipLines[y].spans = 0;
            m_clipLines[y].count = 0;
            ++y;
        }
    } else if (hasRegionClip) {

        const QVector<QRect> rects = clipRegion.rects();
        const int numRects = rects.size();

        { // resize
            const int maxSpans = (ymax - ymin) * numRects;
            if (maxSpans > allocated) {
                m_spans = (QSpan *)realloc(m_spans, maxSpans * sizeof(QSpan));
                allocated = maxSpans;
            }
        }

        int y = 0;
        int firstInBand = 0;
        while (firstInBand < numRects) {
            const int currMinY = rects.at(firstInBand).y();
            const int currMaxY = currMinY + rects.at(firstInBand).height();

            while (y < currMinY) {
                m_clipLines[y].spans = 0;
                m_clipLines[y].count = 0;
                ++y;
            }

            int lastInBand = firstInBand;
            while (lastInBand + 1 < numRects && rects.at(lastInBand+1).top() == y)
                ++lastInBand;

            while (y < currMaxY) {

                m_clipLines[y].spans = m_spans + count;
                m_clipLines[y].count = lastInBand - firstInBand + 1;

                for (int r = firstInBand; r <= lastInBand; ++r) {
                    const QRect &currRect = rects.at(r);
                    QSpan *span = m_spans + count;
                    span->x = currRect.x();
                    span->len = currRect.width();
                    span->y = y;
                    span->coverage = 255;
                    ++count;
                }
                ++y;
            }

            firstInBand = lastInBand + 1;
        }

        Q_ASSERT(count <= allocated);

        while (y < clipSpanHeight) {
            m_clipLines[y].spans = 0;
            m_clipLines[y].count = 0;
            ++y;
        }

    }
}

void QClipData::fixup()
{
    Q_ASSERT(m_spans);

    if (count == 0) {
        ymin = ymax = xmin = xmax = 0;
        return;
    }

//      qDebug("QClipData::fixup: count=%d",count);
    int y = -1;
    ymin = m_spans[0].y;
    ymax = m_spans[count-1].y + 1;
    xmin = INT_MAX;
    xmax = 0;
    for (int i = 0; i < count; ++i) {
//           qDebug() << "    " << spans[i].x << spans[i].y << spans[i].len << spans[i].coverage;
        if (m_spans[i].y != y) {
            y = m_spans[i].y;
            m_clipLines[y].spans = m_spans+i;
            m_clipLines[y].count = 0;
//              qDebug() << "        new line: y=" << y;
        }
        ++m_clipLines[y].count;
        xmin = qMin(xmin, (int)m_spans[i].x);
        xmax = qMax(xmax, (int)m_spans[i].x + m_spans[i].len);
    }
    ++xmax;
//     qDebug("xmin=%d,xmax=%d,ymin=%d,ymax=%d", xmin, xmax, ymin, ymax);
}

/*
    Convert \a rect to clip spans.
 */
void QClipData::setClipRect(const QRect &rect)
{
//    qDebug() << "setClipRect" << clipSpanHeight << count << allocated << rect;
    hasRectClip = true;
    clipRect = rect;

    xmin = rect.x();
    xmax = rect.x() + rect.width();
    ymin = qMin(rect.y(), clipSpanHeight);
    ymax = qMin(rect.y() + rect.height(), clipSpanHeight);

//    qDebug() << xmin << xmax << ymin << ymax;
}

/*
    Convert \a region to clip spans.
 */
void QClipData::setClipRegion(const QRegion &region)
{
    if (region.numRects() == 1) {
        setClipRect(region.rects().at(0));
        return;
    }

    hasRegionClip = true;
    clipRegion = region;

    { // set bounding rect
        const QRect rect = region.boundingRect();
        xmin = rect.x();
        xmax = rect.x() + rect.width();
        ymin = rect.y();
        ymax = rect.y() + rect.height();
    }
}

/*!
    \internal
    spans must be sorted on y
*/
static const QSpan *qt_intersect_spans(const QClipData *clip, int *currentClip,
                                       const QSpan *spans, const QSpan *end,
                                       QSpan **outSpans, int available)
{
    const_cast<QClipData *>(clip)->initialize();

    QSpan *out = *outSpans;

    const QSpan *clipSpans = clip->m_spans + *currentClip;
    const QSpan *clipEnd = clip->m_spans + clip->count;

    while (available && spans < end ) {
        if (clipSpans >= clipEnd) {
            spans = end;
            break;
        }
        if (clipSpans->y > spans->y) {
            ++spans;
            continue;
        }
        if (spans->y != clipSpans->y) {
            if (spans->y < clip->count && clip->m_clipLines[spans->y].spans)
                clipSpans = clip->m_clipLines[spans->y].spans;
            else
                ++clipSpans;
            continue;
        }
        Q_ASSERT(spans->y == clipSpans->y);

        int sx1 = spans->x;
        int sx2 = sx1 + spans->len;
        int cx1 = clipSpans->x;
        int cx2 = cx1 + clipSpans->len;

        if (cx1 < sx1 && cx2 < sx1) {
            ++clipSpans;
            continue;
        } else if (sx1 < cx1 && sx2 < cx1) {
            ++spans;
            continue;
        }
        int x = qMax(sx1, cx1);
        int len = qMin(sx2, cx2) - x;
        if (len) {
            out->x = qMax(sx1, cx1);
            out->len = qMin(sx2, cx2) - out->x;
            out->y = spans->y;
            out->coverage = qt_div_255(spans->coverage * clipSpans->coverage);
            ++out;
            --available;
        }
        if (sx2 < cx2) {
            ++spans;
        } else {
            ++clipSpans;
        }
    }

    *outSpans = out;
    *currentClip = clipSpans - clip->m_spans;
    return spans;
}

static void qt_span_fill_clipped(int spanCount, const QSpan *spans, void *userData)
{
//     qDebug() << "qt_span_fill_clipped" << spanCount;
    QSpanData *fillData = reinterpret_cast<QSpanData *>(userData);

    Q_ASSERT(fillData->blend && fillData->unclipped_blend);

    const int NSPANS = 256;
    QSpan cspans[NSPANS];
    int currentClip = 0;
    const QSpan *end = spans + spanCount;
    while (spans < end) {
        QSpan *clipped = cspans;
        spans = qt_intersect_spans(fillData->clip, &currentClip, spans, end, &clipped, NSPANS);
//         qDebug() << "processed " << processed << "clipped" << clipped-cspans
//                  << "span:" << cspans->x << cspans->y << cspans->len << spans->coverage;

        if (clipped - cspans)
            fillData->unclipped_blend(clipped - cspans, cspans, fillData);
    }
}

/*
    \internal
    Clip spans to \a{clip}-rectangle.
    Returns number of unclipped spans
*/
static int qt_intersect_spans(QT_FT_Span *spans, int numSpans,
                              const QRect &clip)
{
    const short minx = clip.left();
    const short miny = clip.top();
    const short maxx = clip.right();
    const short maxy = clip.bottom();

    int n = 0;
    for (int i = 0; i < numSpans; ++i) {
        if (spans[i].y > maxy)
            break;
        if (spans[i].y < miny
            || spans[i].x > maxx
            || spans[i].x + spans[i].len <= minx) {
            continue;
        }
        if (spans[i].x < minx) {
            spans[n].len = qMin(spans[i].len - (minx - spans[i].x), maxx - minx + 1);
            spans[n].x = minx;
        } else {
            spans[n].x = spans[i].x;
            spans[n].len = qMin(spans[i].len, ushort(maxx - spans[n].x + 1));
        }
        if (spans[n].len == 0)
            continue;
        spans[n].y = spans[i].y;
        spans[n].coverage = spans[i].coverage;
        ++n;
    }
    return n;
}

/*
    \internal
    Clip spans to \a{clip}-region.
    Returns number of unclipped spans
*/
static int qt_intersect_spans(QT_FT_Span *spans, int numSpans,
                              int *currSpan,
                              QT_FT_Span *outSpans, int maxOut,
                              const QRegion &clip)
{
    const QVector<QRect> rects = clip.rects();
    const int numRects = rects.size();

    int r = 0;
    short miny, minx, maxx, maxy;
    {
        const QRect &rect = rects[0];
        miny = rect.top();
        minx = rect.left();
        maxx = rect.right();
        maxy = rect.bottom();
    }

    // TODO: better mapping of currY and startRect

    int n = 0;
    int i = *currSpan;
    int currY = spans[i].y;
    while (i < numSpans) {

        if (spans[i].y != currY && r != 0) {
            currY = spans[i].y;
            r = 0;
            const QRect &rect = rects[r];
            miny = rect.top();
            minx = rect.left();
            maxx = rect.right();
            maxy = rect.bottom();
        }

        if (spans[i].y < miny) {
            ++i;
            continue;
        }

        if (spans[i].y > maxy || spans[i].x > maxx) {
            if (++r >= numRects) {
                ++i;
                continue;
            }

            const QRect &rect = rects[r];
            miny = rect.top();
            minx = rect.left();
            maxx = rect.right();
            maxy = rect.bottom();
            continue;
        }

        if (spans[i].x + spans[i].len <= minx) {
            ++i;
            continue;
        }

        outSpans[n].y = spans[i].y;
        outSpans[n].coverage = spans[i].coverage;

        if (spans[i].x < minx) {
            const ushort cutaway = minx - spans[i].x;
            outSpans[n].len = qMin(spans[i].len - cutaway, maxx - minx + 1);
            outSpans[n].x = minx;
            if (outSpans[n].len == spans[i].len - cutaway) {
                ++i;
            } else {
                // span wider than current rect
                spans[i].len -= outSpans[n].len + cutaway;
                spans[i].x = maxx + 1;
            }
        } else { // span starts inside current rect
            outSpans[n].x = spans[i].x;
            outSpans[n].len = qMin(spans[i].len,
                                   ushort(maxx - spans[i].x + 1));
            if (outSpans[n].len == spans[i].len) {
                ++i;
            } else {
                // span wider than current rect
                spans[i].len -= outSpans[n].len;
                spans[i].x = maxx + 1;
            }
        }

        if (++n >= maxOut)
            break;
    }

    *currSpan = i;
    return n;
}

static void qt_span_fill_clipRect(int count, const QSpan *spans,
                                  void *userData)
{
    QSpanData *fillData = reinterpret_cast<QSpanData *>(userData);
    Q_ASSERT(fillData->blend && fillData->unclipped_blend);

    Q_ASSERT(fillData->clip);
    Q_ASSERT(!fillData->clip->clipRect.isEmpty());

    // hw: check if this const_cast<> is safe!!!
    count = qt_intersect_spans(const_cast<QSpan*>(spans), count,
                               fillData->clip->clipRect);
    if (count > 0)
        fillData->unclipped_blend(count, spans, fillData);
}

static void qt_span_fill_clipRegion(int count, const QSpan *spans,
                                    void *userData)
{
    QSpanData *fillData = reinterpret_cast<QSpanData *>(userData);
    Q_ASSERT(fillData->blend && fillData->unclipped_blend);

    Q_ASSERT(fillData->clip);
    Q_ASSERT(!fillData->clip->clipRegion.isEmpty());

    const int NSPANS = 256;
    QSpan cspans[NSPANS];
    int currentClip = 0;
    while (currentClip < count) {
        const int unclipped = qt_intersect_spans(const_cast<QSpan*>(spans),
                                                 count, &currentClip,
                                                 &cspans[0], NSPANS,
                                                 fillData->clip->clipRegion);
        if (unclipped > 0)
            fillData->unclipped_blend(unclipped, cspans, fillData);
    }

}

static void qt_span_clip(int count, const QSpan *spans, void *userData)
{
    ClipData *clipData = reinterpret_cast<ClipData *>(userData);

//     qDebug() << " qt_span_clip: " << count << clipData->operation;
//     for (int i = 0; i < qMin(count, 10); ++i) {
//         qDebug() << "    " << spans[i].x << spans[i].y << spans[i].len << spans[i].coverage;
//     }

    switch (clipData->operation) {

    case Qt::IntersectClip:
        {
            QClipData *newClip = clipData->newClip;
            newClip->initialize();

            int currentClip = 0;
            const QSpan *end = spans + count;
            while (spans < end) {
                QSpan *newspans = newClip->m_spans + newClip->count;
                spans = qt_intersect_spans(clipData->oldClip, &currentClip, spans, end,
                                           &newspans, newClip->allocated - newClip->count);
                newClip->count = newspans - newClip->m_spans;
                if (spans < end) {
                    newClip->allocated *= 2;
                    newClip->m_spans = (QSpan *)realloc(newClip->m_spans, newClip->allocated*sizeof(QSpan));
                }
            }
        }
        break;

    case Qt::UniteClip:
    case Qt::ReplaceClip:
        clipData->newClip->appendSpans(spans, count);
        break;
    case Qt::NoClip:
        break;
    }
}

#ifndef QT_NO_DEBUG
QImage QRasterBuffer::bufferImage() const
{
    QImage image(m_width, m_height, QImage::Format_ARGB32_Premultiplied);

    for (int y = 0; y < m_height; ++y) {
        uint *span = (uint *)const_cast<QRasterBuffer *>(this)->scanLine(y);

        for (int x=0; x<m_width; ++x) {
            uint argb = span[x];
            image.setPixel(x, y, argb);
        }
    }
    return image;
}
#endif


void QRasterBuffer::flushToARGBImage(QImage *target) const
{
    int w = qMin(m_width, target->width());
    int h = qMin(m_height, target->height());

    for (int y=0; y<h; ++y) {
        uint *sourceLine = (uint *)const_cast<QRasterBuffer *>(this)->scanLine(y);
        QRgb *dest = (QRgb *) target->scanLine(y);
        for (int x=0; x<w; ++x) {
            QRgb pixel = sourceLine[x];
            int alpha = qAlpha(pixel);
            if (!alpha) {
                dest[x] = 0;
            } else {
                dest[x] = (alpha << 24)
                        | ((255*qRed(pixel)/alpha) << 16)
                        | ((255*qGreen(pixel)/alpha) << 8)
                        | ((255*qBlue(pixel)/alpha) << 0);
            }
        }
    }
}


class QGradientCache
{
    struct CacheInfo
    {
        inline CacheInfo(QGradientStops s, int op, QGradient::InterpolationMode mode) :
            stops(s), opacity(op), interpolationMode(mode) {}
        uint buffer[GRADIENT_STOPTABLE_SIZE];
        QGradientStops stops;
        int opacity;
        QGradient::InterpolationMode interpolationMode;
    };

    typedef QMultiHash<quint64, CacheInfo> QGradientColorTableHash;

public:
    inline const uint *getBuffer(const QGradient &gradient, int opacity) {
        quint64 hash_val = 0;

        QGradientStops stops = gradient.stops();
        for (int i = 0; i < stops.size() && i <= 2; i++)
            hash_val += stops[i].second.rgba();

        QGradientColorTableHash::const_iterator it = cache.constFind(hash_val);

        if (it == cache.constEnd())
            return addCacheElement(hash_val, gradient, opacity);
        else {
            do {
                const CacheInfo &cache_info = it.value();
                if (cache_info.stops == stops && cache_info.opacity == opacity && cache_info.interpolationMode == gradient.interpolationMode())
                    return cache_info.buffer;
                ++it;
            } while (it != cache.constEnd() && it.key() == hash_val);
            // an exact match for these stops and opacity was not found, create new cache
            return addCacheElement(hash_val, gradient, opacity);
        }
    }

    inline int paletteSize() const { return GRADIENT_STOPTABLE_SIZE; }
protected:
    inline int maxCacheSize() const { return 60; }
    inline void generateGradientColorTable(const QGradient& g,
                                           uint *colorTable,
                                           int size, int opacity) const;
    uint *addCacheElement(quint64 hash_val, const QGradient &gradient, int opacity) {
        if (cache.size() == maxCacheSize()) {
            int elem_to_remove = qrand() % maxCacheSize();
            cache.remove(cache.keys()[elem_to_remove]); // may remove more than 1, but OK
        }
        CacheInfo cache_entry(gradient.stops(), opacity, gradient.interpolationMode());
        generateGradientColorTable(gradient, cache_entry.buffer, paletteSize(), opacity);
        return cache.insert(hash_val, cache_entry).value().buffer;
    }

    QGradientColorTableHash cache;
};

void QGradientCache::generateGradientColorTable(const QGradient& gradient, uint *colorTable, int size, int opacity) const
{
    QGradientStops stops = gradient.stops();
    int stopCount = stops.count();
    Q_ASSERT(stopCount > 0);

    bool colorInterpolation = (gradient.interpolationMode() == QGradient::ColorInterpolation);

    uint current_color = ARGB_COMBINE_ALPHA(stops[0].second.rgba(), opacity);
    if (stopCount == 1) {
        current_color = PREMUL(current_color);
        for (int i = 0; i < size; ++i)
            colorTable[i] = current_color;
        return;
    }

    // The position where the gradient begins and ends
    qreal begin_pos = stops[0].first;
    qreal end_pos = stops[stopCount-1].first;

    int pos = 0; // The position in the color table.
    uint next_color;

    qreal incr = 1 / qreal(size); // the double increment.
    qreal dpos = 1.5 * incr; // current position in gradient stop list (0 to 1)

     // Up to first point
    colorTable[pos++] = PREMUL(current_color);
    while (dpos <= begin_pos) {
        colorTable[pos] = colorTable[pos - 1];
        ++pos;
        dpos += incr;
    }

    int current_stop = 0; // We always interpolate between current and current + 1.

    qreal t; // position between current left and right stops
    qreal t_delta; // the t increment per entry in the color table

    if (dpos < end_pos) {
        // Gradient area
        while (dpos > stops[current_stop+1].first)
            ++current_stop;

        if (current_stop != 0)
            current_color = ARGB_COMBINE_ALPHA(stops[current_stop].second.rgba(), opacity);
        next_color = ARGB_COMBINE_ALPHA(stops[current_stop+1].second.rgba(), opacity);

        if (colorInterpolation) {
            current_color = PREMUL(current_color);
            next_color = PREMUL(next_color);
        }

        qreal diff = stops[current_stop+1].first - stops[current_stop].first;
        qreal c = (diff == 0) ? qreal(0) : 256 / diff;
        t = (dpos - stops[current_stop].first) * c;
        t_delta = incr * c;

        while (true) {
            Q_ASSERT(current_stop < stopCount);

            int dist = qRound(t);
            int idist = 256 - dist;

            if (colorInterpolation)
                colorTable[pos] = INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist);
            else
                colorTable[pos] = PREMUL(INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist));

            ++pos;
            dpos += incr;

            if (dpos >= end_pos)
                break;

            t += t_delta;

            int skip = 0;
            while (dpos > stops[current_stop+skip+1].first)
                ++skip;

            if (skip != 0) {
                current_stop += skip;
                if (skip == 1)
                    current_color = next_color;
                else
                    current_color = ARGB_COMBINE_ALPHA(stops[current_stop].second.rgba(), opacity);
                next_color = ARGB_COMBINE_ALPHA(stops[current_stop+1].second.rgba(), opacity);

                if (colorInterpolation) {
                    if (skip != 1)
                        current_color = PREMUL(current_color);
                    next_color = PREMUL(next_color);
                }

                qreal diff = stops[current_stop+1].first - stops[current_stop].first;
                qreal c = (diff == 0) ? qreal(0) : 256 / diff;
                t = (dpos - stops[current_stop].first) * c;
                t_delta = incr * c;
            }
        }
    }

    // After last point
    current_color = PREMUL(ARGB_COMBINE_ALPHA(stops[stopCount - 1].second.rgba(), opacity));
    while (pos < size - 1) {
        colorTable[pos] = current_color;
        ++pos;
    }

    // Make sure the last color stop is represented at the end of the table
    colorTable[size - 1] = current_color;
}

Q_GLOBAL_STATIC(QGradientCache, qt_gradient_cache)


void QSpanData::init(QRasterBuffer *rb, const QRasterPaintEngine *pe)
{
    rasterBuffer = rb;
#ifdef Q_WS_QWS
    rasterEngine = const_cast<QRasterPaintEngine *>(pe);
#endif
    type = None;
    txop = 0;
    bilinear = false;
    m11 = m22 = m33 = 1.;
    m12 = m13 = m21 = m23 = dx = dy = 0.0;
    clip = pe ? pe->d_func()->clip() : 0;
}

extern QImage qt_imageForBrush(int brushStyle, bool invert);

void QSpanData::setup(const QBrush &brush, int alpha)
{
    Qt::BrushStyle brushStyle = qbrush_style(brush);
    switch (brushStyle) {
    case Qt::SolidPattern: {
        type = Solid;
        QColor c = qbrush_color(brush);
        solid.color = PREMUL(ARGB_COMBINE_ALPHA(c.rgba(), alpha));
        break;
    }

    case Qt::LinearGradientPattern:
        {
            type = LinearGradient;
            const QLinearGradient *g = static_cast<const QLinearGradient *>(brush.gradient());
            gradient.alphaColor = !brush.isOpaque() || alpha != 256;
            gradient.colorTable = const_cast<uint*>(qt_gradient_cache()->getBuffer(*g, alpha));
            gradient.spread = g->spread();

            QLinearGradientData &linearData = gradient.linear;

            linearData.origin.x = g->start().x();
            linearData.origin.y = g->start().y();
            linearData.end.x = g->finalStop().x();
            linearData.end.y = g->finalStop().y();
            break;
        }

    case Qt::RadialGradientPattern:
        {
            type = RadialGradient;
            const QRadialGradient *g = static_cast<const QRadialGradient *>(brush.gradient());
            gradient.alphaColor = !brush.isOpaque() || alpha != 256;
            gradient.colorTable = const_cast<uint*>(qt_gradient_cache()->getBuffer(*g, alpha));
            gradient.spread = g->spread();

            QRadialGradientData &radialData = gradient.radial;

            QPointF center = g->center();
            radialData.center.x = center.x();
            radialData.center.y = center.y();
            QPointF focal = g->focalPoint();
            radialData.focal.x = focal.x();
            radialData.focal.y = focal.y();
            radialData.radius = g->radius();
        }
        break;

    case Qt::ConicalGradientPattern:
        {
            type = ConicalGradient;
            const QConicalGradient *g = static_cast<const QConicalGradient *>(brush.gradient());
            gradient.alphaColor = !brush.isOpaque() || alpha != 256;
            gradient.colorTable = const_cast<uint*>(qt_gradient_cache()->getBuffer(*g, alpha));
            gradient.spread = QGradient::RepeatSpread;

            QConicalGradientData &conicalData = gradient.conical;

            QPointF center = g->center();
            conicalData.center.x = center.x();
            conicalData.center.y = center.y();
            conicalData.angle = g->angle() * 2 * Q_PI / 360.0;
        }
        break;

    case Qt::Dense1Pattern:
    case Qt::Dense2Pattern:
    case Qt::Dense3Pattern:
    case Qt::Dense4Pattern:
    case Qt::Dense5Pattern:
    case Qt::Dense6Pattern:
    case Qt::Dense7Pattern:
    case Qt::HorPattern:
    case Qt::VerPattern:
    case Qt::CrossPattern:
    case Qt::BDiagPattern:
    case Qt::FDiagPattern:
    case Qt::DiagCrossPattern:
        type = Texture;
        if (!tempImage)
            tempImage = new QImage();
        *tempImage = rasterBuffer->colorizeBitmap(qt_imageForBrush(brushStyle, true), brush.color());
        initTexture(tempImage, alpha, QTextureData::Tiled);
        break;
    case Qt::TexturePattern:
        type = Texture;
        if (!tempImage)
            tempImage = new QImage();

        if (qHasPixmapTexture(brush) && brush.texture().isQBitmap())
            *tempImage = rasterBuffer->colorizeBitmap(brush.textureImage(), brush.color());
        else
            *tempImage = brush.textureImage();
        initTexture(tempImage, alpha, QTextureData::Tiled, tempImage->rect());
        break;

    case Qt::NoBrush:
    default:
        type = None;
        break;
    }
    adjustSpanMethods();
}

void QSpanData::adjustSpanMethods()
{
    bitmapBlit = 0;
    alphamapBlit = 0;
    alphaRGBBlit = 0;

    fillRect = 0;

    switch(type) {
    case None:
        unclipped_blend = 0;
        break;
    case Solid:
        unclipped_blend = rasterBuffer->drawHelper->blendColor;
        bitmapBlit = rasterBuffer->drawHelper->bitmapBlit;
        alphamapBlit = rasterBuffer->drawHelper->alphamapBlit;
        alphaRGBBlit = rasterBuffer->drawHelper->alphaRGBBlit;
        fillRect = rasterBuffer->drawHelper->fillRect;
        break;
    case LinearGradient:
    case RadialGradient:
    case ConicalGradient:
        unclipped_blend = rasterBuffer->drawHelper->blendGradient;
        break;
    case Texture:
#ifdef Q_WS_QWS
#ifndef QT_NO_RASTERCALLBACKS
        if (!rasterBuffer->buffer())
            unclipped_blend = qBlendTextureCallback;
        else
#endif
            unclipped_blend = qBlendTexture;
#else
        unclipped_blend = qBlendTexture;
#endif
        break;
    }
    // setup clipping
    if (!unclipped_blend) {
        blend = 0;
    } else if (!clip) {
        blend = unclipped_blend;
    } else if (clip->hasRectClip) {
        blend = clip->clipRect.isEmpty() ? 0 : qt_span_fill_clipRect;
    } else if (clip->hasRegionClip) {
        blend = clip->clipRegion.isEmpty() ? 0 : qt_span_fill_clipRegion;
    } else {
        blend = qt_span_fill_clipped;
    }
}

void QSpanData::setupMatrix(const QTransform &matrix, int bilin)
{
    QTransform inv = matrix.inverted();
    m11 = inv.m11();
    m12 = inv.m12();
    m13 = inv.m13();
    m21 = inv.m21();
    m22 = inv.m22();
    m23 = inv.m23();
    m33 = inv.m33();
    dx = inv.dx();
    dy = inv.dy();
    txop = inv.type();
    bilinear = bilin;

    const bool affine = !m13 && !m23;
    fast_matrix = affine
        && m11 * m11 + m21 * m21 < 1e4
        && m12 * m12 + m22 * m22 < 1e4
        && qAbs(dx) < 1e4
        && qAbs(dy) < 1e4;

    adjustSpanMethods();
}

extern const QVector<QRgb> *qt_image_colortable(const QImage &image);

void QSpanData::initTexture(const QImage *image, int alpha, QTextureData::Type _type, const QRect &sourceRect)
{
    const QImageData *d = const_cast<QImage *>(image)->data_ptr();
    if (!d || d->height == 0) {
        texture.imageData = 0;
        texture.width = 0;
        texture.height = 0;
        texture.x1 = 0;
        texture.y1 = 0;
        texture.x2 = 0;
        texture.y2 = 0;
        texture.bytesPerLine = 0;
        texture.format = QImage::Format_Invalid;
        texture.colorTable = 0;
        texture.hasAlpha = alpha != 256;
    } else {
        texture.imageData = d->data;
        texture.width = d->width;
        texture.height = d->height;

        if (sourceRect.isNull()) {
            texture.x1 = 0;
            texture.y1 = 0;
            texture.x2 = texture.width;
            texture.y2 = texture.height;
        } else {
            texture.x1 = sourceRect.x();
            texture.y1 = sourceRect.y();
            texture.x2 = qMin(texture.x1 + sourceRect.width(), d->width);
            texture.y2 = qMin(texture.y1 + sourceRect.height(), d->height);
        }

        texture.bytesPerLine = d->bytes_per_line;

        texture.format = d->format;
        texture.colorTable = (d->format <= QImage::Format_Indexed8 && !d->colortable.isEmpty()) ? &d->colortable : 0;
        texture.hasAlpha = image->hasAlphaChannel() || alpha != 256;
    }
    texture.const_alpha = alpha;
    texture.type = _type;

    adjustSpanMethods();
}

#ifdef Q_WS_WIN


#endif


/*!
    \internal

    Draws a line using the floating point midpoint algorithm. The line
    \a line is already in device coords at this point.
*/

static void drawLine_midpoint_i(int x1, int y1, int x2, int y2, ProcessSpans span_func, QSpanData *data,
                                LineDrawMode style, const QIntRect &devRect)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << "   - drawLine_midpoint_i" << QLine(QPoint(x1, y1), QPoint(x2, y2));
#endif

    int x, y;
    int dx, dy, d, incrE, incrNE;

    dx = x2 - x1;
    dy = y2 - y1;

    const int NSPANS = 256;
    QT_FT_Span spans[NSPANS];
    int current = 0;
    bool ordered = true;

    if (dy == 0) {
        // specialcase horizontal lines
        if (y1 >= devRect.y1 && y1 < devRect.y2) {
            int start = qMax(devRect.x1, qMin(x1, x2));
            int stop = qMax(x1, x2) + 1;
            int stop_clipped = qMin(devRect.x2, stop);
            int len = stop_clipped - start;
            if (style == LineDrawNormal && stop == stop_clipped)
                len--;
            if (len > 0) {
                spans[0].x = ushort(start);
                spans[0].len = ushort(len);
                spans[0].y = y1;
                spans[0].coverage = 255;
                span_func(1, spans, data);
            }
        }
        return;
    } else if (dx == 0) {
        // specialcase vertical lines
        if (x1 >= devRect.x1 && x1 < devRect.x2) {
            int start = qMax(devRect.y1, qMin(y1, y2));
            int stop = qMax(y1, y2) + 1;
            int stop_clipped = qMin(devRect.y2, stop);
            int len = stop_clipped - start;
            if (style == LineDrawNormal && stop == stop_clipped)
                len--;
            // hw: create spans directly instead to possibly avoid clipping
            if (len > 0)
                fillRect_normalized(QRect(x1, start, 1, len).normalized(), data, 0);
        }
        return;
    }


    if (qAbs(dx) >= qAbs(dy)) {       /* if x is the major axis: */

        if (x2 < x1) {  /* if coordinates are out of order */
            qt_swap_int(x1, x2);
            dx = -dx;

            qt_swap_int(y1, y2);
            dy = -dy;
        }

        if (style == LineDrawNormal)
            --x2;

        // In the loops below we increment before call the span function so
        // we need to stop one pixel before
        x2 = qMin(x2, devRect.x2 - 1);

        // completely clipped, so abort
        if (x2 <= x1) {
            return;
        }

        int x = x1;
        int y = y1;

        if (y2 <= y1)
            ordered = false;

        {
            const int index = (ordered ? current : NSPANS - 1 - current);
            spans[index].coverage = 255;
            spans[index].x = x;
            spans[index].y = y;

            if (x >= devRect.x1 && y >= devRect.y1 && y < devRect.y2)
                spans[index].len = 1;
            else
                spans[index].len = 0;
        }

        if (y2 > y1) { // 315 -> 360 and 135 -> 180 (unit circle degrees)
            y2 = qMin(y2, devRect.y2 - 1);

            incrE = dy * 2;
            d = incrE - dx;
            incrNE = (dy - dx) * 2;

            if (y > y2)
                goto flush_and_return;

            while (x < x2) {
                ++x;
                if (d > 0) {
                    if (spans[current].len > 0)
                        ++current;
                    if (current == NSPANS) {
                        span_func(NSPANS, spans, data);
                        current = 0;
                    }

                    ++y;
                    d += incrNE;
                    if (y > y2)
                        goto flush_and_return;

                    spans[current].len = 0;
                    spans[current].coverage = 255;
                    spans[current].x = x;
                    spans[current].y = y;
                } else {
                    d += incrE;
                    if (x == devRect.x1)
                        spans[current].x = devRect.x1;
                }

                if (x < devRect.x1 || y < devRect.y1)
                    continue;

                Q_ASSERT(x<devRect.x2);
                Q_ASSERT(y<devRect.y2);
                Q_ASSERT(spans[current].y == y);
                spans[current].len++;
            }
            if (spans[current].len > 0) {
                ++current;
            }
        } else {  // 0-45 and 180->225 (unit circle degrees)

            y1 = qMin(y1, devRect.y2 - 1);

            incrE = dy * 2;
            d = incrE + dx;
            incrNE = (dy + dx) * 2;

            if (y < devRect.y1)
                goto flush_and_return;

            while (x < x2) {
                ++x;
                if (d < 0) {
                    if (spans[NSPANS - 1 - current].len > 0)
                        ++current;
                    if (current == NSPANS) {
                        span_func(NSPANS, spans, data);
                        current = 0;
                    }

                    --y;
                    d += incrNE;
                    if (y < devRect.y1)
                        goto flush_and_return;

                    const int index = NSPANS - 1 - current;
                    spans[index].len = 0;
                    spans[index].coverage = 255;
                    spans[index].x = x;
                    spans[index].y = y;
                } else {
                    d += incrE;
                    if (x == devRect.x1)
                        spans[NSPANS - 1 - current].x = devRect.x1;
                }

                if (x < devRect.x1 || y > y1)
                    continue;

                Q_ASSERT(x<devRect.x2 && y<devRect.y2);
                Q_ASSERT(spans[NSPANS - 1 - current].y == y);
                spans[NSPANS - 1 - current].len++;
            }
            if (spans[NSPANS - 1 - current].len > 0) {
                ++current;
            }
        }

    } else {

        // if y is the major axis:

        if (y2 < y1) {      /* if coordinates are out of order */
            qt_swap_int(y1, y2);
            dy = -dy;

            qt_swap_int(x1, x2);
            dx = -dx;
        }

        if (style == LineDrawNormal)
            --y2;

        // In the loops below we increment before call the span function so
        // we need to stop one pixel before
        y2 = qMin(y2, devRect.y2 - 1);

        // completely clipped, so abort
        if (y2 <= y1) {
            return;
        }

        x = x1;
        y = y1;

        if (x>=devRect.x1 && y>=devRect.y1 && x < devRect.x2) {
            Q_ASSERT(x >= devRect.x1 && y >= devRect.y1 && x < devRect.x2 && y < devRect.y2);
            if (current == NSPANS) {
                span_func(NSPANS, spans, data);
                current = 0;
            }
            spans[current].len = 1;
            spans[current].coverage = 255;
            spans[current].x = x;
            spans[current].y = y;
            ++current;
        }

        if (x2 > x1) { // 90 -> 135 and 270 -> 315 (unit circle degrees)
            x2 = qMin(x2, devRect.x2 - 1);
            incrE = dx * 2;
            d = incrE - dy;
            incrNE = (dx - dy) * 2;

            if (x > x2)
                goto flush_and_return;

            while (y < y2) {
                if (d > 0) {
                    ++x;
                    d += incrNE;
                    if (x > x2)
                        goto flush_and_return;
                } else {
                    d += incrE;
                }
                ++y;
                if (x < devRect.x1 || y < devRect.y1)
                    continue;
                Q_ASSERT(x<devRect.x2 && y<devRect.y2);
                if (current == NSPANS) {
                    span_func(NSPANS, spans, data);
                    current = 0;
                }
                spans[current].len = 1;
                spans[current].coverage = 255;
                spans[current].x = x;
                spans[current].y = y;
                ++current;
            }
        } else { // 45 -> 90 and 225 -> 270 (unit circle degrees)
            x1 = qMin(x1, devRect.x2 - 1);
            incrE = dx * 2;
            d = incrE + dy;
            incrNE = (dx + dy) * 2;

            if (x < devRect.x1)
                goto flush_and_return;

            while (y < y2) {
                if (d < 0) {
                    --x;
                    d += incrNE;
                    if (x < devRect.x1)
                        goto flush_and_return;
                } else {
                    d += incrE;
                }
                ++y;
                if (y < devRect.y1 || x > x1)
                    continue;
                Q_ASSERT(x>=devRect.x1 && x<devRect.x2 && y>=devRect.y1 && y<devRect.y2);
                if (current == NSPANS) {
                    span_func(NSPANS, spans, data);
                    current = 0;
                }
                spans[current].len = 1;
                spans[current].coverage = 255;
                spans[current].x = x;
                spans[current].y = y;
                ++current;
            }
        }
    }
flush_and_return:
    if (current > 0)
        span_func(current, ordered ? spans : spans + (NSPANS - current), data);
}

static void offset_pattern(int offset, bool *inDash, int *dashIndex, int *currentOffset, const QVarLengthArray<qreal> &pattern)
{
    while (offset--) {
        if (--*currentOffset == 0) {
            *inDash = !*inDash;
            *dashIndex = ((*dashIndex + 1) % pattern.size());
            *currentOffset = int(pattern[*dashIndex]);
        }
    }
}

static void drawLine_midpoint_dashed_i(int x1, int y1, int x2, int y2,
                                       QPen *pen,
                                       ProcessSpans span_func, QSpanData *data,
                                       LineDrawMode style, const QIntRect &devRect,
                                       int *patternOffset)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << "   - drawLine_midpoint_dashed_i" << x1 << y1 << x2 << y2 << *patternOffset;
#endif

    int x, y;
    int dx, dy, d, incrE, incrNE;

    dx = x2 - x1;
    dy = y2 - y1;

    Q_ASSERT(*patternOffset >= 0);

    const QVector<qreal> penPattern = pen->dashPattern();
    QVarLengthArray<qreal> pattern(penPattern.size());

    int patternLength = 0;
    for (int i = 0; i < penPattern.size(); ++i)
        patternLength += qMax<qreal>(1.0, (penPattern.at(i)));

    // pattern must be reversed if coordinates are out of order
    int reverseLength = -1;
    if (dy == 0 && x1 > x2)
        reverseLength = x1 - x2;
    else if (dx == 0 && y1 > y2)
        reverseLength = y1 - y2;
    else if (qAbs(dx) >= qAbs(dy) && x2 < x1) // x major axis
        reverseLength = qAbs(dx);
    else if (qAbs(dy) >= qAbs(dx) && y2 < y1) // y major axis
        reverseLength = qAbs(dy);

    const bool reversed = (reverseLength > -1);
    if (reversed) { // reverse pattern
        for (int i = 0; i < penPattern.size(); ++i)
            pattern[penPattern.size() - 1 - i] = qMax<qreal>(1.0, penPattern.at(i));

        *patternOffset = (patternLength - 1 - *patternOffset);
        *patternOffset += patternLength - (reverseLength % patternLength);
        *patternOffset = *patternOffset % patternLength;
    } else {
        for (int i = 0; i < penPattern.size(); ++i)
            pattern[i] = qMax<qreal>(1.0, penPattern.at(i));
    }

    int dashIndex = 0;
    bool inDash = !reversed;
    int currPattern = int(pattern[dashIndex]);

    // adjust pattern for offset
    offset_pattern(*patternOffset, &inDash, &dashIndex, &currPattern, pattern);

    const int NSPANS = 256;
    QT_FT_Span spans[NSPANS];
    int current = 0;
    bool ordered = true;

    if (dy == 0) {
        // specialcase horizontal lines
        if (y1 >= devRect.y1 && y1 < devRect.y2) {
            int start_unclipped = qMin(x1, x2);
            int start = qMax(devRect.x1, start_unclipped);
            int stop = qMax(x1, x2) + 1;
            int stop_clipped = qMin(devRect.x2, stop);
            int len = stop_clipped - start;
            if (style == LineDrawNormal && stop == stop_clipped)
                len--;

            // adjust pattern for starting offset
            offset_pattern(start - start_unclipped, &inDash, &dashIndex, &currPattern, pattern);

            if (len > 0) {
                int x = start;
                while (x < stop_clipped) {
                    if (current == NSPANS) {
                        span_func(NSPANS, spans, data);
                        current = 0;
                    }
                    const int dash = qMin(currPattern, stop_clipped - x);
                    if (inDash) {
                        spans[current].x = ushort(x);
                        spans[current].len = ushort(dash);
                        spans[current].y = y1;
                        spans[current].coverage = 255;
                        ++current;
                    }
                    if (dash < currPattern) {
                        currPattern -= dash;
                    } else {
                        dashIndex = (dashIndex + 1) % pattern.size();
                        currPattern = int(pattern[dashIndex]);
                        inDash = !inDash;
                    }
                    x += dash;
                }
            }
        }
        goto flush_and_return;
    } else if (dx == 0) {
        if (x1 >= devRect.x1 && x1 < devRect.x2) {
            int start_unclipped = qMin(y1, y2);
            int start = qMax(devRect.y1, start_unclipped);
            int stop = qMax(y1, y2) + 1;
            int stop_clipped = qMin(devRect.y2, stop);
            if (style == LineDrawNormal && stop == stop_clipped)
                --stop;
            else
                stop = stop_clipped;

            // adjust pattern for starting offset
            offset_pattern(start - start_unclipped, &inDash, &dashIndex, &currPattern, pattern);

            // loop over dashes
            int y = start;
            while (y < stop) {
                const int dash = qMin(currPattern, stop - y);
                if (inDash) {
                    for (int i = 0; i < dash; ++i) {
                        if (current == NSPANS) {
                            span_func(NSPANS, spans, data);
                            current = 0;
                        }
                        spans[current].x = x1;
                        spans[current].len = 1;
                        spans[current].coverage = 255;
                        spans[current].y = ushort(y + i);
                        ++current;
                    }
                }
                if (dash < currPattern) {
                    currPattern -= dash;
                } else {
                    dashIndex = (dashIndex + 1) % pattern.size();
                    currPattern = int(pattern[dashIndex]);
                    inDash = !inDash;
                }
                y += dash;
            }
        }
        goto flush_and_return;
    }

    if (qAbs(dx) >= qAbs(dy)) {       /* if x is the major axis: */

        if (x2 < x1) {  /* if coordinates are out of order */
            qt_swap_int(x1, x2);
            dx = -dx;

            qt_swap_int(y1, y2);
            dy = -dy;
        }

        if (style == LineDrawNormal)
            --x2;

        // In the loops below we increment before call the span function so
        // we need to stop one pixel before
        x2 = qMin(x2, devRect.x2 - 1);

        // completely clipped, so abort
        if (x2 <= x1)
            goto flush_and_return;

        int x = x1;
        int y = y1;

        if (x >= devRect.x1 && y >= devRect.y1 && y < devRect.y2) {
            Q_ASSERT(x < devRect.x2);
            if (inDash) {
                if (current == NSPANS) {
                    span_func(NSPANS, spans, data);
                    current = 0;
                }
                spans[current].len = 1;
                spans[current].coverage = 255;
                spans[current].x = x;
                spans[current].y = y;
                ++current;
            }
            if (--currPattern <= 0) {
                inDash = !inDash;
                dashIndex = (dashIndex + 1) % pattern.size();
                currPattern = int(pattern[dashIndex]);
            }
        }

        if (y2 > y1) { // 315 -> 360 and 135 -> 180 (unit circle degrees)
            y2 = qMin(y2, devRect.y2 - 1);

            incrE = dy * 2;
            d = incrE - dx;
            incrNE = (dy - dx) * 2;

            if (y > y2)
                goto flush_and_return;

            while (x < x2) {
                if (d > 0) {
                    ++y;
                    d += incrNE;
                    if (y > y2)
                        goto flush_and_return;
                } else {
                    d += incrE;
                }
                ++x;

                const bool skip = x < devRect.x1 || y < devRect.y1;
                Q_ASSERT(skip || (x < devRect.x2 && y < devRect.y2));
                if (inDash && !skip) {
                    if (current == NSPANS) {
                        span_func(NSPANS, spans, data);
                        current = 0;
                    }
                    spans[current].len = 1;
                    spans[current].coverage = 255;
                    spans[current].x = x;
                    spans[current].y = y;
                    ++current;
                }
                if (--currPattern <= 0) {
                    inDash = !inDash;
                    dashIndex = (dashIndex + 1) % pattern.size();
                    currPattern = int(pattern[dashIndex]);
                }
            }
        } else {  // 0-45 and 180->225 (unit circle degrees)
            y1 = qMin(y1, devRect.y2 - 1);

            incrE = dy * 2;
            d = incrE + dx;
            incrNE = (dy + dx) * 2;

            if (y < devRect.y1)
                goto flush_and_return;

            while (x < x2) {
                if (d < 0) {
                    if (current > 0) {
                        span_func(current, spans, data);
                        current = 0;
                    }

                    --y;
                    d += incrNE;
                    if (y < devRect.y1)
                        goto flush_and_return;
                } else {
                    d += incrE;
                }
                ++x;

                const bool skip = x < devRect.x1 || y > y1;
                Q_ASSERT(skip || (x < devRect.x2 && y < devRect.y2));
                if (inDash && !skip) {
                    if (current == NSPANS) {
                        span_func(NSPANS, spans, data);
                        current = 0;
                    }
                    spans[current].len = 1;
                    spans[current].coverage = 255;
                    spans[current].x = x;
                    spans[current].y = y;
                    ++current;
                }
                if (--currPattern <= 0) {
                    inDash = !inDash;
                    dashIndex = (dashIndex + 1) % pattern.size();
                    currPattern = int(pattern[dashIndex]);
                }
            }
        }
    } else {

        // if y is the major axis:

        if (y2 < y1) {      /* if coordinates are out of order */
            qt_swap_int(y1, y2);
            dy = -dy;

            qt_swap_int(x1, x2);
            dx = -dx;
        }

        if (style == LineDrawNormal)
            --y2;

        // In the loops below we increment before call the span function so
        // we need to stop one pixel before
        y2 = qMin(y2, devRect.y2 - 1);

        // completely clipped, so abort
        if (y2 <= y1)
            goto flush_and_return;

        x = x1;
        y = y1;

        if (x>=devRect.x1 && y>=devRect.y1 && x < devRect.x2) {
            Q_ASSERT(x < devRect.x2);
            if (inDash) {
                if (current == NSPANS) {
                    span_func(NSPANS, spans, data);
                    current = 0;
                }
                spans[current].len = 1;
                spans[current].coverage = 255;
                spans[current].x = x;
                spans[current].y = y;
                ++current;
            }
            if (--currPattern <= 0) {
                inDash = !inDash;
                dashIndex = (dashIndex + 1) % pattern.size();
                currPattern = int(pattern[dashIndex]);
            }
        }

        if (x2 > x1) { // 90 -> 135 and 270 -> 315 (unit circle degrees)
            x2 = qMin(x2, devRect.x2 - 1);
            incrE = dx * 2;
            d = incrE - dy;
            incrNE = (dx - dy) * 2;

            if (x > x2)
                goto flush_and_return;

            while (y < y2) {
                if (d > 0) {
                    ++x;
                    d += incrNE;
                    if (x > x2)
                        goto flush_and_return;
                } else {
                    d += incrE;
                }
                ++y;
                const bool skip = x < devRect.x1 || y < devRect.y1;
                Q_ASSERT(skip || (x < devRect.x2 && y < devRect.y2));
                if (inDash && !skip) {
                    if (current == NSPANS) {
                        span_func(NSPANS, spans, data);
                        current = 0;
                    }
                    spans[current].len = 1;
                    spans[current].coverage = 255;
                    spans[current].x = x;
                    spans[current].y = y;
                    ++current;
                }
                if (--currPattern <= 0) {
                    inDash = !inDash;
                    dashIndex = (dashIndex + 1) % pattern.size();
                    currPattern = int(pattern[dashIndex]);
                }
            }
        } else { // 45 -> 90 and 225 -> 270 (unit circle degrees)
            x1 = qMin(x1, devRect.x2 - 1);
            incrE = dx * 2;
            d = incrE + dy;
            incrNE = (dx + dy) * 2;

            if (x < devRect.x1)
                goto flush_and_return;

            while (y < y2) {
                if (d < 0) {
                    --x;
                    d += incrNE;
                    if (x < devRect.x1)
                        goto flush_and_return;
                } else {
                    d += incrE;
                }
                ++y;
                const bool skip = y < devRect.y1 || x > x1;
                Q_ASSERT(skip || (x >= devRect.x1 && x < devRect.x2 && y < devRect.y2));
                if (inDash && !skip) {
                    if (current == NSPANS) {
                        span_func(NSPANS, spans, data);
                        current = 0;
                    }
                    spans[current].len = 1;
                    spans[current].coverage = 255;
                    spans[current].x = x;
                    spans[current].y = y;
                    ++current;
                }
                if (--currPattern <= 0) {
                    inDash = !inDash;
                    dashIndex = (dashIndex + 1) % pattern.size();
                    currPattern = int(pattern[dashIndex]);
                }
            }
        }
    }
flush_and_return:
    if (current > 0)
        span_func(current, ordered ? spans : spans + (NSPANS - current), data);

    // adjust offset
    if (reversed) {
        *patternOffset = (patternLength - 1 - *patternOffset);
    } else {
        *patternOffset = 0;
        for (int i = 0; i <= dashIndex; ++i)
            *patternOffset += int(pattern[i]);
        *patternOffset += patternLength - currPattern - 1;
        *patternOffset = (*patternOffset % patternLength);
    }
}

/*!
    \internal
    \a x and \a y is relative to the midpoint of \a rect.
*/
static inline void drawEllipsePoints(int x, int y, int length,
                                     const QRect &rect,
                                     const QRect &clip,
                                     ProcessSpans pen_func, ProcessSpans brush_func,
                                     QSpanData *pen_data, QSpanData *brush_data)
{
    if (length == 0)
        return;

    QT_FT_Span outline[4];
    const int midx = rect.x() + (rect.width() + 1) / 2;
    const int midy = rect.y() + (rect.height() + 1) / 2;

    x = x + midx;
    y = midy - y;

    // topleft
    outline[0].x = midx + (midx - x) - (length - 1) - (rect.width() & 0x1);
    outline[0].len = qMin(length, x - outline[0].x);
    outline[0].y = y;
    outline[0].coverage = 255;

    // topright
    outline[1].x = x;
    outline[1].len = length;
    outline[1].y = y;
    outline[1].coverage = 255;

    // bottomleft
    outline[2].x = outline[0].x;
    outline[2].len = outline[0].len;
    outline[2].y = midy + (midy - y) - (rect.height() & 0x1);
    outline[2].coverage = 255;

    // bottomright
    outline[3].x = x;
    outline[3].len = length;
    outline[3].y = outline[2].y;
    outline[3].coverage = 255;

    if (brush_func && outline[0].x + outline[0].len < outline[1].x) {
        QT_FT_Span fill[2];

        // top fill
        fill[0].x = outline[0].x + outline[0].len - 1;
        fill[0].len = qMax(0, outline[1].x - fill[0].x);
        fill[0].y = outline[1].y;
        fill[0].coverage = 255;

        // bottom fill
        fill[1].x = outline[2].x + outline[2].len - 1;
        fill[1].len = qMax(0, outline[3].x - fill[1].x);
        fill[1].y = outline[3].y;
        fill[1].coverage = 255;

        int n = (fill[0].y >= fill[1].y ? 1 : 2);
        n = qt_intersect_spans(fill, n, clip);
        if (n > 0)
            brush_func(n, fill, brush_data);
    }
    if (pen_func) {
        int n = (outline[1].y >= outline[2].y ? 2 : 4);
        n = qt_intersect_spans(outline, n, clip);
        if (n > 0)
            pen_func(n, outline, pen_data);
    }
}

/*!
    \internal
    Draws an ellipse using the integer point midpoint algorithm.
*/
static void drawEllipse_midpoint_i(const QRect &rect, const QRect &clip,
                                   ProcessSpans pen_func, ProcessSpans brush_func,
                                   QSpanData *pen_data, QSpanData *brush_data)
{
#ifdef FLOATING_POINT_BUGGY_OR_NO_FPU // no fpu, so use fixed point
    const QFixed a = QFixed(rect.width()) >> 1;
    const QFixed b = QFixed(rect.height()) >> 1;
    QFixed d = b*b - (a*a*b) + ((a*a) >> 2);
#else
    const qreal a = qreal(rect.width()) / 2;
    const qreal b = qreal(rect.height()) / 2;
    qreal d = b*b - (a*a*b) + 0.25*a*a;
#endif

    int x = 0;
    int y = (rect.height() + 1) / 2;
    int startx = x;

    // region 1
    while (a*a*(2*y - 1) > 2*b*b*(x + 1)) {
        if (d < 0) { // select E
            d += b*b*(2*x + 3);
            ++x;
        } else {     // select SE
            d += b*b*(2*x + 3) + a*a*(-2*y + 2);
            drawEllipsePoints(startx, y, x - startx + 1, rect, clip,
                              pen_func, brush_func, pen_data, brush_data);
            startx = ++x;
            --y;
        }
    }
    drawEllipsePoints(startx, y, x - startx + 1, rect, clip,
                      pen_func, brush_func, pen_data, brush_data);

    // region 2
#ifdef FLOATING_POINT_BUGGY_OR_NO_FPU
    d = b*b*(x + (QFixed(1) >> 1))*(x + (QFixed(1) >> 1))
        + a*a*((y - 1)*(y - 1) - b*b);
#else
    d = b*b*(x + 0.5)*(x + 0.5) + a*a*((y - 1)*(y - 1) - b*b);
#endif
    const int miny = rect.height() & 0x1;
    while (y > miny) {
        if (d < 0) { // select SE
            d += b*b*(2*x + 2) + a*a*(-2*y + 3);
            ++x;
        } else {     // select S
            d += a*a*(-2*y + 3);
        }
        --y;
        drawEllipsePoints(x, y, 1, rect, clip,
                          pen_func, brush_func, pen_data, brush_data);
    }
}

/*!
    \fn void QRasterPaintEngine::drawPoints(const QPoint *points, int pointCount)
    \overload

    Draws the first \a pointCount points in the buffer \a points

    The default implementation converts the first \a pointCount QPoints in \a points
    to QPointFs and calls the floating point version of drawPoints.
*/

/*!
    \fn void QRasterPaintEngine::drawEllipse(const QRect &rect)
    \overload

    Reimplement this function to draw the largest ellipse that can be
    contained within rectangle \a rect.
*/

#ifdef QT_DEBUG_DRAW
void dumpClip(int width, int height, QClipData *clip)
{
    QImage clipImg(width, height, QImage::Format_ARGB32_Premultiplied);
    clipImg.fill(0xffff0000);

    int x0 = width;
    int x1 = 0;
    int y0 = height;
    int y1 = 0;

    for (int i = 0; i < clip->count; ++i) {
        QSpan *span = clip->spans + i;
        for (int j = 0; j < span->len; ++j)
            clipImg.setPixel(span->x + j, span->y, 0xffffff00);
        x0 = qMin(x0, int(span->x));
        x1 = qMax(x1, int(span->x + span->len - 1));

        y0 = qMin(y0, int(span->y));
        y1 = qMax(y1, int(span->y));
    }

    static int counter = 0;

    Q_ASSERT(y0 >= 0);
    Q_ASSERT(x0 >= 0);
    Q_ASSERT(y1 >= 0);
    Q_ASSERT(x1 >= 0);

    fprintf(stderr,"clip %d: %d %d - %d %d\n", counter, x0, y0, x1, y1);
    clipImg.save(QString(QLatin1String("clip-%0.png")).arg(counter++));
}
#endif


QT_END_NAMESPACE
