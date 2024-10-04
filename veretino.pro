QT       += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

VERSION = 0.5.0
DEFINES += APP_VERSION=\\\"$${VERSION}\\\"
DEFINES += APP_NAME=\\\"Veretino\\\"
DEFINES += "APP_NAME_VERSION=\"\\\"Veretino $${VERSION}\\\"\""

SOURCES += \
    src/clickablelabel.cpp \
    src/datacontainer.cpp \
    src/datamaintainer.cpp \
    src/dialogabout.cpp \
    src/dialogcontentslist.cpp \
    src/dialogdbcreation.cpp \
    src/dialogdbstatus.cpp \
    src/dialogexistingdbs.cpp \
    src/dialogfileprocresult.cpp \
    src/dialogsettings.cpp \
    src/files.cpp \
    src/filterrule.cpp \
    src/iconprovider.cpp \
    src/jsondb.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/manager.cpp \
    src/menuactions.cpp \
    src/modeselector.cpp \
    src/numbers.cpp \
    src/procstate.cpp \
    src/progressbar.cpp \
    src/proxymodel.cpp \
    src/settings.cpp \
    src/shacalculator.cpp \
    src/statusbar.cpp \
    src/tools.cpp \
    src/treeitem.cpp \
    src/treemodel.cpp \
    src/treemodeliterator.cpp \
    src/treewidgetfiletypes.cpp \
    src/treewidgetitem.cpp \
    src/view.cpp

HEADERS += \
    src/clickablelabel.h \
    src/datacontainer.h \
    src/datamaintainer.h \
    src/dialogabout.h \
    src/dialogcontentslist.h \
    src/dialogdbcreation.h \
    src/dialogdbstatus.h \
    src/dialogexistingdbs.h \
    src/dialogfileprocresult.h \
    src/dialogsettings.h \
    src/files.h \
    src/filterrule.h \
    src/iconprovider.h \
    src/jsondb.h \
    src/mainwindow.h \
    src/manager.h \
    src/menuactions.h \
    src/modeselector.h \
    src/numbers.h \
    src/procstate.h \
    src/progressbar.h \
    src/proxymodel.h \
    src/settings.h \
    src/shacalculator.h \
    src/statusbar.h \
    src/tools.h \
    src/treeitem.h \
    src/treemodel.h \
    src/treemodeliterator.h \
    src/treewidgetfiletypes.h \
    src/treewidgetitem.h \
    src/view.h

FORMS += \
    src/dialogabout.ui \
    src/dialogcontentslist.ui \
    src/dialogdbcreation.ui \
    src/dialogdbstatus.ui \
    src/dialogexistingdbs.ui \
    src/dialogfileprocresult.ui \
    src/dialogsettings.ui \
    src/mainwindow.ui

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
    data.files += res/icons/generic/veretino.svg
    data.path = $$PREFIX/share/icons/hicolor/scalable/apps

    INSTALLS += shortcutfiles
    INSTALLS += data
}

DISTFILES += \
    res/veretino.desktop \
    res/icons/generic/veretino.svg

!isEmpty(target.path): INSTALLS += target
