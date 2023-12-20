QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

VERSION = 0.3.1
DEFINES += APP_VERSION=\\\"$${VERSION}\\\"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/aboutdialog.cpp \
    src/datacontainer.cpp \
    src/datamaintainer.cpp \
    src/dbstatusdialog.cpp \
    src/files.cpp \
    src/jsondb.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/manager.cpp \
    src/modeselector.cpp \
    src/procstate.cpp \
    src/proxymodel.cpp \
    src/settingsdialog.cpp \
    src/shacalculator.cpp \
    src/tools.cpp \
    src/treeitem.cpp \
    src/treemodel.cpp \
    src/treemodeliterator.cpp \
    src/view.cpp

HEADERS += \
    src/aboutdialog.h \
    src/datacontainer.h \
    src/datamaintainer.h \
    src/dbstatusdialog.h \
    src/files.h \
    src/jsondb.h \
    src/mainwindow.h \
    src/manager.h \
    src/modeselector.h \
    src/procstate.h \
    src/proxymodel.h \
    src/settingsdialog.h \
    src/shacalculator.h \
    src/tools.h \
    src/treeitem.h \
    src/treemodel.h \
    src/treemodeliterator.h \
    src/view.h

FORMS += \
    src/aboutdialog.ui \
    src/dbstatusdialog.ui \
    src/mainwindow.ui \
    src/settingsdialog.ui

RESOURCES += \
    res/resources.qrc

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin

win32:RC_ICONS += res/veretino.ico
unix {
    isEmpty(PREFIX) {
        PREFIX = /usr
    }

    target.path = $$PREFIX/bin

    shortcutfiles.files = res/veretino.desktop
    shortcutfiles.path = $$PREFIX/share/applications/
    data.files += res/veretino.png
    data.path = $$PREFIX/share/pixmaps/

    INSTALLS += shortcutfiles
    INSTALLS += data
}

DISTFILES += \
    res/veretino.desktop \
    res/veretino.png

!isEmpty(target.path): INSTALLS += target
