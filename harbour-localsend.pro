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

# Sailfish Secrets holds the key the app's own files are encrypted with, so a
# copy of the data directory decrypts to nothing. Guarded because every lane
# that is not the device lacks it: without it SecureStore writes in the clear
# and says so, rather than the build failing.
# pkg-config is queried directly rather than through PKGCONFIG, and this is
# not a style preference. sailfishapp.prf adds itself the PKGCONFIG way, and
# declaring link_pkgconfig here evaluates the mechanism early enough that
# -lsailfishapp never reaches the link line - the compile succeeds, the link
# fails on four SailfishApp symbols, and nothing points at the cause. It has
# cost two builds; scripts/check_qt56.py now fails on the pattern.
packagesExist(sailfishsecrets) {
    DEFINES += HAVE_SAILFISH_SECRETS
    QMAKE_CXXFLAGS += $$system(pkg-config --cflags sailfishsecrets)
    LIBS += $$system(pkg-config --libs sailfishsecrets)
    message("Sailfish Secrets found: app data will be encrypted at rest")
} else {
    warning("Sailfish Secrets not found: app data will be stored in the clear")
}

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
    src/knowndevices.cpp \
    src/protocol.cpp \
    src/ratelimiter.cpp \
    src/securestore.cpp \
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
    src/knowndevices.h \
    src/protocol.h \
    src/ratelimiter.h \
    src/securestore.h \
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
    qml/pages/AddDevicePage.qml \
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
