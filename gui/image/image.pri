# -*-mode:sh-*-
# Qt image handling

# Qt kernel module

HEADERS += \
        image/qbitmap.h \
        image/qimage.h \
        image/qimage_p.h \
        image/qimageiohandler.h \
        image/qimagereader.h \
        image/qimagewriter.h \
        image/qnativeimage_p.h \
        image/qpixmap.h \
        image/qpixmap_raster_p.h \
        image/qpixmapcache.h \
        image/qpixmapdata_p.h \

SOURCES += \
        image/qbitmap.cpp \
        image/qimage.cpp \
        image/qimageiohandler.cpp \
        image/qimagereader.cpp \
        image/qimagewriter.cpp \
        image/qpixmap.cpp \
        image/qpixmapcache.cpp \
        image/qpixmapdata.cpp \
        image/qpixmap_raster.cpp \
        image/qnativeimage.cpp \

win32 {
    SOURCES += image/qpixmap_win.cpp
}
embedded {
    SOURCES += image/qpixmap_qws.cpp
}
x11 {
    HEADERS += image/qpixmap_x11_p.h
    SOURCES += image/qpixmap_x11.cpp 
}
mac {
    HEADERS += image/qpixmap_mac_p.h
    SOURCES += image/qpixmap_mac.cpp
}

# Built-in image format support
HEADERS += \
        image/qbmphandler_p.h \
        image/qppmhandler_p.h \
        image/qxbmhandler_p.h \
        image/qxpmhandler_p.h

SOURCES += \
        image/qbmphandler.cpp \
        image/qppmhandler.cpp \
        image/qxbmhandler.cpp \
        image/qxpmhandler.cpp

DEFINES *= QT_NO_IMAGEFORMAT_PNG
