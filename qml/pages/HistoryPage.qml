import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"
import "../components/Formatting.js" as Formatting

// What has been sent and received.
//
// Its real job is answering "where did that file go", which is why a received
// entry carries its folder and offers to open it rather than just naming a
// count.
Page {
    id: page

    objectName: "historyPage"
    allowedOrientations: defaultAllowedOrientations

    SilicaListView {
        id: historyList
        anchors.fill: parent
        model: historyModel

        PullDownMenu {
            MenuItem {
                text: qsTr("Clear history")
                visible: historyModel.count > 0
                onClicked: remorse.execute(qsTr("Clearing history"), function () {
                    historyModel.clear()
                })
            }
        }

        header: PageHeader {
            title: qsTr("History")
            description: historyModel.count > 0
                         ? qsTr("%n transfer(s)", "", historyModel.count) : ""
        }

        delegate: ListItem {
            id: record
            width: historyList.width
            contentHeight: Theme.itemSizeLarge

            readonly property bool received: model.direction === "receive"
            readonly property bool ok: model.status === "finished"

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Open folder")
                    visible: record.received && model.destination.length > 0
                    onClicked: Qt.openUrlExternally(Formatting.fileUrl(model.destination))
                }
                MenuItem {
                    text: qsTr("Remove")
                    onClicked: record.remorseAction(qsTr("Removing"), function () {
                        historyModel.removeAt(index)
                    })
                }
            }

            DeviceBadge {
                id: badge
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.itemSizeSmall
                deviceType: model.peerDeviceType
                fingerprint: model.peerAlias
            }

            // Direction, as an arrow rather than a word: it is scanned, not
            // read, and it has to survive being translated into anything.
            Item {
                id: arrow
                anchors {
                    left: badge.right
                    leftMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.iconSizeSmall * 0.6
                height: width

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width
                    height: Math.max(1, parent.width / 8)
                    radius: height / 2
                    color: record.ok ? Theme.highlightColor : Theme.errorColor
                }
                Rectangle {
                    x: record.received ? 0 : parent.width - width
                    y: parent.height / 2 - height / 2
                    width: parent.width * 0.5
                    height: Math.max(1, parent.width / 8)
                    radius: height / 2
                    color: record.ok ? Theme.highlightColor : Theme.errorColor
                    rotation: record.received ? -35 : 35
                    transformOrigin: record.received ? Item.Left : Item.Right
                }
                Rectangle {
                    x: record.received ? 0 : parent.width - width
                    y: parent.height / 2 - height / 2
                    width: parent.width * 0.5
                    height: Math.max(1, parent.width / 8)
                    radius: height / 2
                    color: record.ok ? Theme.highlightColor : Theme.errorColor
                    rotation: record.received ? 35 : -35
                    transformOrigin: record.received ? Item.Left : Item.Right
                }
            }

            Column {
                anchors {
                    left: arrow.right
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                spacing: Theme.paddingSmall / 2

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeSmall
                    color: record.highlighted ? Theme.highlightColor : Theme.primaryColor
                    text: record.received ? qsTr("From %1").arg(model.peerAlias)
                                          : qsTr("To %1").arg(model.peerAlias)
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    text: {
                        var names = model.fileNames
                        var lead = names && names.length > 0 ? names[0] : ""
                        if (names && names.length > 1)
                            lead += " " + qsTr("+%1 more").arg(names.length - 1)
                        return lead + " · " + Formatting.fileSize(model.totalBytes)
                    }
                }

                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeTiny
                    color: record.ok ? Theme.secondaryColor : Theme.errorColor
                    text: {
                        var when = Format.formatDate(model.timestamp, Formatter.DurationElapsed)
                        return record.ok ? when : when + " · " + qsTr("incomplete")
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: historyModel.count === 0
            text: qsTr("Nothing yet")
            hintText: qsTr("Transfers you send and receive will be listed here.")
        }

        VerticalScrollDecorator {}
    }

    RemorsePopup { id: remorse }
}
