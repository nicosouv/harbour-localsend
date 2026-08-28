import QtQuick 2.0
import Sailfish.Silica 1.0

// One nearby device.
//
// The address sits next to the hardware name rather than behind a detail
// page: on a network with two identical laptops it is the only thing telling
// them apart, and it is the first thing anybody debugging a firewall wants.
ListItem {
    id: delegate

    // Not "alias": that word means something else directly after `property`,
    // and a name that only works by accident of position is a trap.
    property string deviceAlias: ""
    property string hardware: ""
    property string deviceType: "desktop"
    property string fingerprint: ""
    property string address: ""
    property bool stale: false
    // We have completed a transfer with this key before.
    property bool known: false
    // This name is on record under a different key. Somebody may be wearing
    // it; the announcement that carries a name is not authenticated.
    property bool conflict: false
    // What a tap does, which the tray decides: with files staged it sends,
    // with an empty tray it opens the picker first.
    property bool sendsImmediately: false

    contentHeight: Theme.itemSizeLarge
    menu: contextMenuComponent

    signal sendRequested()
    signal detailsRequested()
    signal blockRequested()

    onClicked: sendRequested()

    DeviceBadge {
        id: avatar
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.itemSizeSmall
        fingerprint: delegate.fingerprint
        deviceType: delegate.deviceType
        stale: delegate.stale
        conflict: delegate.conflict
    }

    Column {
        anchors {
            left: avatar.right
            leftMargin: Theme.paddingLarge
            right: action.left
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        spacing: Theme.paddingSmall / 2

        Label {
            width: parent.width
            text: delegate.deviceAlias
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
            opacity: delegate.stale ? 0.5 : 1.0
        }

        Label {
            width: parent.width
            text: delegate.hardware.length > 0
                  ? delegate.hardware + " · " + delegate.address
                  : delegate.address
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: delegate.highlighted ? Theme.secondaryHighlightColor
                                        : Theme.secondaryColor
            opacity: delegate.stale ? 0.5 : 1.0
        }

        // Loud on purpose. This is the one thing on the page that says a
        // device may not be who it says it is, and the whole attack it warns
        // about consists of looking exactly like a device you trust.
        Label {
            width: parent.width
            visible: delegate.conflict
            text: qsTr("This name has used a different key before")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.errorColor
        }
    }

    // A quiet chevron rather than a button: the whole row is the target and
    // this only says which way it leads.
    Item {
        id: action
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmall
        height: width

        readonly property color strokeColor: delegate.highlighted
            ? Theme.highlightColor : Theme.secondaryColor
        readonly property real barLength: width * 0.44
        readonly property real barThickness: Math.max(1, width / 12)

        // Both bars pivot about their right end, which sits on the centre
        // line, so they meet in a point instead of running parallel.
        Rectangle {
            x: parent.width * 0.28
            y: (parent.height - height) / 2
            width: parent.barLength
            height: parent.barThickness
            radius: height / 2
            color: parent.strokeColor
            rotation: 45
            transformOrigin: Item.Right
        }
        Rectangle {
            x: parent.width * 0.28
            y: (parent.height - height) / 2
            width: parent.barLength
            height: parent.barThickness
            radius: height / 2
            color: parent.strokeColor
            rotation: -45
            transformOrigin: Item.Right
        }
    }

    Component {
        id: contextMenuComponent

        ContextMenu {
            MenuItem {
                text: delegate.sendsImmediately ? qsTr("Send staged files")
                                                : qsTr("Choose files to send")
                onClicked: delegate.sendRequested()
            }
            MenuItem {
                text: qsTr("Device details")
                onClicked: delegate.detailsRequested()
            }
            MenuItem {
                text: qsTr("Block this device")
                onClicked: delegate.blockRequested()
            }
        }
    }
}
