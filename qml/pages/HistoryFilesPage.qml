import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components/Formatting.js" as Formatting

// The files of one received transfer, each a tap away from its app.
//
// Reached from the history when a transfer brought several files, or when its
// one file is gone: a tap that silently did nothing would leave the person
// wondering whether the app or the file was at fault.
Page {
    id: page

    // From HistoryModel.filesAt(): { name, path, available } per file.
    property var files: []
    property string peerAlias: ""
    property string destination: ""
    property var timestamp

    objectName: "historyFilesPage"
    allowedOrientations: defaultAllowedOrientations

    SilicaListView {
        id: fileList
        anchors.fill: parent
        model: page.files

        header: Column {
            width: fileList.width

            PageHeader {
                title: qsTr("From %1").arg(page.peerAlias)
                description: page.timestamp
                             ? Format.formatDate(page.timestamp, Formatter.DurationElapsed)
                             : ""
            }

            // The folder, spelled out: the download location is configurable,
            // and this is the one place left that names it.
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                visible: page.destination.length > 0
                text: qsTr("in %1").arg(page.destination)
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }

        delegate: ListItem {
            id: fileItem

            readonly property bool available: modelData.available

            width: fileList.width
            contentHeight: Theme.itemSizeSmall
            highlighted: available && down

            onClicked: {
                if (available)
                    Qt.openUrlExternally(Formatting.fileUrl(modelData.path))
            }

            Column {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                spacing: Theme.paddingSmall / 2

                Label {
                    width: parent.width
                    text: modelData.name
                    textFormat: Text.PlainText
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeSmall
                    color: !fileItem.available ? Theme.secondaryColor
                         : (fileItem.highlighted ? Theme.highlightColor : Theme.primaryColor)
                }

                Label {
                    width: parent.width
                    visible: !fileItem.available
                    text: qsTr("Moved or deleted")
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
