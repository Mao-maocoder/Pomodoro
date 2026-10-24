import QtQuick
import QtQuick.Controls

Item {
    id: control
    implicitWidth: 308
    implicitHeight: 50
    property int selectedIndex: 0
    property color accentColor: "#D7604D"
    property real lensStrength: 7.0
    property int animationDuration: 680
    readonly property real inset: 4
    readonly property real cellWidth: (width - inset * 2) / 3
    readonly property real targetX: inset + selectedIndex * cellWidth
    property real glassX: inset
    property bool initialized: false
    readonly property bool moving: slide.running
    readonly property bool effectsEnabled: GraphicsInfo.api !== GraphicsInfo.Software
        && GraphicsInfo.api !== GraphicsInfo.Unknown && GraphicsInfo.api !== GraphicsInfo.Null
    readonly property bool shaderReady: glassEffect.status === ShaderEffect.Compiled
    readonly property string shaderLog: glassEffect.log
    signal phaseRequested(int phase)

    function moveSelection() {
        slide.stop()
        if (!initialized || Math.abs(glassX - targetX) < 0.05) {
            glassX = targetX
            return
        }
        slide.from = glassX
        slide.to = targetX
        slide.start()
    }
    onTargetXChanged: moveSelection()
    onWidthChanged: {
        slide.stop()
        glassX = inset + selectedIndex * ((width - inset * 2) / 3)
    }
    Component.onCompleted: {
        glassX = targetX
        initialized = true
    }
    NumberAnimation {
        id: slide
        objectName: "phaseSlide"
        target: control
        property: "glassX"
        duration: control.animationDuration
        easing.type: Easing.InOutCubic
    }

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        gradient: Gradient {
            GradientStop { position: 0; color: "#F0EAE2" }
            GradientStop { position: 1; color: "#F4EEE7" }
        }
        border.width: 1
        border.color: "#F7F2EC"
    }

    // The software renderer keeps the same controls and sliding capsule.
    Item {
        visible: !control.effectsEnabled
        x: control.glassX
        y: control.inset
        width: control.cellWidth
        height: control.height - control.inset * 2
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 2
            anchors.bottomMargin: -2
            radius: height / 2
            color: "#28A97960"
        }
        Rectangle {
            anchors.fill: parent
            radius: height / 2
            gradient: Gradient {
                GradientStop { position: 0; color: "#FFFDFA" }
                GradientStop { position: 0.55; color: "#FBECE3" }
                GradientStop { position: 1; color: "#F2D7C8" }
            }
            border.width: 1
            border.color: "#FFFFFF"
        }
    }

    Item {
        id: labelsLayer
        anchors.fill: parent
        Repeater {
            model: ["专注", "短休息", "长休息"]
            Text {
                required property int index
                required property string modelData
                x: control.inset + index * control.cellWidth
                width: control.cellWidth
                height: control.height
                text: modelData
                font.family: "Microsoft YaHei UI"
                font.pixelSize: 15
                font.weight: control.selectedIndex === index ? Font.DemiBold : Font.Normal
                color: !control.effectsEnabled && control.selectedIndex === index
                    ? control.accentColor : "#796B5D"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                renderType: Text.QtRendering
            }
        }
    }

    ShaderEffectSource {
        id: labelsTexture
        anchors.fill: parent
        sourceItem: control.effectsEnabled ? labelsLayer : null
        hideSource: control.effectsEnabled
        live: true
        visible: false
        smooth: true
        textureSize: Qt.size(Math.ceil(width * 2), Math.ceil(height * 2))
    }
    ShaderEffect {
        id: glassEffect
        objectName: "phaseGlassEffect"
        anchors.fill: parent
        visible: control.effectsEnabled
        property var source: labelsTexture
        property size controlSize: Qt.size(width, height)
        property vector4d glassRect: Qt.vector4d(control.glassX, control.inset,
            control.cellWidth, height - control.inset * 2)
        property color accentColor: control.accentColor
        property real lensStrength: control.lensStrength
        fragmentShader: "qrc:/shaders/liquidglass.frag.qsb"
    }

    Row {
        x: control.inset
        width: control.width - control.inset * 2
        height: control.height
        Repeater {
            model: ["专注", "短休息", "长休息"]
            Button {
                required property int index
                required property string modelData
                objectName: "phaseButton" + index
                width: control.cellWidth
                height: control.height
                hoverEnabled: true
                focusPolicy: Qt.TabFocus
                Accessible.name: modelData
                Accessible.description: control.selectedIndex === index ? "当前阶段" : "切换计时阶段"
                contentItem: Item {}
                background: Rectangle {
                    anchors.fill: parent
                    anchors.margins: 3
                    radius: height / 2
                    color: parent.hovered ? "#08FFFFFF" : "transparent"
                    border.width: parent.visualFocus ? 2 : 0
                    border.color: control.accentColor
                }
                onClicked: control.phaseRequested(index)
                Keys.onLeftPressed: control.phaseRequested(Math.max(0, index - 1))
                Keys.onRightPressed: control.phaseRequested(Math.min(2, index + 1))
                Keys.onReturnPressed: control.phaseRequested(index)
            }
        }
    }
}
