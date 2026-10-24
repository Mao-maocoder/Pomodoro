import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool accent: false
    property bool selected: false
    property bool compact: false
    property color accentColor: "#D7604D"
    hoverEnabled: true
    implicitHeight: compact ? 38 : 48
    implicitWidth: Math.max(implicitContentWidth + (compact ? 26 : 38), compact ? 44 : 84)
    leftPadding: compact ? 13 : 19
    rightPadding: leftPadding
    topPadding: 10
    bottomPadding: 10
    font.family: "Microsoft YaHei UI"
    font.pixelSize: compact ? 13 : 15
    font.weight: accent ? Font.DemiBold : Font.Normal
    contentItem: Text {
        text: control.text
        font: control.font
        color: !control.enabled ? "#ADA59D" : control.accent ? "#FFFCF8" : control.selected ? control.accentColor : "#675B50"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: height / 2
        color: !control.enabled ? "#EDE7DF"
             : control.accent ? (control.down ? Qt.darker(control.accentColor, 1.13) : control.hovered ? Qt.lighter(control.accentColor, 1.07) : control.accentColor)
             : control.selected ? "#FBE7DE"
             : control.down ? "#E6DDD2" : control.hovered ? "#EDE5DB" : "#F3EDE5"
        border.width: control.visualFocus ? 2 : 0
        border.color: control.accentColor
        Behavior on color { ColorAnimation { duration: 100 } }
    }
    Accessible.name: text
}

