import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"
import "../components/Formatting.js" as Formatting

// The devices that are refused before anybody is asked.
//
// This page exists because blocking has to be undoable somewhere, and the
// device list is not that somewhere: a blocked device is not shown there at
// all. Leaving it visible but inert would only invite tapping it.
Page {
    id: page

    objectName: "blockedDevicesPage"
    allowedOrientations: defaultAllowedOrientations

    // knownDevices.blocked() is a plain call rather than a bound property, so
    // the list is refreshed when the store says something changed.
    property var entries: knownDevices.blocked()

    Connections {
        target: knownDevices
        onChanged: page.entries = knownDevices.blocked()
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: page.entries

        header: Column {
            width: listView.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Blocked devices") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Transfers from these are refused without asking, and they are not shown among nearby devices. Blocking follows the key rather than the name, so renaming does not undo it.")
            }

            // The empty state lives in the header rather than in a
            // ViewPlaceholder, which centres itself on the view and would sit
            // on top of the paragraph above.
            Column {
                width: parent.width
                visible: page.entries.length === 0
                spacing: Theme.paddingMedium

                Item {
                    width: 1
                    height: Theme.paddingLarge
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - Theme.horizontalPageMargin * 2
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Nothing blocked")
                    color: Theme.highlightColor
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - Theme.horizontalPageMargin * 2
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    text: qsTr("Block a device from its entry in the list, or when it asks to send you something.")
                }
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }

        delegate: ListItem {
            id: entry
            width: listView.width
            contentHeight: Theme.itemSizeMedium

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Unblock")
                    onClicked: entry.remorseAction(qsTr("Unblocking"), function () {
                        knownDevices.setBlocked(modelData.fingerprint,
                                                modelData.alias, false)
                    })
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
                    text: modelData.alias.length > 0 ? modelData.alias
                                                     : qsTr("Unnamed device")
                    textFormat: Text.PlainText
                    truncationMode: TruncationMode.Fade
                    color: entry.highlighted ? Theme.highlightColor
                                             : Theme.primaryColor
                }

                // The first half of the key, grouped the same way the device
                // details page groups it, so the two can be compared.
                Label {
                    width: parent.width
                    text: Formatting.fingerprintGroups(
                              modelData.fingerprint.substring(0, 16))
                    textFormat: Text.PlainText
                    font.pixelSize: Theme.fontSizeTiny
                    font.family: "monospace"
                    color: entry.highlighted ? Theme.secondaryHighlightColor
                                             : Theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
