import QtQuick
import QtQuick.Controls

ToolButton {
    id: control
    required property string iconKind
    required property string label
    readonly property color strokeColor: iconKind === "close" && hovered ? "#FFFFFF" : "#675B50"
    implicitWidth: 36
    implicitHeight: 32
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    Accessible.name: label
    ToolTip.visible: hovered
    ToolTip.delay: 400
    ToolTip.text: label

    background: Rectangle {
        radius: height / 2
        color: control.down ? (control.iconKind === "close" ? "#B9473D" : "#E7DDD0")
             : control.hovered ? (control.iconKind === "close" ? "#D7604D" : "#EFE7DB")
             : "transparent"
        border.width: control.visualFocus ? 1 : 0
        border.color: "#D7604D"
    }
    contentItem: Item {
        Canvas {
            id: drawing
            anchors.centerIn: parent
            width: 16
            height: 16
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = control.strokeColor
                ctx.lineWidth = 1.35
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                if (control.iconKind === "minimize") {
                    ctx.moveTo(3, 8); ctx.lineTo(13, 8)
                } else if (control.iconKind === "maximize") {
                    ctx.rect(3.5, 3.5, 9, 9)
                } else if (control.iconKind === "restore") {
                    ctx.moveTo(6.5, 5); ctx.lineTo(6.5, 2.5)
                    ctx.lineTo(13.5, 2.5); ctx.lineTo(13.5, 9.5); ctx.lineTo(11, 9.5)
                    ctx.rect(2.5, 5.5, 8, 8)
                } else {
                    ctx.moveTo(3.5, 3.5); ctx.lineTo(12.5, 12.5)
                    ctx.moveTo(12.5, 3.5); ctx.lineTo(3.5, 12.5)
                }
                ctx.stroke()
            }
        }
    }
    onStrokeColorChanged: drawing.requestPaint()
    onIconKindChanged: drawing.requestPaint()
}
