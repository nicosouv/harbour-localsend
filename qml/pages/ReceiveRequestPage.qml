import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"
import "../components/Formatting.js" as Formatting

// Somebody wants to send you something.
//
// Deliberately not a Silica Dialog: accepting a Dialog is a forward swipe,
// and the one decision that must never be made by accident is the one that
// writes a stranger's files onto the phone. Two buttons, both explicit.
Page {
    id: page

    objectName: "receiveRequestPage"
    allowedOrientations: defaultAllowedOrientations

    // Leaving without answering is an answer. The flag keeps accept() from
    // being undone by the destruction that follows it.
    property bool decided: false

    function answer(accepted) {
        if (page.decided)
            return
        page.decided = true
        if (accepted)
            receiver.accept()
        else
            receiver.decline()
    }

    Component.onDestruction: {
        if (!page.decided && transfer.state === "pending")
            receiver.decline()
    }

    SilicaListView {
        id: fileList

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: buttons.top
        }
        clip: true
        model: transfer

        header: Column {
            width: fileList.width
            spacing: 0

            PageHeader { title: qsTr("Incoming files") }

            Item {
                width: parent.width
                height: senderInfo.height + Theme.paddingLarge * 2

                Column {
                    // Not "sender": that is the SendService context property,
                    // and an id shadowing it would silently break any page
                    // that later needed the real one.
                    id: senderInfo
                    anchors.centerIn: parent
                    width: parent.width - Theme.horizontalPageMargin * 2
                    spacing: Theme.paddingMedium

                    DeviceBadge {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Theme.itemSizeExtraLarge
                        deviceType: transfer.peerDeviceType
                        // The address, not a fingerprint we do not hold here:
                        // it still gives the sender a stable colour.
                        fingerprint: transfer.peerAddress
                    }

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: transfer.peerAlias
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                        truncationMode: TruncationMode.Fade
                    }

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        text: qsTr("%n file(s)", "", transfer.fileCount)
                              + " · " + Formatting.fileSize(transfer.totalBytes)
                    }

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                        text: qsTr("from %1").arg(transfer.peerAddress)
                    }
                }
            }

            SectionHeader { text: qsTr("What they are sending") }
        }

        delegate: TransferFileDelegate {
            width: fileList.width
            fileName: model.fileName
            fileSize: model.fileSize
            fileStatus: "waiting"
        }

        VerticalScrollDecorator {}
    }

    Column {
        id: buttons
        anchors {
            bottom: parent.bottom
            bottomMargin: Theme.paddingLarge
        }
        width: parent.width
        spacing: Theme.paddingMedium

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - Theme.horizontalPageMargin * 2
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: qsTr("Saved to %1").arg(appSettings.destination)
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingLarge

            Button {
                text: qsTr("Decline")
                onClicked: {
                    page.answer(false)
                    pageStack.pop()
                }
            }

            Button {
                text: qsTr("Accept")
                // The accented one, because it is the one being asked for.
                color: Theme.highlightColor
                onClicked: page.answer(true)
            }
        }
    }
}
