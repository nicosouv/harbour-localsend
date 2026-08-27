#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QGuiApplication>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QTranslator>

#include <sailfishapp.h>

#include "appsettings.h"
#include "devicemodel.h"
#include "discovery.h"
#include "historymodel.h"
#include "receiveservice.h"
#include "selectionmodel.h"
#include "sendservice.h"
#include "transfermodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication *app = SailfishApp::application(argc, argv);
    app->setOrganizationName(QStringLiteral("harbour-localsend"));
    app->setApplicationName(QStringLiteral("harbour-localsend"));

    QQuickView *view = SailfishApp::createView();

    AppSettings settings;

    // One manager for the whole app. Discovery's subnet sweep alone opens a
    // couple of hundred requests, and a second manager would mean a second
    // connection pool competing with it for the same radio.
    QNetworkAccessManager network;

    DeviceModel devices;
    TransferModel transfer;
    HistoryModel history;
    SelectionModel selection;

    Discovery discovery(&settings, &devices, &network);
    ReceiveService receiver(&settings, &discovery, &transfer, &history, &network);
    SendService sender(&settings, &transfer, &history, &network);

    QTranslator *translator = new QTranslator(app);
    const QString translationsPath = SailfishApp::pathTo(
        QStringLiteral("translations")).toLocalFile();

    if (translator->load(QStringLiteral("harbour-localsend-%1").arg(settings.language()),
                         translationsPath)) {
        app->installTranslator(translator);
    }

    // Reloading the view is the only way to re-evaluate every qsTr() in a
    // running Qt 5.6 app; retranslate() does not reach QML bindings.
    QObject::connect(&settings, &AppSettings::languageChanged,
                     [app, translator, view, &settings, translationsPath]() {
        app->removeTranslator(translator);
        if (translator->load(
                QStringLiteral("harbour-localsend-%1").arg(settings.language()),
                translationsPath)) {
            app->installTranslator(translator);
        }
        const QUrl source = view->source();
        view->engine()->clearComponentCache();
        view->setSource(QUrl());
        view->setSource(source);
    });

    QQmlContext *context = view->rootContext();
    context->setContextProperty(QStringLiteral("appSettings"), &settings);
    context->setContextProperty(QStringLiteral("deviceModel"), &devices);
    context->setContextProperty(QStringLiteral("transfer"), &transfer);
    context->setContextProperty(QStringLiteral("historyModel"), &history);
    context->setContextProperty(QStringLiteral("selection"), &selection);
    context->setContextProperty(QStringLiteral("discovery"), &discovery);
    context->setContextProperty(QStringLiteral("receiver"), &receiver);
    context->setContextProperty(QStringLiteral("sender"), &sender);

    // Being discoverable is the whole point of the app, so it starts listening
    // before the first frame rather than when some page happens to load.
    if (settings.receiveEnabled())
        receiver.startListening();
    discovery.start();

    // Receiving is what the port is for: turning it off should also stop us
    // announcing, or peers would keep seeing a device that refuses everything.
    QObject::connect(&settings, &AppSettings::receiveEnabledChanged,
                     [&settings, &receiver, &discovery]() {
        if (settings.receiveEnabled()) {
            receiver.startListening();
            discovery.start();
        } else {
            receiver.stopListening();
            discovery.stop();
        }
    });

    view->setSource(SailfishApp::pathTo(QStringLiteral("qml/harbour-localsend.qml")));
    view->show();

    return app->exec();
}
