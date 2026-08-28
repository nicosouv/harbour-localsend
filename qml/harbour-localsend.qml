import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Notifications 1.0
import "pages"

// Navigation for transfers lives here rather than in any page.
//
// The reason is that a transfer can begin without anyone touching the app: an
// incoming request arrives while the phone is in a pocket, on whatever page
// was last open. Only the window is guaranteed to exist at that moment, so
// only the window can decide what to put on screen.
ApplicationWindow {
    id: appWindow

    initialPage: Component { MainPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    function currentObjectName() {
        return pageStack.currentPage ? pageStack.currentPage.objectName : ""
    }

    function showTransferPage() {
        var current = appWindow.currentObjectName()
        if (current === "transferPage")
            return

        // Accepting turns the request page into the progress page, rather
        // than stacking one on top of the other and leaving a dead page to
        // swipe back through afterwards.
        if (current === "receiveRequestPage")
            pageStack.replace(Qt.resolvedUrl("pages/TransferPage.qml"))
        else
            pageStack.push(Qt.resolvedUrl("pages/TransferPage.qml"))
    }

    Connections {
        target: receiver

        onRequestArrived: {
            if (appWindow.currentObjectName() !== "receiveRequestPage")
                pageStack.push(Qt.resolvedUrl("pages/ReceiveRequestPage.qml"))

            // A request expires on its own, so a notification for one nobody
            // is looking at is the difference between receiving the files and
            // not.
            if (appSettings.notificationsEnabled && !Qt.application.active) {
                requestNotification.summary = qsTr("%1 wants to send you files")
                                              .arg(peerAlias)
                requestNotification.body = qsTr("%n file(s)", "", fileCount)
                requestNotification.publish()
            }
        }

        onTransferStarted: appWindow.showTransferPage()

        onTransferFinished: {
            if (!appSettings.notificationsEnabled || Qt.application.active)
                return

            if (status === "finished") {
                doneNotification.summary = qsTr("%n file(s) received", "", fileCount)
                doneNotification.body = destination
            } else {
                doneNotification.summary = qsTr("Transfer incomplete")
                doneNotification.body = transfer.peerAlias
            }
            doneNotification.publish()
        }
    }

    Connections {
        target: sender

        onPinRequired: {
            // Replace rather than stack: a second refusal must not leave two
            // PIN dialogs behind each other.
            if (appWindow.currentObjectName() === "pinDialog") {
                pageStack.replace(Qt.resolvedUrl("pages/PinDialog.qml"),
                                  { retry: retry, peerAlias: transfer.peerAlias })
            } else {
                pageStack.push(Qt.resolvedUrl("pages/PinDialog.qml"),
                               { retry: retry, peerAlias: transfer.peerAlias })
            }
        }

        onFinished: {
            if (!appSettings.notificationsEnabled || Qt.application.active)
                return
            if (status !== "finished")
                return

            doneNotification.summary = qsTr("%n file(s) sent", "", fileCount)
            doneNotification.body = transfer.peerAlias
            doneNotification.publish()
        }
    }

    // Only appName, summary and body are set. The finer properties of
    // Notification (urgency, transience) arrived at different Sailfish
    // releases, and one that is missing is not a warning: it fails the root
    // QML file, which is the one file whose failure means the app does not
    // start at all.
    Notification {
        id: requestNotification
        appName: "LocalSend"
    }

    Notification {
        id: doneNotification
        appName: "LocalSend"
    }

    // Files arriving from another app's share sheet. They are staged rather
    // than sent: the share sheet knows what to send but not to whom, and the
    // device is the one thing only the person holding the phone can choose.
    function receiveSharedFiles(paths) {
        if (!paths || paths.length === 0)
            return

        appWindow.activate()
        selection.addAll(paths)

        // Back to the device list, where the tray now shows what was shared
        // and a tap on a device sends it. A transfer already on screen is
        // left alone: popping it away would hide a transfer in flight to make
        // room for staging the next one.
        if (appWindow.currentObjectName() !== "transferPage"
                && pageStack.depth > 1) {
            pageStack.pop(null, PageStackAction.Immediate)
        }
    }

    Loader {
        id: shareTarget
        source: "components/ShareTarget.qml"
    }

    Connections {
        target: shareTarget.item
        ignoreUnknownSignals: true
        onFilesShared: appWindow.receiveSharedFiles(paths)
    }

    // Loaded indirectly so a missing keepalive plugin degrades to "the
    // transfer may pause with the screen off" instead of a window that will
    // not build.
    Loader {
        id: keepAlive
        source: "components/BackgroundKeeper.qml"
    }

    Binding {
        target: keepAlive.item
        property: "active"
        when: keepAlive.status === Loader.Ready
        value: appSettings.keepAwake
               && (transfer.state === "active" || transfer.state === "requesting")
    }
}
