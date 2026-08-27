import QtQuick 2.0
import Sailfish.Silica 1.0

// The shape of a device, drawn rather than iconified.
//
// Silica's icon set has no vocabulary for "the laptop over there" versus "the
// phone over there", and the distinction is the one thing a person scanning a
// device list actually needs. So each protocol deviceType gets an outline of
// its own: five shapes, built from rectangles so they cost nothing to draw and
// stay crisp at any size.
Item {
    id: glyph

    property string deviceType: "desktop"
    property color color: Theme.primaryColor

    // Thin enough to read as a line drawing, never below one pixel.
    readonly property real stroke: Math.max(1, height / 20)
    readonly property real unit: Math.min(width, height)

    // --- mobile: a phone standing up ------------------------------------

    Rectangle {
        visible: glyph.deviceType === "mobile"
        anchors.centerIn: parent
        width: glyph.unit * 0.54
        height: glyph.unit * 0.88
        radius: glyph.unit * 0.11
        color: "transparent"
        border.width: glyph.stroke
        border.color: glyph.color

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: glyph.unit * 0.07
            width: parent.width * 0.42
            height: glyph.stroke
            radius: height / 2
            color: glyph.color
        }
    }

    // --- desktop: a screen on a stand ------------------------------------

    Item {
        visible: glyph.deviceType === "desktop"
        anchors.fill: parent

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: (glyph.height - glyph.unit * 0.78) / 2
            width: glyph.unit * 0.86
            height: glyph.unit * 0.58
            radius: glyph.unit * 0.05
            color: "transparent"
            border.width: glyph.stroke
            border.color: glyph.color
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: (glyph.height - glyph.unit * 0.78) / 2 + glyph.unit * 0.68
            width: glyph.unit * 0.44
            height: glyph.stroke * 1.6
            radius: height / 2
            color: glyph.color
        }
    }

    // --- web: a globe, as two crossed outlines ---------------------------

    Item {
        visible: glyph.deviceType === "web"
        anchors.fill: parent

        Rectangle {
            anchors.centerIn: parent
            width: glyph.unit * 0.82
            height: width
            radius: width / 2
            color: "transparent"
            border.width: glyph.stroke
            border.color: glyph.color
        }

        // The meridian: a tall stadium reads as an ellipse at this size.
        Rectangle {
            anchors.centerIn: parent
            width: glyph.unit * 0.36
            height: glyph.unit * 0.82
            radius: width / 2
            color: "transparent"
            border.width: glyph.stroke
            border.color: glyph.color
        }

        Rectangle {
            anchors.centerIn: parent
            width: glyph.unit * 0.82
            height: glyph.stroke
            color: glyph.color
        }
    }

    // --- headless: a terminal with a prompt -------------------------------

    Item {
        visible: glyph.deviceType === "headless"
        anchors.fill: parent

        Rectangle {
            anchors.centerIn: parent
            width: glyph.unit * 0.86
            height: glyph.unit * 0.68
            radius: glyph.unit * 0.05
            color: "transparent"
            border.width: glyph.stroke
            border.color: glyph.color
        }

        // A prompt chevron: two bars pivoting about a shared right end, so
        // they meet in a point rather than running parallel.
        Rectangle {
            x: glyph.width / 2 - glyph.unit * 0.26
            y: glyph.height / 2 - glyph.stroke / 2
            width: glyph.unit * 0.19
            height: glyph.stroke
            radius: height / 2
            color: glyph.color
            rotation: 40
            transformOrigin: Item.Right
        }
        Rectangle {
            x: glyph.width / 2 - glyph.unit * 0.26
            y: glyph.height / 2 - glyph.stroke / 2
            width: glyph.unit * 0.19
            height: glyph.stroke
            radius: height / 2
            color: glyph.color
            rotation: -40
            transformOrigin: Item.Right
        }
        Rectangle {
            x: glyph.width / 2 + glyph.unit * 0.02
            y: glyph.height / 2 + glyph.unit * 0.09
            width: glyph.unit * 0.20
            height: glyph.stroke
            radius: height / 2
            color: glyph.color
        }
    }

    // --- server: stacked units --------------------------------------------

    Column {
        visible: glyph.deviceType === "server"
        anchors.centerIn: parent
        spacing: glyph.unit * 0.08

        Repeater {
            model: 3

            Rectangle {
                width: glyph.unit * 0.80
                height: glyph.unit * 0.22
                radius: glyph.unit * 0.04
                color: "transparent"
                border.width: glyph.stroke
                border.color: glyph.color

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: glyph.unit * 0.07
                    width: glyph.stroke * 1.6
                    height: width
                    radius: width / 2
                    color: glyph.color
                }
            }
        }
    }

    // --- anything else: a neutral box --------------------------------------

    Rectangle {
        visible: glyph.deviceType !== "mobile" && glyph.deviceType !== "desktop"
                 && glyph.deviceType !== "web" && glyph.deviceType !== "headless"
                 && glyph.deviceType !== "server"
        anchors.centerIn: parent
        width: glyph.unit * 0.74
        height: width
        radius: glyph.unit * 0.08
        color: "transparent"
        border.width: glyph.stroke
        border.color: glyph.color
    }
}
