# -*-mode:sh-*-
# Qt image handling

# Qt kernel module

HEADERS += \
        image/qbitmap.h \
        image/qimage.h \
        image/qimage_p.h \
        image/qnativeimage_p.h \
        image/qpixmap.h \
        image/qpixmap_raster_p.h \
        image/qpixmapcache.h \
        image/qpixmapdata_p.h 

SOURCES += \
        image/qbitmap.cpp \
        image/qimage.cpp \
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

