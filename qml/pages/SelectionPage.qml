import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import "../components/Formatting.js" as Formatting

// The staged files, in full.
//
// Reachable by tapping the tray, which is where somebody goes when they are
// no longer sure what is in it - usually to take one thing back out.
Page {
    id: page

    objectName: "selectionPage"
    allowedOrientations: defaultAllowedOrientations

    SilicaListView {
        id: fileList
        anchors.fill: parent
        model: selection

        PullDownMenu {
            MenuItem {
                text: qsTr("Clear all")
                onClicked: {
                    selection.clear()
                    pageStack.pop()
                }
            }
            MenuItem {
                text: qsTr("Add more files")
                onClicked: pageStack.push(filePickerComponent)
            }
        }

        header: PageHeader {
            title: qsTr("Ready to send")
            description: qsTr("%n file(s)", "", selection.count)
                         + " · " + Formatting.fileSize(selection.totalBytes)
        }

        delegate: ListItem {
            id: fileItem
            width: fileList.width
            contentHeight: Theme.itemSizeSmall

            // Removal is a remorse action: the list is a decision in progress
            // and undoing a mis-tap should not mean opening the picker again.
            function removeFile() {
                remorseAction(qsTr("Removing"), function () {
                    selection.removeAt(index)
                })
            }

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Remove")
                    onClicked: fileItem.removeFile()
                }
            }

            Column {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                spacing: Theme.paddingSmall / 2

                Label {
                    width: parent.width
                    text: model.fileName
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeSmall
                    color: fileItem.highlighted ? Theme.highlightColor
                                                : Theme.primaryColor
                }

                Label {
                    width: parent.width
                    text: Formatting.fileSize(model.fileSize)
                          + (model.fileType.length > 0 ? " · " + model.fileType : "")
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
            }
        }

        ViewPlaceholder {
            enabled: selection.count === 0
            text: qsTr("Nothing staged")
            hintText: qsTr("Pull down to add files")
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: filePickerComponent

        MultiContentPickerDialog {
            title: qsTr("Select files to send")

            onAccepted: {
                var paths = []
                for (var i = 0; i < selectedContent.count; i++) {
                    var item = selectedContent.get(i)
                    var path = item.filePath
                    if (!path || path.length === 0)
                        path = item.url
                    if (path && path.length > 0)
                        paths.push("" + path)
                }
                selection.addAll(paths)
            }
        }
    }
}
