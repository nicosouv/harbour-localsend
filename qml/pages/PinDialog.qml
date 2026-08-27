import QtQuick 2.0
import Sailfish.Silica 1.0

// The other device is asking for a PIN before it will even show its owner
// the request.
Dialog {
    id: dialog

    objectName: "pinDialog"
    allowedOrientations: defaultAllowedOrientations

    // True when a code has already been tried and refused.
    property bool retry: false
    property string peerAlias: ""

    canAccept: pinField.text.length > 0

    onAccepted: sender.submitPin(pinField.text)
    onRejected: sender.cancel()

    Column {
        width: parent.width

        DialogHeader {
            title: qsTr("PIN required")
            acceptText: qsTr("Send")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - Theme.horizontalPageMargin * 2
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: dialog.retry ? Theme.errorColor : Theme.secondaryColor
            text: dialog.retry
                  ? qsTr("That code was not accepted. Try again.")
                  : qsTr("%1 is asking for a PIN before accepting files.")
                    .arg(dialog.peerAlias)
        }

        TextField {
            id: pinField
            width: parent.width
            label: qsTr("PIN")
            placeholderText: qsTr("PIN")
            inputMethodHints: Qt.ImhDigitsOnly
            focus: true
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: dialog.accept()
        }
    }
}
