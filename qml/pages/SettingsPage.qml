import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Page {
    id: page

    objectName: "settingsPage"
    allowedOrientations: defaultAllowedOrientations

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
                onClicked: appSettings.pinEnabled = checked
            }

            TextField {
                width: parent.width
                visible: appSettings.pinEnabled
                text: appSettings.pin
                label: qsTr("PIN")
                placeholderText: qsTr("4 to 8 digits")
                inputMethodHints: Qt.ImhDigitsOnly
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                onActiveFocusChanged: {
                    if (!activeFocus)
                        appSettings.pin = text
                }
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
