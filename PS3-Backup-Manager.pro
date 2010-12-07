TEMPLATE = app
QT = gui \
     core \
     network
unix {
    CONFIG += qt \
    static \
    release #\
    #warn_on \
    #console
}
win32 {
    CONFIG += qt \
    static \
    release
}
macx {
    CONFIG += qt \
    x86 x86_64 \
    release
}
DESTDIR = bin
OBJECTS_DIR = obj
MOC_DIR = obj
UI_DIR = obj
RCC_DIR = src
FORMS += ui/mainwindow.ui \
    ui/ftpconnectdialog.ui \
    ui/progressdialog.ui \
    ui/fileconflictdialog.ui \
    ui/sfoeditordialog.ui \
    ui/licensedialog.ui
SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/myfilesystemmodel.cpp \
    src/mydir.cpp \
    src/ftpconnectdialog.cpp \
    src/adrefresher.cpp \
    src/myftptreewidget.cpp \
    src/myfilesystemview.cpp \
    src/mygamestreewidget.cpp \
    src/coverdownloader.cpp \
    src/progressdialog.cpp \
    src/fileconflictdialog.cpp \
    src/fileutils.cpp \
    src/myfileinfo.cpp \
    src/gamelist.cpp \
    src/appclipboard.cpp \
    src/gameitem.cpp \
    src/sfoparamlist.cpp \
    src/sfoparamitem.cpp \
    src/sfoeditordialog.cpp \
    src/licensedialog.cpp \
    src/ad.cpp
HEADERS += src/mainwindow.h \
    src/openurlthread.h \
    src/myfilesystemmodel.h \
    src/mydir.h \
    src/ftpconnectdialog.h \
    src/adrefresher.h \
    src/myftptreewidget.h \
    src/cleanerthread.h \
    src/myfilesystemview.h \
    src/appclipboard.h \
    src/mygamestreewidget.h \
    src/gamelist.h \
    src/gamelistthread.h \
    src/coverdownloader.h \
    src/progressdialog.h \
    src/removethread.h \
    src/copythread.h \
    src/fileconflictdialog.h \
    src/fileutils.h \
    src/movethread.h \
    src/syncthread.h \
    src/splitthread.h \
    src/myfileinfo.h \
    src/gameitem.h \
    src/sfoparamitem.h \
    src/sfoparamlist.h \
    src/sfoeditordialog.h \
    src/licensedialog.h \
    src/ad.h \
    src/covercachethread.h
RESOURCES += \
    rc/ressources.qrc
TRANSLATIONS += translations/app_fr.ts
win32:RC_FILE = rc/win_ressources.rc
macx:RC_FILE = rc/mac_icons.icns

contains(CONFIG, static): {
    ######### on main.cpp use defined ########
    DEFINES += _USE_STATIC_BUILDS_

    LIBS += -L$$[QT_INSTALL_PLUGINS]/iconengines -L$$[QT_INSTALL_PLUGINS]/imageformats

    exists($$[QT_INSTALL_PLUGINS]/iconengines/libqsvgicon.a) {
        QT += svg
        QTPLUGIN += qsvgicon
        DEFINES += _USE_qsvgicon
    }

    exists($$[QT_INSTALL_PLUGINS]/imageformats/libqico.a) {
        QTPLUGIN += qico
        DEFINES += _USE_qico
    }

    exists($$[QT_INSTALL_PLUGINS]/imageformats/libqjpeg.a) {
        QTPLUGIN += qjpeg
        DEFINES += _USE_qjpeg
    }

    exists($$[QT_INSTALL_PLUGINS]/imageformats/libqgif.a) {
        QTPLUGIN += qgif
        DEFINES += _USE_qgif
    }
}
