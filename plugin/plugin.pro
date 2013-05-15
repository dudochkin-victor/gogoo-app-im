TEMPLATE = lib
TARGET = IM
QT += declarative dbus network
CONFIG += qt plugin link_pkgconfig mobility
PKGCONFIG += TelepathyQt4 TelepathyQt4Yell TelepathyQt4YellFarstream qt-gst-qml-sink QtGLib-2.0 glib-2.0 TelepathyLoggerQt4
LIBS += -ltelepathy-qt4-yell-models -L../telepathy-qml-lib -ltelepathy-qml -ltelepathy-qt4-yell-farstream -ltelepathy-logger-qt4
TARGET = $$qtLibraryTarget($$TARGET)
DESTDIR = $$TARGET
OBJECTS_DIR = .obj
MOC_DIR = .moc
MOBILITY = multimedia

plugin.files += $$TARGET
plugin.path += $$[QT_INSTALL_IMPORTS]/MeeGo/App/
INSTALLS += plugin

SOURCES += components.cpp \
    accounthelper.cpp \
    contactssortfilterproxymodel.cpp \
    accountsmodelfactory.cpp \
    imaccountsmodel.cpp \
    settingshelper.cpp \
    imavatarimageprovider.cpp \
    accountssortfilterproxymodel.cpp \
    imgroupchatmodelitem.cpp \
    imgroupchatmodel.cpp

HEADERS += components.h \
    accounthelper.h \
    contactssortfilterproxymodel.h \
    accountsmodelfactory.h \
    imaccountsmodel.h \
    settingshelper.h \
    imavatarimageprovider.h \
    accountssortfilterproxymodel.h \
    imgroupchatmodelitem.h \
    imgroupchatmodel.h
