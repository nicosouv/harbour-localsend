TARGET = harbour-localsend

CONFIG += sailfishapp
CONFIG += c++11

QT += network

# Qt cannot generate an X.509 certificate, and every device on a LocalSend
# network is its own certificate authority, so the TLS identity is built
# against OpenSSL directly. See src/certificate.cpp.
#
# Linked by name rather than through PKGCONFIG: sailfishapp.prf pulls itself
# in the same way, and declaring link_pkgconfig here evaluates it early enough
# that -lsailfishapp never reaches the link line. The headers are in the
# default include path on the target, so nothing else is needed. Only
# libcrypto is used - QSslSocket is Qt's, and no SSL_* function is called.
LIBS += -lcrypto

# Version is passed by the spec file (%qmake5 VERSION=%{version})
isEmpty(VERSION) {
    VERSION = 0.1.0
}
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += src/harbour-localsend.cpp \
    src/appsettings.cpp \
    src/certificate.cpp \
    src/crypto.cpp \
    src/deviceinfo.cpp \
    src/devicemodel.cpp \
    src/discovery.cpp \
    src/historymodel.cpp \
    src/httpserver.cpp \
    src/protocol.cpp \
    src/ratelimiter.cpp \
    src/receiveservice.cpp \
    src/selectionmodel.cpp \
    src/sendservice.cpp \
    src/tlsclient.cpp \
    src/transfermodel.cpp

HEADERS += src/appsettings.h \
    src/certificate.h \
    src/crypto.h \
    src/deviceinfo.h \
    src/devicemodel.h \
    src/discovery.h \
    src/historymodel.h \
    src/httpserver.h \
    src/protocol.h \
    src/ratelimiter.h \
    src/receiveservice.h \
    src/selectionmodel.h \
    src/sendservice.h \
    src/tlsclient.h \
    src/transfermodel.h

DISTFILES += qml/harbour-localsend.qml \
    qml/cover/CoverPage.qml \
    qml/pages/MainPage.qml \
    qml/pages/TransferPage.qml \
    qml/pages/ReceiveRequestPage.qml \
    qml/pages/SelectionPage.qml \
    qml/pages/HistoryPage.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/AboutPage.qml \
    qml/pages/PinDialog.qml \
    qml/components/DeviceGlyph.qml \
    qml/components/DeviceBadge.qml \
    qml/components/DeviceDelegate.qml \
    qml/components/RadarPulse.qml \
    qml/components/ProgressRing.qml \
    qml/components/TransferFileDelegate.qml \
    qml/components/SelectionTray.qml \
    qml/components/BackgroundKeeper.qml \
    qml/components/Formatting.js \
    qml/components/DeviceLook.js \
    rpm/harbour-localsend.spec \
    harbour-localsend.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

CONFIG += sailfishapp_i18n

TRANSLATIONS += translations/harbour-localsend-en.ts \
                translations/harbour-localsend-fr.ts \
                translations/harbour-localsend-de.ts \
                translations/harbour-localsend-es.ts \
                translations/harbour-localsend-fi.ts \
                translations/harbour-localsend-it.ts \
                translations/harbour-localsend-nb_NO.ts
