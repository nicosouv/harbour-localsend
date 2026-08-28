import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Page {
    id: page

    objectName: "settingsPage"
    allowedOrientations: defaultAllowedOrientations

    // A function call in a binding is evaluated once; the count would go stale
    // the moment something was unblocked on the page this one pushes.
    property int blockedCount: knownDevices.blocked().length

    Connections {
        target: knownDevices
        onChanged: page.blockedCount = knownDevices.blocked().length
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: settings.height

        Column {
            id: settings
            width: parent.width

            PageHeader { title: qsTr("Settings") }

            // --- identity -------------------------------------------------

            SectionHeader { text: qsTr("This device") }

            TextField {
                width: parent.width
                text: appSettings.alias
                label: qsTr("Device name")
                placeholderText: qsTr("Device name")
                description: qsTr("What other devices call you.")
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false

                // Committed on losing focus rather than per keystroke: every
                // change is announced to the network, and a rename typed one
                // letter at a time would announce eleven different devices.
                onActiveFocusChanged: {
                    if (!activeFocus && text.trim().length > 0)
                        appSettings.alias = text.trim()
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Suggest another name")
                onClicked: appSettings.rerollAlias()
            }

            Item { width: 1; height: Theme.paddingLarge }

            // --- transport ---------------------------------------------------

            SectionHeader { text: qsTr("Security") }

            TextSwitch {
                text: qsTr("Encrypt transfers")
                description: appSettings.transportError.length > 0
                    ? qsTr("Unavailable on this device: %1").arg(appSettings.transportError)
                    : qsTr("Files are encrypted between the two devices. Turning this off makes every transfer readable by anyone on the same network.")
                checked: appSettings.secureTransport
                enabled: appSettings.transportError.length === 0
                onClicked: appSettings.secureTransport = checked
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                // Worth saying once here rather than letting somebody wonder
                // why their device vanished from a friend's list.
                text: qsTr("This device is identified by the fingerprint of its certificate, so changing this setting makes it look like a new device to everyone else.")
            }

            ValueButton {
                label: qsTr("Blocked devices")
                value: page.blockedCount > 0
                       ? qsTr("%n device(s)", "", page.blockedCount)
                       : qsTr("None")
                description: qsTr("Refused before you are asked, and hidden from the device list.")
                onClicked: pageStack.push(Qt.resolvedUrl("BlockedDevicesPage.qml"))
            }

            Item { width: 1; height: Theme.paddingLarge }

            // --- receiving -------------------------------------------------

            SectionHeader { text: qsTr("Receiving") }

            TextSwitch {
                text: qsTr("Allow incoming files")
                description: qsTr("When off, this device stops announcing itself and refuses transfers.")
                checked: appSettings.receiveEnabled
                onClicked: appSettings.receiveEnabled = checked
            }

            TextSwitch {
                text: qsTr("Accept without asking")
                description: qsTr("Files are saved as soon as they arrive. Convenient at home, unwise on a network you share.")
                enabled: appSettings.receiveEnabled
                checked: appSettings.quickSave
                onClicked: appSettings.quickSave = checked
            }

            TextSwitch {
                text: qsTr("Require a PIN")
                description: qsTr("Senders must enter this code before you are even asked.")
                enabled: appSettings.receiveEnabled
                checked: appSettings.pinEnabled
                onClicked: {
                    appSettings.pinEnabled = checked
                    // Switching the requirement on without a code would leave
                    // a barrier that lets everything through, so the dialog
                    // follows immediately rather than waiting to be found.
                    if (checked && !appSettings.pinIsSet)
                        pageStack.push(pinSetupComponent)
                }
            }

            ValueButton {
                visible: appSettings.pinEnabled
                label: qsTr("PIN")
                value: appSettings.pinIsSet ? qsTr("Set") : qsTr("Not set")
                description: qsTr("Only a salted hash of the code is stored, so it can be changed but never shown again.")
                onClicked: pageStack.push(pinSetupComponent)
            }

            ValueButton {
                label: qsTr("Port")
                value: appSettings.port
                description: qsTr("53317 is the standard. Change it only if something else is using the port.")
                onClicked: pageStack.push(portDialogComponent)
            }

            Item { width: 1; height: Theme.paddingLarge }

            // --- storage ---------------------------------------------------

            SectionHeader { text: qsTr("Saving") }

            ValueButton {
                label: qsTr("Save to")
                value: appSettings.destination
                onClicked: pageStack.push(folderPickerComponent)
            }

            TextSwitch {
                text: qsTr("A folder per sender")
                description: qsTr("Received files go into a subfolder named after the device that sent them.")
                checked: appSettings.folderPerSender
                onClicked: appSettings.folderPerSender = checked
            }

            Item { width: 1; height: Theme.paddingLarge }

            // --- behaviour --------------------------------------------------

            SectionHeader { text: qsTr("While transferring") }

            TextSwitch {
                text: qsTr("Notify me")
                description: qsTr("A notification when a transfer arrives or finishes in the background.")
                checked: appSettings.notificationsEnabled
                onClicked: appSettings.notificationsEnabled = checked
            }

            TextSwitch {
                text: qsTr("Keep going with the screen off")
                description: qsTr("Stops the device suspending mid-transfer. Uses more battery.")
                checked: appSettings.keepAwake
                onClicked: appSettings.keepAwake = checked
            }

            TextSwitch {
                text: qsTr("Keep a history")
                description: qsTr("Records what was sent and received, and where it was saved.")
                checked: appSettings.historyEnabled
                onClicked: appSettings.historyEnabled = checked
            }

            Item { width: 1; height: Theme.paddingLarge }

            // --- language ---------------------------------------------------

            SectionHeader { text: qsTr("Language") }

            ComboBox {
                width: parent.width
                label: qsTr("Interface language")
                description: qsTr("The app reloads when this changes.")

                // Kept in the same order as the values below; the index is
                // the only thing tying the two together.
                property var codes: ["en", "fr", "de", "es", "fi", "it", "nb_NO"]

                currentIndex: {
                    var index = codes.indexOf(appSettings.language)
                    return index < 0 ? 0 : index
                }

                // The language is set from the items rather than from
                // onCurrentIndexChanged: that handler also fires while the
                // combo is being built, before its binding has settled, and
                // would reset a French install to English on every visit.
                menu: ContextMenu {
                    MenuItem { text: "English";      onClicked: appSettings.language = "en" }
                    MenuItem { text: "Français";     onClicked: appSettings.language = "fr" }
                    MenuItem { text: "Deutsch";      onClicked: appSettings.language = "de" }
                    MenuItem { text: "Español";      onClicked: appSettings.language = "es" }
                    MenuItem { text: "Suomi";        onClicked: appSettings.language = "fi" }
                    MenuItem { text: "Italiano";     onClicked: appSettings.language = "it" }
                    MenuItem { text: "Norsk bokmål"; onClicked: appSettings.language = "nb_NO" }
                }
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: pinSetupComponent

        Dialog {
            id: pinDialog

            canAccept: pinField.text.length >= 4 && pinField.text.length <= 8

            Column {
                width: parent.width

                DialogHeader {
                    title: appSettings.pinIsSet ? qsTr("Change the PIN")
                                                : qsTr("Set a PIN")
                }

                TextField {
                    id: pinField
                    width: parent.width
                    label: qsTr("PIN")
                    placeholderText: qsTr("4 to 8 digits")
                    inputMethodHints: Qt.ImhDigitsOnly
                    echoMode: TextInput.Password
                    focus: true
                    description: qsTr("Stored as a salted hash, so it cannot be shown again — only replaced.")
                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: pinDialog.accept()
                }

                Item { width: 1; height: Theme.paddingLarge }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Remove the PIN")
                    visible: appSettings.pinIsSet
                    onClicked: {
                        appSettings.setPin("")
                        appSettings.pinEnabled = false
                        pageStack.pop()
                    }
                }
            }

            onAccepted: appSettings.setPin(pinField.text)
        }
    }

    Component {
        id: folderPickerComponent

        FolderPickerDialog {
            title: qsTr("Where to save incoming files")
            path: appSettings.destination
            onAccepted: appSettings.destination = selectedPath
        }
    }

    Component {
        id: portDialogComponent

        Dialog {
            id: portDialog
            canAccept: {
                var value = parseInt(portField.text, 10)
                return !isNaN(value) && value >= 1024 && value <= 65535
            }

            Column {
                width: parent.width

                DialogHeader { title: qsTr("Listening port") }

                TextField {
                    id: portField
                    width: parent.width
                    text: appSettings.port
                    label: qsTr("Port")
                    inputMethodHints: Qt.ImhDigitsOnly
                    description: qsTr("Other LocalSend devices look on 53317 by default. A different port still works, but only if the other side is told about it.")
                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: portDialog.accept()
                }
            }

            onAccepted: appSettings.port = parseInt(portField.text, 10)
        }
    }
}
