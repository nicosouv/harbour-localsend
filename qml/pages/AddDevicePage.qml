import QtQuick 2.0
import Sailfish.Silica 1.0

// Adding a device by address, for the ones nothing can find on its own.
//
// Multicast never leaves the broadcast domain and the sweep only covers our
// own /24, so a device on another subnet, behind a VPN, or on a guest network
// with client isolation is unreachable however long anybody waits. Typing its
// address is the only way in, and it is remembered afterwards because a
// device that had to be added by hand would have to be added again on every
// launch otherwise.
//
// A page rather than a Dialog: the lookup is a round trip that can fail, and
// a Dialog would have to close before knowing whether it worked.
Page {
    id: page

    objectName: "addDevicePage"
    allowedOrientations: defaultAllowedOrientations

    // "", "found", "missing" — drives the one line of feedback below.
    property string outcome: ""
    property string foundAlias: ""

    function lookUp() {
        if (addressField.text.trim().length === 0)
            return
        page.outcome = ""
        page.foundAlias = ""
        addressField.focus = false
        discovery.addDeviceAt(addressField.text.trim(),
                              parseInt(portField.text, 10))
    }

    Connections {
        target: discovery
        onManualLookupFinished: {
            page.outcome = found ? "found" : "missing"
            page.foundAlias = peerAlias
            if (found)
                addressField.text = ""
        }
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: appSettings.manualDevices

        header: Column {
            width: listView.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Add by address") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("For a device on another network, behind a VPN, or on a Wi-Fi that keeps clients apart. Both plain and encrypted transports are tried.")
            }

            TextField {
                id: addressField
                width: parent.width
                label: qsTr("Address")
                // Not translated: an IPv4 example reads the same everywhere.
                placeholderText: "192.168.1.42"
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                errorHighlight: page.outcome === "missing"
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: page.lookUp()
            }

            TextField {
                id: portField
                width: parent.width
                label: qsTr("Port")
                text: "53317"
                placeholderText: "53317"
                inputMethodHints: Qt.ImhDigitsOnly
                description: qsTr("Leave this alone unless the other device was moved off the standard port.")
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: page.lookUp()
            }

            Item {
                width: parent.width
                height: Theme.itemSizeSmall

                BusyIndicator {
                    anchors.centerIn: parent
                    size: BusyIndicatorSize.Small
                    running: discovery.manualLookupBusy
                }

                Label {
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    visible: !discovery.manualLookupBusy && page.outcome.length > 0
                    font.pixelSize: Theme.fontSizeSmall
                    color: page.outcome === "found" ? Theme.highlightColor
                                                    : Theme.errorColor
                    text: page.outcome === "found"
                        ? qsTr("Found %1").arg(page.foundAlias)
                        : qsTr("Nothing answered at that address")
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Look for it")
                enabled: !discovery.manualLookupBusy
                         && addressField.text.trim().length > 0
                onClicked: page.lookUp()
            }

            SectionHeader {
                text: qsTr("Remembered addresses")
                visible: appSettings.manualDevices.length > 0
            }
        }

        delegate: ListItem {
            id: entry
            width: listView.width

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Forget")
                    onClicked: entry.remorseAction(qsTr("Forgetting"), function () {
                        appSettings.removeManualDevice(modelData)
                    })
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                anchors.verticalCenter: parent.verticalCenter
                text: modelData
                truncationMode: TruncationMode.Fade
                color: entry.highlighted ? Theme.highlightColor : Theme.primaryColor
            }
        }

        VerticalScrollDecorator {}
    }
}
