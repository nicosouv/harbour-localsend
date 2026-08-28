import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"
import "../components/Formatting.js" as Formatting

// The cover has one job: say whether anything is happening without being
// opened. Idle it counts the devices in range; mid-transfer it becomes the
// progress readout, because that is the state somebody actually glances at
// their lock screen for.
CoverBackground {
    id: cover

    readonly property bool busy: transfer.state === "active"
                                 || transfer.state === "requesting"
    readonly property bool waiting: transfer.state === "pending"

    // --- idle --------------------------------------------------------

    Column {
        anchors.centerIn: parent
        width: parent.width - Theme.paddingLarge * 2
        spacing: Theme.paddingMedium
        visible: !cover.busy && !cover.waiting

        RadarPulse {
            anchors.horizontalCenter: parent.horizontalCenter
            width: cover.width * 0.42
            height: width
            active: discovery.running && appSettings.receiveEnabled
            period: 4200

            DeviceBadge {
                anchors.centerIn: parent
                width: parent.width * 0.34
                deviceType: "mobile"
                accent: Theme.highlightColor
            }
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: appSettings.alias
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.primaryColor
            truncationMode: TruncationMode.Fade
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
            text: {
                if (!appSettings.receiveEnabled)
                    return qsTr("Receiving off")
                if (deviceModel.count === 0)
                    return qsTr("No devices")
                return qsTr("%n nearby", "", deviceModel.count)
            }
        }
    }

    // --- an incoming request nobody has answered ----------------------

    Column {
        anchors.centerIn: parent
        width: parent.width - Theme.paddingLarge * 2
        spacing: Theme.paddingMedium
        visible: cover.waiting

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Incoming")
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.highlightColor
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: transfer.peerAlias
            textFormat: Text.PlainText
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.primaryColor
            truncationMode: TruncationMode.Fade
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: qsTr("%n file(s)", "", transfer.fileCount)
        }
    }

    // --- transferring --------------------------------------------------

    Column {
        anchors.centerIn: parent
        width: parent.width - Theme.paddingLarge * 2
        spacing: Theme.paddingMedium
        visible: cover.busy

        ProgressRing {
            anchors.horizontalCenter: parent.horizontalCenter
            width: cover.width * 0.46
            height: width
            value: transfer.progress
            indeterminate: transfer.state === "requesting"
            thickness: Math.max(2, width / 14)

            Label {
                anchors.centerIn: parent
                text: transfer.state === "requesting"
                      ? "…" : Formatting.percent(transfer.progress)
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
            }
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.primaryColor
            truncationMode: TruncationMode.Fade
            text: transfer.direction === "send" ? qsTr("Sending") : qsTr("Receiving")
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            truncationMode: TruncationMode.Fade
            text: transfer.peerAlias
            textFormat: Text.PlainText
        }
    }

    CoverActionList {
        enabled: cover.busy

        CoverAction {
            iconSource: "image://theme/icon-cover-cancel"
            onTriggered: {
                if (transfer.direction === "send")
                    sender.cancel()
                else
                    receiver.cancel()
            }
        }
    }

    CoverActionList {
        enabled: !cover.busy && !cover.waiting

        CoverAction {
            iconSource: "image://theme/icon-cover-refresh"
            onTriggered: discovery.refresh()
        }
    }
}
