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

/***************************************************************************/
/*                                                                         */
/*  qgrayraster_p.h, derived from ftgrays.h                                */
/*                                                                         */
/*    FreeType smooth renderer declaration                                 */
/*                                                                         */
/*  Copyright 1996-2001 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, ../../3rdparty/freetype/docs/FTL.TXT.  By continuing to use,  */
/*  modify, or distribute this file you indicate that you have read        */
/*  the license and understand and accept it fully.                        */
/***************************************************************************/


#ifndef __FTGRAYS_H__
#define __FTGRAYS_H__

/*
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
*/

#ifdef __cplusplus
  extern "C" {
#endif


#include <private/qrasterdefs_p.h>

  /*************************************************************************/
  /*                                                                       */
  /* To make ftgrays.h independent from configuration files we check       */
  /* whether QT_FT_EXPORT_VAR has been defined already.                    */
  /*                                                                       */
  /* On some systems and compilers (Win32 mostly), an extra keyword is     */
  /* necessary to compile the library as a DLL.                            */
  /*                                                                       */
#ifndef QT_FT_EXPORT_VAR
#define QT_FT_EXPORT_VAR( x )  extern  x
#endif

  QT_FT_EXPORT_VAR( const QT_FT_Raster_Funcs )  qt_ft_grays_raster;


#ifdef __cplusplus
  }
#endif

#endif /* __FTGRAYS_H__ */

/* END */
