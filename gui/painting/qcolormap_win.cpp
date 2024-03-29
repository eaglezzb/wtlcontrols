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

#include "qcolor.h"
#include "qcolormap.h"
#include "core/qvector.h"
#include "core/qt_windows.h"

#if defined(Q_OS_WINCE)
#include "qguifunctions_wince.h"
#endif

QT_BEGIN_NAMESPACE

class QColormapPrivate
{
public:
    inline QColormapPrivate()
        : ref(1), mode(QColormap::Direct), depth(0), hpal(0)
    { }

    QAtomicInt ref;

    QColormap::Mode mode;
    int depth;
    int numcolors;

    HPALETTE hpal;
    QVector<QColor> palette;
};

static QColormapPrivate *screenMap = 0;

void QColormap::initialize()
{
    HDC dc = GetDC(0);

    screenMap = new QColormapPrivate;
    screenMap->depth = GetDeviceCaps(dc, BITSPIXEL);

    screenMap->numcolors = -1;
    if (GetDeviceCaps(dc, RASTERCAPS) & RC_PALETTE)
        screenMap->numcolors = GetDeviceCaps(dc, SIZEPALETTE);

    if (screenMap->numcolors <= 16 || screenMap->numcolors > 256)        // no need to create palette
        return;

    LOGPALETTE* pal = 0;
    int numPalEntries = 6*6*6; // color cube

    pal = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + numPalEntries * sizeof(PALETTEENTRY));
    // Make 6x6x6 color cube
    int idx = 0;
    for(int ir = 0x0; ir <= 0xff; ir+=0x33) {
        for(int ig = 0x0; ig <= 0xff; ig+=0x33) {
            for(int ib = 0x0; ib <= 0xff; ib+=0x33) {
                pal->palPalEntry[idx].peRed = ir;
                pal->palPalEntry[idx].peGreen = ig;
                pal->palPalEntry[idx].peBlue = ib;
                pal->palPalEntry[idx].peFlags = 0;
                idx++;
            }
        }
    }

    pal->palVersion = 0x300;
    pal->palNumEntries = numPalEntries;

    screenMap->hpal = CreatePalette(pal);
    if (!screenMap->hpal)
        qErrnoWarning("QColor::initialize: Failed to create logical palette");
    free (pal);

    SelectPalette(dc, screenMap->hpal, FALSE);
    RealizePalette(dc);

    PALETTEENTRY paletteEntries[256];
    screenMap->numcolors = GetPaletteEntries(screenMap->hpal, 0, 255, paletteEntries);

    screenMap->palette.resize(screenMap->numcolors);
    for (int i = 0; i < screenMap->numcolors; i++) {
        screenMap->palette[i] = qRgb(paletteEntries[i].peRed,
                                     paletteEntries[i].peGreen,
                                     paletteEntries[i].peBlue);
    }
}

void QColormap::cleanup()
{
    if (!screenMap)
        return;

    if (screenMap->hpal) {                                // delete application global
        DeleteObject(screenMap->hpal);                        // palette
        screenMap->hpal = 0;
    }

    delete screenMap;
    screenMap = 0;
}

QColormap QColormap::instance(int)
{ return QColormap(); }

QColormap::QColormap()
    : d(screenMap)
{ d->ref.ref(); }

QColormap::QColormap(const QColormap &colormap)
    :d (colormap.d)
{ d->ref.ref(); }

QColormap::~QColormap()
{
    if (!d->ref.deref())
        delete d;
}

QColormap::Mode QColormap::mode() const
{ return d->mode; }

int QColormap::depth() const
{ return d->depth; }

int QColormap::size() const
{ return d->numcolors; }

uint QColormap::pixel(const QColor &color) const
{
    const QColor c = color.toRgb();
    COLORREF rgb = RGB(c.red(), c.green(), c.blue());
    if (d->hpal)
        return PALETTEINDEX(GetNearestPaletteIndex(d->hpal, rgb));
    return rgb;
}

const QColor QColormap::colorAt(uint pixel) const
{
    if (d->hpal) {
        if (pixel < uint(d->numcolors))
            return d->palette.at(pixel);
        return QColor();
    }
    return QColor(GetRValue(pixel), GetGValue(pixel), GetBValue(pixel));
}


HPALETTE QColormap::hPal()
{ return screenMap ? screenMap->hpal : 0; }


const QVector<QColor> QColormap::colormap() const
{ return d->palette; }

QColormap &QColormap::operator=(const QColormap &colormap)
{ qAtomicAssign(d, colormap.d); return *this; }


QT_END_NAMESPACE
