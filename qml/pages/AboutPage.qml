import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components/Formatting.js" as Formatting

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
                label: qsTr("Transport")
                value: appSettings.encrypted ? qsTr("HTTPS (encrypted)")
                                             : qsTr("HTTP (not encrypted)")
            }
            DetailItem {
                label: qsTr("Stored data")
                value: appSettings.storageEncrypted ? qsTr("Encrypted with a stored key")
                                                    : qsTr("Owner-only files")
            }
            SectionHeader { text: qsTr("Fingerprint") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("What other devices know this one by. Read it out to somebody to let them confirm it really is you they are sending to.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                font.pixelSize: Theme.fontSizeExtraSmall
                font.family: "monospace"
                color: Theme.highlightColor
                text: Formatting.fingerprintGroups(appSettings.fingerprint)
            }

            SectionHeader { text: qsTr("Good to know") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                // The honest description of what the encryption does and does
                // not buy. Somebody sending something sensitive deserves the
                // real shape of it rather than the word "encrypted".
                text: appSettings.encrypted
                    ? qsTr("Transfers are encrypted between the two devices with a certificate this phone generated for itself. There is no certificate authority on a local network, so what identifies a device is the fingerprint above: it travels in every announcement, and a device presenting anything else is refused before a single byte is sent.")
                    : qsTr("Encryption is off, so transfers use plain HTTP on port %1 and are readable by anyone who can watch the network. On a home network or your own hotspot that is nobody; on café or office Wi-Fi it may not be.").arg(appSettings.port)
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

            // The heart is written out rather than drawn: the tiny font here
            // is the theme's, and there is no promise it carries U+2665.
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.highlightColor
                text: qsTr("Made with <3 for Sailfish OS")
                textFormat: Text.PlainText
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator {}
    }
}
