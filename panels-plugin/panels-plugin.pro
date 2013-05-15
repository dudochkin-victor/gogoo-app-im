
TARGET = im_panels_plugin
TEMPLATE = lib
QT += declarative dbus network
CONFIG += qt plugin link_pkgconfig
PKGCONFIG += TelepathyQt4 meego-ux-content

# use pkg-config paths for include in both g++ and moc
INCLUDEPATH += $$system(pkg-config --cflags meego-ux-content \
    | tr \' \' \'\\n\' | grep ^-I | cut -d 'I' -f 2-)

LIBS += -ltelepathy-qt4-yell-models -L../telepathy-qml-lib -ltelepathy-qml -lmeegouxcontent
TARGET = $$qtLibraryTarget($$TARGET)
DESTDIR = $$TARGET
OBJECTS_DIR = .obj
MOC_DIR = .moc

SOURCES += imfeedmodel.cpp \
           imservmodel.cpp \
           implugin.cpp \
    imfeedmodelitem.cpp \
    imfeedmodelfilter.cpp

HEADERS += imfeedmodel.h \
           imservmodel.h \
           implugin.h \
    imfeedmodelitem.h \
    imfeedmodelfilter.h

clientfiles.files = *.client
clientfiles.path = $$INSTALL_ROOT/usr/share/telepathy/clients/

INSTALLS += clientfiles

servicefiles.files = *.service
servicefiles.path = $$INSTALL_ROOT/usr/share/dbus-1/services/

INSTALLS += servicefiles

target.path = $$[QT_INSTALL_PLUGINS]/MeeGo/Content/
INSTALLS += target
