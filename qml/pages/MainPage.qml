import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import "../components"
import "../components/Formatting.js" as Formatting

// Where the app lives: who we are, who else is out there, and what is staged
// to send.
//
// The two orders both work. Tap a device with an empty tray and the picker
// opens, then the transfer starts as soon as files come back. Stage files
// first and a tap sends them straight away. Neither is the "real" flow.
Page {
    id: page

    objectName: "mainPage"
    allowedOrientations: defaultAllowedOrientations

    // Set when the picker was opened by tapping a device, so the transfer can
    // continue on its own once files come back. Null when the picker was
    // opened from the menu, where there is nothing to continue to.
    property var pendingDevice: null

    function chooseFilesFor(device) {
        page.pendingDevice = device === undefined ? null : device
        pageStack.push(filePickerComponent)
    }

    function filesPicked(paths) {
        selection.addAll(paths)

        var target = page.pendingDevice
        page.pendingDevice = null
        if (target && selection.count > 0)
            page.startSend(target)
    }

    function startSend(device) {
        if (selection.empty) {
            page.chooseFilesFor(device)
            return
        }
        if (transfer.active)
            return

        sender.sendFiles(device, selection.paths())
        pageStack.push(Qt.resolvedUrl("TransferPage.qml"))
    }

    SilicaListView {
        id: deviceList

        anchors.fill: parent
        // The tray floats over the list, so the last row has to be able to
        // clear it rather than sit underneath.
        bottomMargin: tray.visibleSize
        model: deviceModel

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("History")
                onClicked: pageStack.push(Qt.resolvedUrl("HistoryPage.qml"))
            }
            MenuItem {
                text: discovery.scanning ? qsTr("Stop scanning") : qsTr("Scan network")
                onClicked: {
                    if (discovery.scanning)
                        discovery.cancelScan()
                    else
                        discovery.scanSubnet()
                }
            }
            MenuItem {
                text: qsTr("Look again")
                onClicked: discovery.refresh()
            }
        }

        PushUpMenu {
            MenuItem {
                text: qsTr("Clear selection")
                visible: !selection.empty
                onClicked: selection.clear()
            }
            MenuItem {
                text: qsTr("Add files")
                onClicked: page.chooseFilesFor(null)
            }
        }

        header: Column {
            width: deviceList.width
            spacing: 0

            PageHeader { title: qsTr("LocalSend") }

            // --- our own device, announcing -----------------------------

            Item {
                width: parent.width
                height: radar.height + Theme.paddingLarge

                RadarPulse {
                    id: radar
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(page.width, page.height) * 0.56
                    height: width
                    active: discovery.running && appSettings.receiveEnabled

                    DeviceBadge {
                        anchors.centerIn: parent
                        width: parent.width * 0.26
                        deviceType: "mobile"
                        accent: Theme.highlightColor
                    }
                }
            }

            BackgroundItem {
                width: parent.width
                height: identity.height + Theme.paddingLarge
                onClicked: pageStack.push(aliasDialogComponent)

                Column {
                    id: identity
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    spacing: Theme.paddingSmall / 2

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: appSettings.alias
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                        truncationMode: TruncationMode.Fade
                    }

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                        truncationMode: TruncationMode.Fade
                        text: {
                            if (!appSettings.receiveEnabled)
                                return qsTr("Receiving is off — others cannot send to you")
                            if (!receiver.listening)
                                return receiver.listenError.length > 0
                                    ? qsTr("Port %1 is unavailable").arg(appSettings.port)
                                    : qsTr("Not listening")
                            var where = discovery.localAddress
                            return where.length > 0
                                ? qsTr("Ready · %1 on port %2").arg(where).arg(receiver.port)
                                : qsTr("Ready on port %1").arg(receiver.port)
                        }
                    }

                    // Stated plainly rather than left to be assumed. Whether
                    // what you are about to send is readable by the network
                    // is not a detail to bury in a settings page.
                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeExtraSmall
                        visible: appSettings.receiveEnabled && receiver.listening
                        color: appSettings.encrypted ? Theme.highlightColor
                                                     : Theme.errorColor
                        text: appSettings.encrypted ? qsTr("Encrypted")
                                                    : qsTr("Not encrypted")
                    }
                }
            }

            // --- a warning worth interrupting for ------------------------

            Item {
                width: parent.width
                height: multicastWarning.visible
                        ? multicastWarning.height + Theme.paddingLarge : 0

                Label {
                    id: multicastWarning
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    // Multicast being blocked is invisible otherwise: the
                    // device list is simply empty, and nothing says why.
                    visible: discovery.running && !discovery.multicastReady
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.errorColor
                    text: qsTr("This network is blocking discovery. Pull down and choose Scan network.")
                }
            }

            SectionHeader { text: qsTr("Nearby devices") }

            // --- sweep progress -----------------------------------------

            Item {
                width: parent.width
                height: discovery.scanning ? scanRow.height + Theme.paddingLarge : 0
                visible: discovery.scanning

                Column {
                    id: scanRow
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    spacing: Theme.paddingSmall

                    Label {
                        text: qsTr("Scanning the network… %1%").arg(discovery.scanProgress)
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryHighlightColor
                    }

                    Rectangle {
                        width: parent.width
                        height: Math.max(1, Theme.paddingSmall / 3)
                        radius: height / 2
                        color: Theme.rgba(Theme.primaryColor, 0.12)

                        Rectangle {
                            width: parent.width * (discovery.scanProgress / 100)
                            height: parent.height
                            radius: parent.radius
                            color: Theme.highlightColor
                            Behavior on width { NumberAnimation { duration: 200 } }
                        }
                    }
                }
            }

            // --- nothing found yet ----------------------------------------
            //
            // Part of the header rather than a ViewPlaceholder. Silica centres
            // a placeholder on the view and takes no account of the header, so
            // with a header this tall it lands squarely on top of the radar and
            // the device name. Here it simply follows them.
            Item {
                width: parent.width
                height: emptyState.height + Theme.paddingLarge * 2
                visible: deviceModel.count === 0 && !discovery.scanning

                Column {
                    id: emptyState
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    spacing: Theme.paddingMedium

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        text: qsTr("Nobody yet")
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.secondaryHighlightColor
                    }

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                        text: appSettings.receiveEnabled
                            ? qsTr("Open LocalSend on another device on the same network. It should turn up here within a few seconds.")
                            : qsTr("Receiving is off. Turn it back on in Settings to be found.")
                    }
                }
            }
        }

        delegate: DeviceDelegate {
            width: deviceList.width
            deviceAlias: model.alias
            hardware: model.hardware
            deviceType: model.deviceType
            fingerprint: model.fingerprint
            address: model.address
            stale: model.stale
            sendsImmediately: !selection.empty

            onSendRequested: page.startSend(deviceModel.get(index))
            onDetailsRequested: pageStack.push(deviceDetailComponent,
                                               { device: deviceModel.get(index) })
        }

        VerticalScrollDecorator {}
    }

    SelectionTray {
        id: tray
        onReviewRequested: pageStack.push(Qt.resolvedUrl("SelectionPage.qml"))
        onClearRequested: selection.clear()
    }

    // --- pushed pages -------------------------------------------------

    Component {
        id: filePickerComponent

        MultiContentPickerDialog {
            title: qsTr("Select files to send")

            onAccepted: {
                var paths = []
                for (var i = 0; i < selectedContent.count; i++) {
                    var item = selectedContent.get(i)
                    // Which of the two the picker fills in depends on where
                    // the content came from, so take whichever is there.
                    var path = item.filePath
                    if (!path || path.length === 0)
                        path = item.url
                    if (path && path.length > 0)
                        paths.push("" + path)
                }
                page.filesPicked(paths)
            }

            onRejected: page.pendingDevice = null
        }
    }

    Component {
        id: aliasDialogComponent

        Dialog {
            id: aliasDialog
            canAccept: aliasField.text.trim().length > 0

            Column {
                width: parent.width

                DialogHeader { title: qsTr("Your device name") }

                TextField {
                    id: aliasField
                    width: parent.width
                    text: appSettings.alias
                    label: qsTr("Shown to other devices")
                    placeholderText: qsTr("Device name")
                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: aliasDialog.accept()
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Suggest another")
                    onClicked: aliasField.text = appSettings.suggestAlias()
                }
            }

            onAccepted: appSettings.alias = aliasField.text.trim()
        }
    }

    Component {
        id: deviceDetailComponent

        Page {
            id: detailPage
            property var device: ({})

            allowedOrientations: defaultAllowedOrientations

            SilicaFlickable {
                anchors.fill: parent
                contentHeight: detailColumn.height

                Column {
                    id: detailColumn
                    width: parent.width

                    PageHeader { title: detailPage.device.alias || qsTr("Device") }

                    DeviceBadge {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Theme.itemSizeExtraLarge
                        fingerprint: detailPage.device.fingerprint || ""
                        deviceType: detailPage.device.deviceType || "desktop"
                    }

                    Item { width: 1; height: Theme.paddingLarge }

                    DetailItem {
                        label: qsTr("Model")
                        value: detailPage.device.hardware || qsTr("Unknown")
                    }
                    DetailItem {
                        label: qsTr("Type")
                        value: detailPage.device.deviceType || qsTr("Unknown")
                    }
                    DetailItem {
                        label: qsTr("Address")
                        value: (detailPage.device.address || "")
                               + ":" + (detailPage.device.port || "")
                    }
                    DetailItem {
                        label: qsTr("Transport")
                        value: detailPage.device.protocol || "http"
                    }
                    DetailItem {
                        label: qsTr("Fingerprint")
                        value: (detailPage.device.fingerprint || "").substring(0, 16)
                    }

                    Item { width: 1; height: Theme.paddingLarge }

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Send files")
                        enabled: !transfer.active
                        onClicked: {
                            var target = detailPage.device
                            pageStack.pop()
                            page.startSend(target)
                        }
                    }
                }

                VerticalScrollDecorator {}
            }
        }
    }
}
