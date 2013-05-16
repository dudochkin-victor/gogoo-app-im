TEMPLATE = lib
TARGET = telepathy-qml
VERSION = 0.1.0

target.path = $$INSTALL_ROOT/usr/lib

SOURCES = telepathytypes.cpp \
          telepathymanager.cpp \
    channelhandler.cpp \
    chatagent.cpp \
    callagent.cpp \
    filetransferagent.cpp \
    farstreamchannel.cpp \
    panelschannelobserver.cpp \
    imconversationmodel.cpp \
    imchannelapprover.cpp \
    notificationmanager.cpp \
    filetransferitem.cpp \
    simplecontactslistmodel.cpp \
    mergedmodel.cpp \
    addcontacthelper.cpp \
    improtocolsmodel.cpp \
    serverauthagent.cpp
HEADERS = telepathytypes.h \
          telepathymanager.h \
    channelhandler.h \
    chatagent.h \
    callagent.h \
    filetransferagent.h \
    farstreamchannel.h \
    panelschannelobserver.h \
    imconversationmodel.h \
    imchannelapprover.h \
    notificationmanager.h \
    filetransferitem.h \
    simplecontactslistmodel.h \
    mergedmodel.h \
    addcontacthelper.h \
    improtocolsmodel.h \
    serverauthagent.h

headers.files = telepathytypes.h \
                telepathymanager.h \
                channelhandler.h \
                chatagent.h \
                accountsmodelfactory.h \
                callagent.h \
                panelschannelobserver.h \
                notificationmanager.h \
                simplecontactslistmodel.h
headers.path = $$INSTALL_ROOT/usr/include/telepathy-qml

QT += declarative dbus network
CONFIG += qt meegotouch link_pkgconfig create_pc create_prl mobility
MOBILITY += multimedia

OBJECTS_DIR = .obj
MOC_DIR = .moc

pkgconfig.path = $$INSTALL_ROOT/usr/lib/pkgconfig
pkgconfig.files = telepathy-qml.pc

PKGCONFIG += TelepathyQt4 TelepathyQt4Yell telepathy-farstream TelepathyQt4YellFarstream glib-2.0 dbus-1 \
             farsight2-0.10 telepathy-farstream gstreamer-0.10 qt-gst-qml-sink QtGLib-2.0 TelepathyLoggerQt4 telepathy-logger-0.2
LIBS += -ltelepathy-qt4-yell-models -ltelepathy-qt4-yell-farstream -ltelepathy-logger-qt4
#-ltelepathy-logger-qt4-models

#INSTALLS += target headers pkgconfig
INSTALLS += target pkgconfig
