import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"
import "../components/Formatting.js" as Formatting

// The transfer in progress, in either direction.
//
// One page for both because there is nothing different to say: the same ring,
// the same files, the same reasons to stop. Only the preposition changes.
Page {
    id: page

    objectName: "transferPage"
    allowedOrientations: defaultAllowedOrientations

    readonly property bool sending: transfer.direction === "send"
    readonly property bool waiting: transfer.state === "requesting"
    readonly property bool running: transfer.state === "active"
    readonly property bool over: transfer.state === "finished"
                                 || transfer.state === "failed"
                                 || transfer.state === "cancelled"

    readonly property color accentColor: {
        if (transfer.state === "failed")
            return Theme.errorColor
        if (transfer.state === "cancelled")
            return Theme.secondaryColor
        return Theme.highlightColor
    }

    function stop() {
        if (page.sending)
            sender.cancel()
        else
            receiver.cancel()
    }

    // Swiping away must not silently abandon a running transfer; the button
    // is the only way to stop one.
    backNavigation: true

    SilicaListView {
        id: fileList

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: footer.top
        }
        clip: true
        model: transfer

        header: Column {
            width: fileList.width
            spacing: 0

            PageHeader {
                title: page.sending ? qsTr("Sending") : qsTr("Receiving")
                description: transfer.peerAlias
            }

            // --- the ring ---------------------------------------------

            Item {
                width: parent.width
                height: ring.height + Theme.paddingLarge * 2

                ProgressRing {
                    id: ring
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(page.width, page.height) * 0.52
                    height: width
                    value: transfer.progress
                    indeterminate: page.waiting
                    ringColor: page.accentColor

                    Column {
                        anchors.centerIn: parent
                        width: parent.width
                        spacing: Theme.paddingSmall / 2

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: Theme.fontSizeHuge
                            color: page.accentColor
                            text: page.waiting ? "…" : Formatting.percent(transfer.progress)
                        }

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                            truncationMode: TruncationMode.Fade
                            text: Formatting.fileSize(transfer.transferredBytes)
                                  + " / " + Formatting.fileSize(transfer.totalBytes)
                        }
                    }
                }
            }

            // --- one line of status, whatever the state ------------------

            Label {
                width: parent.width - Theme.horizontalPageMargin * 2
                x: Theme.horizontalPageMargin
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: transfer.state === "failed" ? Theme.errorColor
                                                   : Theme.secondaryHighlightColor
                text: {
                    if (page.waiting)
                        return qsTr("Waiting for %1 to accept").arg(transfer.peerAlias)

                    if (page.running) {
                        var speed = Formatting.speed(transfer.bytesPerSecond)
                        var left = transfer.secondsRemaining >= 0
                            ? Formatting.duration(transfer.secondsRemaining) : ""
                        if (speed.length > 0 && left.length > 0)
                            return qsTr("%1 · %2 left").arg(speed).arg(left)
                        if (speed.length > 0)
                            return speed
                        return qsTr("Starting…")
                    }

                    if (transfer.state === "finished") {
                        return page.sending
                            ? qsTr("Sent %n file(s)", "", transfer.completedCount)
                            : qsTr("Saved %n file(s)", "", transfer.completedCount)
                    }

                    if (transfer.state === "cancelled")
                        return qsTr("Transfer stopped")

                    return transfer.errorText.length > 0
                        ? transfer.errorText : qsTr("Transfer failed")
                }
            }

            // Where the files went. Worth saying plainly: the download folder
            // is configurable, so "check your downloads" would be a guess.
            Label {
                width: parent.width - Theme.horizontalPageMargin * 2
                x: Theme.horizontalPageMargin
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                visible: !page.sending && transfer.destination.length > 0
                         && transfer.state === "finished"
                text: qsTr("in %1").arg(transfer.destination)
            }

            SectionHeader { text: qsTr("Files") }
        }

        delegate: TransferFileDelegate {
            width: fileList.width
            fileName: model.fileName
            fileSize: model.fileSize
            fileProgress: model.fileProgress
            fileStatus: model.fileStatus
            fileError: model.fileError

            // Only worth offering once there is something on disk to open.
            menu: (model.fileStatus === "done" && model.localPath.length > 0
                   && !page.sending) ? openMenuComponent : null

            Component {
                id: openMenuComponent

                ContextMenu {
                    MenuItem {
                        text: qsTr("Open")
                        onClicked: Qt.openUrlExternally(Formatting.fileUrl(model.localPath))
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }

    // --- the one action that matters at any moment ----------------------

    Item {
        id: footer
        anchors.bottom: parent.bottom
        width: parent.width
        height: actionButton.height + Theme.paddingLarge * 2

        Button {
            id: actionButton
            anchors.centerIn: parent
            text: page.over ? qsTr("Done") : qsTr("Stop")
            onClicked: {
                if (page.over)
                    pageStack.pop()
                else
                    page.stop()
            }
        }
    }
}
