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
#include "stdafx.h"
#include "qhdcimage.h"


typedef struct {
    BITMAPINFOHEADER bmiHeader;
    DWORD redMask;
    DWORD greenMask;
    DWORD blueMask;
} BITMAPINFO_MASK;


QHDCImage::QHDCImage(int width, int height, QImage::Format format, bool isTextBuffer, HDC drawDC)
{
    BITMAPINFO_MASK bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biSizeImage   = 0;

    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.redMask = 0;
    bmi.greenMask = 0;
    bmi.blueMask = 0;

    hdc = CreateCompatibleDC(drawDC);

    uchar *bits = 0;
    bitmap = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO *>(&bmi), DIB_RGB_COLORS, (void**) &bits, 0, 0);

    null_bitmap = (HBITMAP)SelectObject(hdc, bitmap);
    image = QImage(bits, width, height, format);
    GdiFlush();
}

QHDCImage::~QHDCImage()
{
    if (bitmap || hdc) {
        if (null_bitmap)
            SelectObject(hdc, null_bitmap);
        DeleteDC(hdc);
        DeleteObject(bitmap);
    }
}

QImage::Format QHDCImage::systemFormat()
{
    return QImage::Format_RGB32;
}

