import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    objectName: "aboutPage"
    allowedOrientations: defaultAllowedOrientations

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: about.height

        Column {
            id: about
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("About") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("An unofficial LocalSend client for Sailfish OS. Files go straight from one device to the other over your own network — no account, no server, no Internet connection needed.")
            }

            Item { width: 1; height: Theme.paddingMedium }

            DetailItem {
                label: qsTr("Version")
                value: appSettings.appVersion
            }
            DetailItem {
                label: qsTr("Protocol")
                value: qsTr("LocalSend v%1").arg(appSettings.protocolVersion)
            }
            DetailItem {
                label: qsTr("This device")
                value: appSettings.deviceModel
            }
            DetailItem {
                label: qsTr("Fingerprint")
                value: appSettings.fingerprint.substring(0, 16)
            }

            SectionHeader { text: qsTr("Good to know") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                // Worth stating plainly rather than burying: somebody sending
                // something sensitive deserves to know what the wire looks
                // like, and "LocalSend" elsewhere defaults to encrypted.
                text: qsTr("Transfers use plain HTTP on port %1. The encrypted transport that the desktop and mobile apps offer is not implemented here yet, so treat a transfer as visible to anyone who can watch the network. On a home or personal hotspot that is nobody; on café or office Wi-Fi it may not be.").arg(appSettings.port)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Devices find each other with multicast. Plenty of networks block it — guest Wi-Fi almost always does. When that happens, Scan network on the main page finds them the slow way instead.")
            }

            SectionHeader { text: qsTr("Links") }

            BackgroundItem {
                width: parent.width
                onClicked: Qt.openUrlExternally("https://github.com/nicosouv/harbour-localsend")

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Source and issues")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
            }

            BackgroundItem {
                width: parent.width
                onClicked: Qt.openUrlExternally("https://localsend.org")

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("The LocalSend project")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeTiny
                color: Theme.secondaryColor
                text: qsTr("Not affiliated with the LocalSend project. Released under the MIT licence.")
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator {}
    }
}
