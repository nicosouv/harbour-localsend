import QtQuick 2.0
import Sailfish.Silica 1.0

// Rings going out from our own device, for as long as we are announcing.
//
// It is the only honest answer to "is this thing doing anything?" on a screen
// that is otherwise empty while nothing has replied yet. When announcing
// stops the rings stop with it, so a still picture always means a silent
// radio rather than a bug.
Item {
    id: pulse

    property bool active: true
    property color color: Theme.highlightColor
    property int ringCount: 3
    property int period: 3200

    // One animation for the whole set, with each ring reading it at a fixed
    // offset. Separate looping animations drift apart within a minute.
    property real cycle: 0

    NumberAnimation on cycle {
        running: pulse.active && Qt.application.active
        loops: Animation.Infinite
        from: 0
        to: 1
        duration: pulse.period
    }

    Repeater {
        model: pulse.ringCount

        Rectangle {
            // Rings expand and fade; the eased radius keeps them bunched near
            // the centre and slow at the edge, which reads as travelling.
            readonly property real phase: (pulse.cycle + index / pulse.ringCount) % 1
            readonly property real eased: 1 - Math.pow(1 - phase, 3)

            anchors.centerIn: parent
            width: pulse.width * (0.26 + 0.74 * eased)
            height: width
            radius: width / 2
            color: "transparent"
            border.width: Math.max(1, pulse.width / 110)
            border.color: Theme.rgba(pulse.color, 0.45 * (1 - phase))
            visible: pulse.active
        }
    }

    // The still ring, so the centre never looks unanchored between pulses.
    Rectangle {
        anchors.centerIn: parent
        width: pulse.width * 0.26
        height: width
        radius: width / 2
        color: "transparent"
        border.width: Math.max(1, pulse.width / 110)
        border.color: Theme.rgba(pulse.color, pulse.active ? 0.3 : 0.15)
    }
}
