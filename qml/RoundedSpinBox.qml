import QtQuick
import QtQuick.Controls

SpinBox {
    id: control
    implicitWidth: 124
    implicitHeight: 38
    font.family: "Microsoft YaHei UI"
    font.pixelSize: 14
    background: Rectangle {
        radius: height / 2
        color: "#F3EDE5"
        border.width: control.activeFocus ? 1 : 0
        border.color: "#C98770"
    }
    up.indicator: Rectangle {
        x: control.width - width - 4
        y: (control.height - height) / 2
        width: 30
        height: 30
        radius: height / 2
        color: control.up.pressed ? "#E4D7C8" : control.up.hovered ? "#EBE1D5" : "#FFFCF7"
        Text {
            anchors.centerIn: parent
            text: "+"
            font.pixelSize: 20
            color: parent.enabled ? "#675B50" : "#B8AEA2"
        }
    }
    down.indicator: Rectangle {
        x: 4
        y: (control.height - height) / 2
        width: 30
        height: 30
        radius: height / 2
        color: control.down.pressed ? "#E4D7C8" : control.down.hovered ? "#EBE1D5" : "#FFFCF7"
        Text {
            anchors.centerIn: parent
            text: "−"
            font.pixelSize: 20
            color: parent.enabled ? "#675B50" : "#B8AEA2"
        }
    }
}
