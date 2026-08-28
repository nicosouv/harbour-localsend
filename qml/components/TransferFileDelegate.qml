import QtQuick 2.0
import Sailfish.Silica 1.0
import "Formatting.js" as Formatting

// One file inside a transfer.
//
// Every file carries its own progress line, not just the total: when one file
// out of forty fails, the aggregate barely moves and the list is the only
// place that says which one it was.
ListItem {
    id: delegate

    property string fileName: ""
    property real fileSize: 0
    property real fileProgress: 0
    property string fileStatus: "waiting"
    property string fileError: ""

    readonly property bool failed: fileStatus === "failed"
    readonly property bool skipped: fileStatus === "skipped"
    readonly property bool done: fileStatus === "done"
    readonly property bool running: fileStatus === "transferring"

    readonly property color statusColor: failed ? Theme.errorColor
        : (done ? Theme.highlightColor
                : (running ? Theme.highlightColor : Theme.secondaryColor))

    contentHeight: Theme.itemSizeSmall
    highlighted: false

    Rectangle {
        id: marker
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.paddingSmall
        height: width
        radius: width / 2
        color: delegate.statusColor
        opacity: delegate.skipped ? 0.4 : 1.0

        // A quiet heartbeat on the file actually moving, so the eye lands on
        // the right row without having to read anything.
        SequentialAnimation on opacity {
            running: delegate.running && Qt.application.active
            loops: Animation.Infinite
            NumberAnimation { to: 0.3; duration: 700; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutQuad }
        }
    }

    Column {
        anchors {
            left: marker.right
            leftMargin: Theme.paddingMedium
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        spacing: Theme.paddingSmall / 2

        Row {
            width: parent.width
            spacing: Theme.paddingMedium

            Label {
                width: parent.width - trailing.width - Theme.paddingMedium
                text: delegate.fileName
                textFormat: Text.PlainText
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeSmall
                color: delegate.skipped ? Theme.secondaryColor : Theme.primaryColor
            }

            Label {
                id: trailing
                text: {
                    if (delegate.failed)
                        return qsTr("failed")
                    if (delegate.skipped)
                        return qsTr("skipped")
                    if (delegate.done)
                        return Formatting.fileSize(delegate.fileSize)
                    if (delegate.running)
                        return Formatting.percent(delegate.fileProgress)
                    return Formatting.fileSize(delegate.fileSize)
                }
                font.pixelSize: Theme.fontSizeExtraSmall
                color: delegate.failed ? Theme.errorColor : Theme.secondaryColor
            }
        }

        // Shown only while a file is in flight: a full or empty bar on every
        // other row would be forty bars of noise.
        Rectangle {
            width: parent.width
            height: Math.max(1, Theme.paddingSmall / 3)
            radius: height / 2
            color: Theme.rgba(Theme.primaryColor, 0.12)
            visible: delegate.running

            Rectangle {
                width: parent.width * Math.max(0, Math.min(1, delegate.fileProgress))
                height: parent.height
                radius: parent.radius
                color: Theme.highlightColor

                Behavior on width {
                    NumberAnimation { duration: 220; easing.type: Easing.OutQuad }
                }
            }
        }

        Label {
            width: parent.width
            text: delegate.fileError
            textFormat: Text.PlainText
            visible: delegate.failed && delegate.fileError.length > 0
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.errorColor
            truncationMode: TruncationMode.Fade
        }
    }
}
