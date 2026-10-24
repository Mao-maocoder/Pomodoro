import QtQuick
import QtQuick.Window

Item {
    id: dial
    property real remaining: 1
    property bool running: false
    readonly property real elapsed: Math.max(0, Math.min(1, 1 - remaining))
    property real displayedElapsed: elapsed

    // All curves use a 300 x 300 design space, starting below the stem.
    readonly property var curves: [
        [150, 62, 177, 45, 236, 55, 254, 102],
        [254, 102, 277, 123, 289, 152, 285, 184],
        [285, 184, 280, 248, 226, 294, 172, 284],
        [172, 284, 160, 282, 156, 278, 150, 278],
        [150, 278, 144, 278, 140, 282, 128, 284],
        [128, 284, 74, 294, 20, 248, 15, 184],
        [15, 184, 11, 152, 23, 123, 46, 102],
        [46, 102, 64, 55, 123, 45, 150, 62]
    ]
    readonly property var outline: sampleOutline()

    Behavior on displayedElapsed {
        enabled: dial.running
        NumberAnimation { duration: 250; easing.type: Easing.Linear }
    }
    onDisplayedElapsedChanged: progress.requestPaint()

    function traceBody(ctx) {
        ctx.beginPath()
        ctx.moveTo(curves[0][0], curves[0][1])
        for (let i = 0; i < curves.length; ++i) {
            const c = curves[i]
            ctx.bezierCurveTo(c[2], c[3], c[4], c[5], c[6], c[7])
        }
        ctx.closePath()
    }

    function sampleOutline() {
        const points = [{ x: 150, y: 62, distance: 0 }]
        let distance = 0
        for (let i = 0; i < curves.length; ++i) {
            const c = curves[i]
            for (let step = 1; step <= 40; ++step) {
                const t = step / 40
                const u = 1 - t
                const x = u*u*u*c[0] + 3*u*u*t*c[2] + 3*u*t*t*c[4] + t*t*t*c[6]
                const y = u*u*u*c[1] + 3*u*u*t*c[3] + 3*u*t*t*c[5] + t*t*t*c[7]
                const previous = points[points.length - 1]
                const dx = x - previous.x
                const dy = y - previous.y
                distance += Math.sqrt(dx*dx + dy*dy)
                points.push({ x: x, y: y, distance: distance })
            }
        }
        return { points: points, length: distance }
    }

    function prepareContext(canvas) {
        const ctx = canvas.getContext("2d")
        ctx.reset()
        ctx.scale(canvas.width / 300, canvas.height / 300)
        return ctx
    }

    function repaintAll() {
        body.requestPaint()
        progress.requestPaint()
        leaves.requestPaint()
    }
    onWidthChanged: repaintAll()
    onHeightChanged: repaintAll()
    Connections {
        target: dial.Window.window
        function onVisibleChanged() {
            if (dial.Window.window.visible) Qt.callLater(dial.repaintAll)
        }
    }

    Canvas {
        id: body
        anchors.fill: parent
        contextType: "2d"
        onAvailableChanged: if (available) requestPaint()
        onPaint: {
            const ctx = dial.prepareContext(body)
            const fill = ctx.createRadialGradient(112, 128, 10, 150, 172, 155)
            fill.addColorStop(0, "#FFF0E3")
            fill.addColorStop(0.65, "#FDE6D6")
            fill.addColorStop(1, "#F9DCCB")
            dial.traceBody(ctx)
            ctx.fillStyle = fill
            ctx.fill()

            // Soft painted reflection on the upper-left shoulder.
            ctx.beginPath()
            ctx.moveTo(48, 129)
            ctx.bezierCurveTo(54, 114, 63, 103, 77, 94)
            ctx.strokeStyle = "#FFF7ED"
            ctx.lineWidth = 11
            ctx.lineCap = "round"
            ctx.stroke()
        }
    }

    Canvas {
        id: progress
        objectName: "progressRing"
        anchors.fill: parent
        contextType: "2d"
        onAvailableChanged: if (available) requestPaint()
        onPaint: {
            const ctx = dial.prepareContext(progress)
            const outline = dial.outline
            const points = outline.points
            ctx.lineWidth = 8.5
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            dial.traceBody(ctx)
            ctx.strokeStyle = "#EAC8B8"
            ctx.stroke()

            // Arc length, rather than curve index, keeps the flow speed even.
            const travelled = dial.displayedElapsed * outline.length
            if (dial.displayedElapsed >= 1)
                return
            ctx.beginPath()
            let started = false
            for (let i = 1; i < points.length; ++i) {
                const a = points[i - 1]
                const b = points[i]
                if (!started && b.distance >= travelled) {
                    const t = (travelled - a.distance) / (b.distance - a.distance)
                    ctx.moveTo(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t)
                    started = true
                }
                if (started)
                    ctx.lineTo(b.x, b.y)
            }
            ctx.strokeStyle = "#D96B58"
            ctx.stroke()
        }
    }

    Canvas {
        id: leaves
        anchors.fill: parent
        contextType: "2d"
        onAvailableChanged: if (available) requestPaint()
        onPaint: {
            const ctx = dial.prepareContext(leaves)
            ctx.beginPath()
            ctx.moveTo(143, 60)
            ctx.bezierCurveTo(142, 44, 152, 13, 162, 12)
            ctx.bezierCurveTo(171, 9, 177, 16, 174, 21)
            ctx.bezierCurveTo(163, 34, 160, 46, 160, 53)
            ctx.bezierCurveTo(182, 42, 199, 44, 213, 44)
            ctx.bezierCurveTo(216, 44, 217, 40, 216, 48)
            ctx.bezierCurveTo(209, 57, 189, 59, 179, 61)
            ctx.bezierCurveTo(186, 68, 190, 76, 193, 84)
            ctx.bezierCurveTo(178, 79, 165, 69, 150, 68)
            ctx.bezierCurveTo(135, 71, 119, 79, 109, 85)
            ctx.bezierCurveTo(110, 74, 116, 64, 125, 59)
            ctx.bezierCurveTo(112, 55, 101, 52, 91, 49)
            ctx.bezierCurveTo(87, 50, 87, 46, 90, 44)
            ctx.bezierCurveTo(107, 37, 127, 41, 143, 50)
            ctx.closePath()
            const green = ctx.createLinearGradient(100, 18, 190, 88)
            green.addColorStop(0, "#91A57C")
            green.addColorStop(1, "#7D946B")
            ctx.fillStyle = green
            ctx.fill()
        }
    }
}
