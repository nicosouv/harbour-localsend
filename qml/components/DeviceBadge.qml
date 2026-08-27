import QtQuick 2.0
import Sailfish.Silica 1.0
import "DeviceLook.js" as DeviceLook

// A device's avatar: its own colour, its own shape.
//
// The colour comes from the fingerprint, so it is the same on every screen
// and every launch, and two devices with near-identical aliases still look
// nothing alike.
Item {
    id: badge

    property string fingerprint: ""
    property string deviceType: "desktop"
    property bool stale: false
    // Overrides the fingerprint colour; used where the accent is fixed, such
    // as our own device on the main page.
    property color accent: fingerprint.length > 0
        ? DeviceLook.accent(fingerprint, true)
        : Theme.highlightColor

    width: Theme.itemSizeSmall
    height: width

    opacity: stale ? 0.4 : 1.0
    Behavior on opacity { FadeAnimation { duration: 300 } }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.rgba(badge.accent, 0.16)
        border.width: Math.max(1, width / 40)
        border.color: Theme.rgba(badge.accent, 0.45)
    }

    DeviceGlyph {
        anchors.centerIn: parent
        width: parent.width * 0.5
        height: width
        deviceType: badge.deviceType
        color: badge.accent
    }
}
