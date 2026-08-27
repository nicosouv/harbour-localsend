import QtQuick 2.0
import Sailfish.Silica 1.0

// A circular progress indicator with room in the middle.
//
// A ring rather than a bar because the middle is the useful part: percentage,
// speed and time remaining all belong next to each other, and a bar leaves
// them scattered around it.
Item {
    id: ring

    property real value: 0                  // 0..1
    property bool indeterminate: false
    property color ringColor: Theme.highlightColor
    property color trackColor: Theme.rgba(Theme.primaryColor, 0.15)
    property real thickness: Math.max(2, width / 26)

    // No default-property alias to a middle Item: in the file that declares
    // it, the alias captures this file's own children too, and the container
    // ends up inside itself. Callers anchor their own content to the ring,
    // which is one line at each call site and cannot go wrong.

    width: Theme.itemSizeExtraLarge * 2
    height: width

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative

        // The whole canvas turns while indeterminate, which costs one
        // transform rather than a repaint per frame.
        rotation: 0
        RotationAnimation on rotation {
            running: ring.indeterminate && Qt.application.active
            loops: Animation.Infinite
            from: 0
            to: 360
            duration: 1400
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()

            var centreX = width / 2
            var centreY = height / 2
            var radius = Math.min(width, height) / 2 - ring.thickness / 2
            if (radius <= 0)
                return

            ctx.lineWidth = ring.thickness
            ctx.lineCap = "round"

            ctx.strokeStyle = ring.trackColor
            ctx.beginPath()
            ctx.arc(centreX, centreY, radius, 0, 2 * Math.PI)
            ctx.stroke()

            // Twelve o'clock is where a progress ring has to start; the
            // canvas's own zero is at three.
            var start = -Math.PI / 2
            var sweep = ring.indeterminate
                ? Math.PI * 0.55
                : 2 * Math.PI * Math.max(0, Math.min(1, ring.value))

            if (sweep <= 0)
                return

            ctx.strokeStyle = ring.ringColor
            ctx.beginPath()
            ctx.arc(centreX, centreY, radius, start, start + sweep)
            ctx.stroke()
        }
    }

    // Canvas repaints on none of these by itself.
    onValueChanged: canvas.requestPaint()
    onIndeterminateChanged: canvas.requestPaint()
    onRingColorChanged: canvas.requestPaint()
    onTrackColorChanged: canvas.requestPaint()
    onThicknessChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
}
