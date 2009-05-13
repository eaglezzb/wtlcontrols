# Qt kernel module

# Only used on platforms with CONFIG += precompile_header
#PRECOMPILED_HEADER = kernel/qt_gui_pch.h


KERNEL_P= kernel
HEADERS += \
	kernel/qwindowdefs.h \
	kernel/qpalette.h\
	kernel/qwindowdefs_win.h

SOURCES += \
	kernel/qpalette.cpp

unix:x11 {
	INCLUDEPATH += ../3rdparty/xorg
	HEADERS += \
		kernel/qx11embed_x11.h \
		kernel/qx11info_x11.h

	SOURCES += \
		kernel/qapplication_x11.cpp \
		kernel/qclipboard_x11.cpp \
		kernel/qcursor_x11.cpp \
		kernel/qdnd_x11.cpp \
		kernel/qdesktopwidget_x11.cpp \
		kernel/qmotifdnd_x11.cpp \
		kernel/qsound_x11.cpp \
		kernel/qwidget_x11.cpp \
		kernel/qwidgetcreate_x11.cpp \
		kernel/qx11embed_x11.cpp \
		kernel/qx11info_x11.cpp \
		kernel/qkeymapper_x11.cpp

        contains(QT_CONFIG, glib) {
            SOURCES += \
		kernel/qguieventdispatcher_glib.cpp
            HEADERS += \
                kernel/qguieventdispatcher_glib_p.h
            QMAKE_CXXFLAGS += $$QT_CFLAGS_GLIB
	    LIBS +=$$QT_LIBS_GLIB
	}
            SOURCES += \
		kernel/qeventdispatcher_x11.cpp
            HEADERS += \
                kernel/qeventdispatcher_x11_p.h
}

embedded {
	HEADERS += \
		kernel/qeventdispatcher_qws_p.h

	SOURCES += \
		kernel/qapplication_qws.cpp \
		kernel/qclipboard_qws.cpp \
		kernel/qcursor_qws.cpp \
		kernel/qdesktopwidget_qws.cpp \
		kernel/qdnd_qws.cpp \
		kernel/qeventdispatcher_qws.cpp \
		kernel/qsound_qws.cpp \
		kernel/qwidget_qws.cpp \
		kernel/qkeymapper_qws.cpp \
		kernel/qsessionmanager_qws.cpp

        contains(QT_CONFIG, glib) {
            SOURCES += \
		kernel/qeventdispatcher_glib_qws.cpp
            HEADERS += \
                kernel/qeventdispatcher_glib_qws_p.h
            QMAKE_CXXFLAGS += $$QT_CFLAGS_GLIB
	}


}

!embedded:!x11:mac {
	SOURCES += \
		kernel/qclipboard_mac.cpp \
		kernel/qmime_mac.cpp \
		kernel/qt_mac.cpp \
		kernel/qkeymapper_mac.cpp

        OBJECTIVE_HEADERS += \
                qcocoawindow_mac_p.h \
                qcocoawindowdelegate_mac_p.h \
                qcocoaview_mac_p.h \
                qcocoaapplication_mac_p.h \
                qcocoaapplicationdelegate_mac_p.h \

        OBJECTIVE_SOURCES += \
                kernel/qcursor_mac.mm \
                kernel/qdnd_mac.mm \
                kernel/qsound_mac.mm  \
                kernel/qapplication_mac.mm \
		kernel/qwidget_mac.mm \
		kernel/qcocoapanel_mac.mm \
		kernel/qcocoaview_mac.mm \
		kernel/qcocoawindow_mac.mm \
		kernel/qcocoawindowdelegate_mac.mm \
                kernel/qcocoamenuloader_mac.mm \
                kernel/qcocoaapplication_mac.mm \
                kernel/qcocoaapplicationdelegate_mac.mm \
                kernel/qt_cocoa_helpers_mac.mm \
		kernel/qdesktopwidget_mac.mm \
                kernel/qeventdispatcher_mac.mm \
                kernel/qcocoawindowcustomthemeframe_mac.mm

        HEADERS += \
                kernel/qt_cocoa_helpers_mac_p.h \
                kernel/qcocoaapplication_mac_p.h \
                kernel/qcocoaapplicationdelegate_mac_p.h \
                kernel/qeventdispatcher_mac_p.h

        MENU_NIB.files = mac/qt_menu.nib 
        MENU_NIB.path = Resources 
        MENU_NIB.version = Versions
        QMAKE_BUNDLE_DATA += MENU_NIB 
        RESOURCES += mac/maccursors.qrc

        LIBS += -framework AppKit
}

wince*: {
        HEADERS += \
                ../corelib/kernel/qfunctions_wince.h \
                kernel/qguifunctions_wince.h

        SOURCES += \
                ../corelib/kernel/qfunctions_wince.cpp \
                kernel/qguifunctions_wince.cpp
}
