import QtQuick 2.0
import Sailfish.Silica 1.0
import "Formatting.js" as Formatting

// The staged files, docked at the bottom of the main page.
//
// It exists so that picking files and picking a device are independent
// decisions. Without it the only possible order is device first, which is the
// wrong one whenever somebody opens the app already knowing what they want to
// send and not yet knowing whether the other machine is even awake.
DockedPanel {
    id: tray

    signal reviewRequested()
    signal clearRequested()

    width: parent.width
    height: content.height + Theme.paddingLarge * 2
    dock: Dock.Bottom

    // Driven entirely by the model: staging a file opens it, clearing the
    // last one puts it away.
    open: !selection.empty

    Rectangle {
        anchors.fill: parent
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.22)

        Rectangle {
            width: parent.width
            height: Math.max(1, Theme.paddingSmall / 4)
            color: Theme.rgba(Theme.highlightColor, 0.35)
        }
    }

    Row {
        id: content
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        spacing: Theme.paddingMedium

        BackgroundItem {
            width: parent.width - clearButton.width - Theme.paddingMedium
            height: summary.height + Theme.paddingMedium * 2
            onClicked: tray.reviewRequested()

            Column {
                id: summary
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                spacing: Theme.paddingSmall / 2

                Label {
                    text: qsTr("%n file(s) ready", "", selection.count)
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.highlightColor
                }

                Label {
                    text: Formatting.fileSize(selection.totalBytes)
                          + " · " + qsTr("pick a device to send")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryHighlightColor
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }
            }
        }

        IconButton {
            id: clearButton
            anchors.verticalCenter: parent.verticalCenter
            icon.source: "image://theme/icon-m-clear"
            onClicked: tray.clearRequested()
        }
    }
}
