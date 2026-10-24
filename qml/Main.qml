import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    objectName: "mainWindow"
    width: 540
    height: Math.max(650, Math.min(760, Screen.height - 80))
    minimumWidth: 430
    minimumHeight: 650
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    title: "番茄钟"
    color: "#FAF6EF"
    font.family: "Microsoft YaHei UI"
    font.pixelSize: 14
    palette.window: "#FAF6EF"
    palette.button: "#F3EDE5"
    palette.buttonText: "#675B50"
    palette.base: "#FFFCF7"
    palette.text: "#3F342C"
    palette.highlight: "#D7604D"
    palette.highlightedText: "#FFFCF8"
    property var timer: appController.engine
    property color ink: "#3F342C"
    property color muted: "#8F8275"
    property color activeColor: timer.phase === 0 ? "#D7604D"
        : timer.phase === 1 ? "#5E8A73" : "#6485A0"
    property bool modalOpen: settingsDialog.visible || statisticsDialog.visible || confirmDialog.visible
    readonly property bool maximized: visibility === Window.Maximized
    function toggleMaximized() {
        if (maximized) showNormal()
        else showMaximized()
    }
    onClosing: function(event) { event.accepted = appController.closeWindow() }

    Shortcut {
        sequence: "Space"
        enabled: !root.modalOpen
        onActivated: appController.primaryAction()
    }
    Shortcut { sequence: "Ctrl+,"; enabled: !root.modalOpen; onActivated: settingsDialog.open() }
    Shortcut { sequence: "Ctrl+R"; enabled: !root.modalOpen && root.timer.state !== 0; onActivated: appController.requestReset() }
    Connections {
        target: appController
        function onConfirmationRequested(title, message, action, phase) {
            confirmDialog.title = title
            confirmDialog.message = message
            confirmDialog.action = action
            confirmDialog.phase = phase
            confirmDialog.open()
        }
    }

    // Interactive controls above this layer receive their own pointer events.
    // Qt maps their current geometry automatically after resizing or DPI changes.
    MouseArea {
        id: backgroundDragArea
        objectName: "backgroundDragArea"
        anchors.fill: parent
        z: -1
        enabled: !root.modalOpen
        acceptedButtons: Qt.LeftButton
        onPressed: root.startSystemMove()
        onDoubleClicked: root.toggleMaximized()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        anchors.topMargin: 8
        spacing: 0

        Item {
            id: titleArea
            objectName: "titleArea"
            Layout.fillWidth: true
            Layout.preferredHeight: 78

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 34
                spacing: 10
                Image {
                    source: "qrc:/assets/app-icon.png"
                    sourceSize: Qt.size(32, 32)
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                }
                Text { text: "番茄钟"; font.pixelSize: 20; font.weight: Font.DemiBold; color: root.ink }
                Item { Layout.fillWidth: true }
                Row {
                    spacing: 2
                    WindowControlButton {
                        objectName: "minimizeButton"
                        iconKind: "minimize"
                        label: "最小化"
                        onClicked: root.showMinimized()
                    }
                    WindowControlButton {
                        objectName: "maximizeButton"
                        iconKind: root.maximized ? "restore" : "maximize"
                        label: root.maximized ? "还原" : "最大化"
                        onClicked: root.toggleMaximized()
                    }
                    WindowControlButton {
                        objectName: "closeButton"
                        iconKind: "close"
                        label: appController.trayAvailable ? "关闭到托盘" : "关闭"
                        onClicked: root.close()
                    }
                }
            }
            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                spacing: 8
                Text {
                    text: "让每一段专注，都有回响"
                    font.pixelSize: 11
                    color: root.muted
                }
                Item { Layout.fillWidth: true }
                AppButton {
                    objectName: "pinButton"
                    text: appController.alwaysOnTop ? "已置顶" : "置顶"
                    selected: appController.alwaysOnTop
                    compact: true
                    onClicked: appController.toggleOnTop()
                    ToolTip.visible: hovered
                    ToolTip.text: "让窗口保持在其他窗口上方"
                }
                AppButton {
                    objectName: "settingsButton"
                    text: "设置"
                    compact: true
                    onClicked: settingsDialog.open()
                }
            }
        }

        Rectangle {
            id: errorBanner
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.preferredHeight: visible ? errorText.implicitHeight + 22 : 0
            visible: appController.storageError !== "" || appController.soundError !== ""
            color: "#FCE3DA"
            radius: 10
            Text {
                id: errorText
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 11
                anchors.verticalCenter: parent.verticalCenter
                text: appController.storageError !== ""
                    ? "保存失败，当前进度可能无法恢复。\n" + appController.storageError
                    : appController.soundError
                color: "#9E4637"
                wrapMode: Text.Wrap
                font.pixelSize: 11
            }
        }

        Item { Layout.fillHeight: true; Layout.minimumHeight: 16; Layout.maximumHeight: 30 }

        PhaseTabs {
            objectName: "phaseTabs"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(308, root.width - 56)
            Layout.preferredHeight: 50
            selectedIndex: root.timer.phase
            accentColor: root.activeColor
            onPhaseRequested: function(phase) { appController.requestPhase(phase) }
        }

        Item { Layout.preferredHeight: 18 }

        Item {
            id: clockArea
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(root.width - 86, 292, Math.max(130, root.height - 534 - (errorBanner.visible ? errorBanner.Layout.preferredHeight + 16 : 0)))
            Layout.preferredHeight: Layout.preferredWidth
            TomatoDial {
                anchors.fill: parent
                remaining: root.timer.progress
                running: root.timer.state === 1
            }
            Column {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: clockArea.height * 0.075
                spacing: 7
                width: parent.width - 38
                Text {
                    width: parent.width
                    text: root.timer.state === 3 ? "完成" : root.timer.phaseName
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 15
                    color: root.activeColor
                }
                Text {
                    objectName: "timerDisplay"
                    width: parent.width
                    text: root.timer.displayTime
                    horizontalAlignment: Text.AlignHCenter
                    font.family: "Segoe UI"
                    font.pixelSize: Math.min(65, clockArea.width * 0.23)
                    font.weight: Font.Light
                    color: root.ink
                }
                Text {
                    width: parent.width
                    text: root.timer.state === 1 ? "正在进行" : root.timer.state === 2 ? "已暂停"
                          : root.timer.state === 3 ? "下一段 · " + root.timer.nextPhaseName : "准备好了就开始"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 12
                    color: root.muted
                }
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 16
            text: root.timer.statusText
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: root.timer.state === 3 ? root.activeColor : root.muted
        }

        Row {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 16
            spacing: 6
            Repeater {
                model: root.timer.cycleTarget
                Image {
                    required property int index
                    objectName: "cycleTomato" + index
                    width: 24
                    height: 24
                    source: index < root.timer.completedInCycle
                        ? "qrc:/assets/cycle-tomato.png" : "qrc:/assets/cycle-tomato-pending.png"
                    // Ignore transparent margins without modifying the supplied artwork.
                    sourceClipRect: Qt.rect(158, 184, 937, 891)
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
            }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 7
            text: "本轮 " + root.timer.completedInCycle + " / " + root.timer.cycleTarget + " 次专注"
            font.pixelSize: 11
            color: root.muted
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 21
            spacing: 10
            AppButton {
                objectName: "primaryButton"
                text: root.timer.primaryLabel
                accent: true
                accentColor: root.activeColor
                Layout.preferredWidth: 180
                onClicked: appController.primaryAction()
            }
            AppButton {
                objectName: "resetButton"
                text: "重新计时"
                enabled: root.timer.state !== 0
                Layout.preferredWidth: 108
                onClicked: appController.requestReset()
            }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 9
            text: "空格  开始 / 暂停"
            font.pixelSize: 10
            color: "#A89B8D"
        }

        Item { Layout.fillHeight: true; Layout.minimumHeight: 18; Layout.maximumHeight: 36 }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#EAE0D5" }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 16
            spacing: 12
            ColumnLayout {
                spacing: 4
                Text { text: "今日完成"; font.pixelSize: 11; color: root.muted }
                Row {
                    spacing: 6
                    Text { text: appController.todayCount; font.family: "Segoe UI"; font.pixelSize: 24; color: root.ink }
                    Text { text: "个番茄"; font.pixelSize: 11; color: root.muted; anchors.bottom: parent.bottom; anchors.bottomMargin: 5 }
                }
            }
            Item { Layout.fillWidth: true }
            ColumnLayout {
                spacing: 4
                Text { text: "已完成专注"; font.pixelSize: 11; color: root.muted }
                Row {
                    spacing: 6
                    Text { text: appController.todayMinutes; font.family: "Segoe UI"; font.pixelSize: 24; color: root.ink }
                    Text { text: "分钟"; font.pixelSize: 11; color: root.muted; anchors.bottom: parent.bottom; anchors.bottomMargin: 5 }
                }
            }
            Item { Layout.fillWidth: true }
            AppButton {
                objectName: "statisticsButton"
                text: "查看统计"
                compact: true
                onClicked: statisticsDialog.open()
            }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 14
            text: appController.trayAvailable ? "关闭窗口后，番茄钟会留在系统托盘" : "当前系统托盘不可用，关闭窗口将退出"
            font.pixelSize: 10
            color: "#A89B8D"
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: root.maximized ? 0 : 1
        border.color: "#E5DCD0"
        z: 20
    }

    // Let Windows handle resizing so DPI changes and desktop snapping stay native.
    Item {
        id: resizeHandles
        anchors.fill: parent
        z: 30
        visible: !root.maximized && !root.modalOpen
        ResizeHandle {
            objectName: "resizeLeft"
            window: root; edges: Qt.LeftEdge; cursorShape: Qt.SizeHorCursor
            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
            anchors.topMargin: 8; anchors.bottomMargin: 8; width: 5
        }
        ResizeHandle {
            objectName: "resizeRight"
            window: root; edges: Qt.RightEdge; cursorShape: Qt.SizeHorCursor
            anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
            anchors.topMargin: 8; anchors.bottomMargin: 8; width: 5
        }
        ResizeHandle {
            objectName: "resizeTop"
            window: root; edges: Qt.TopEdge; cursorShape: Qt.SizeVerCursor
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            anchors.leftMargin: 8; anchors.rightMargin: 8; height: 5
        }
        ResizeHandle {
            objectName: "resizeBottom"
            window: root; edges: Qt.BottomEdge; cursorShape: Qt.SizeVerCursor
            anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
            anchors.leftMargin: 8; anchors.rightMargin: 8; height: 5
        }
        ResizeHandle {
            objectName: "resizeTopLeft"
            window: root; edges: Qt.TopEdge | Qt.LeftEdge; cursorShape: Qt.SizeFDiagCursor
            anchors.top: parent.top; anchors.left: parent.left; width: 8; height: 8
        }
        ResizeHandle {
            objectName: "resizeTopRight"
            window: root; edges: Qt.TopEdge | Qt.RightEdge; cursorShape: Qt.SizeBDiagCursor
            anchors.top: parent.top; anchors.right: parent.right; width: 8; height: 8
        }
        ResizeHandle {
            objectName: "resizeBottomLeft"
            window: root; edges: Qt.BottomEdge | Qt.LeftEdge; cursorShape: Qt.SizeBDiagCursor
            anchors.bottom: parent.bottom; anchors.left: parent.left; width: 8; height: 8
        }
        ResizeHandle {
            objectName: "resizeBottomRight"
            window: root; edges: Qt.BottomEdge | Qt.RightEdge; cursorShape: Qt.SizeFDiagCursor
            anchors.bottom: parent.bottom; anchors.right: parent.right; width: 8; height: 8
        }
    }

    Dialog {
        id: confirmDialog
        objectName: "confirmDialog"
        property string message: ""
        property int action: 0
        property int phase: 0
        anchors.centerIn: Overlay.overlay
        width: Math.min(root.width - 40, 390)
        modal: true
        padding: 24
        closePolicy: Popup.CloseOnEscape
        background: Rectangle { color: "#FFFCF7"; radius: 18; border.color: "#E6DACE" }
        header: Text {
            text: confirmDialog.title
            padding: 24; bottomPadding: 12
            font.pixelSize: 19; font.weight: Font.DemiBold; color: root.ink
        }
        contentItem: ColumnLayout {
            spacing: 24
            Text { Layout.fillWidth: true; text: confirmDialog.message; wrapMode: Text.Wrap; font.pixelSize: 14; color: "#7D6F62" }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                AppButton { text: "保留进度"; onClicked: confirmDialog.close() }
                AppButton {
                    objectName: "confirmDiscardButton"
                    text: "确认放弃"
                    accent: true
                    onClicked: {
                        appController.confirmAction(confirmDialog.action, confirmDialog.phase)
                        confirmDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: settingsDialog
        objectName: "settingsDialog"
        property int focusValue: 25
        property int shortValue: 5
        property int longValue: 15
        property int cycleValue: 4
        property int volumeValue: 70
        property bool soundValue: true
        property bool pinValue: false
        anchors.centerIn: Overlay.overlay
        width: Math.min(root.width - 40, 438)
        height: Math.min(root.height - 46, 632)
        modal: true
        padding: 24
        closePolicy: Popup.CloseOnEscape
        background: Rectangle { color: "#FFFCF7"; radius: 18; border.color: "#E6DACE" }
        onOpened: {
            let saved = appController.settings
            focusValue = saved.focusMinutes
            shortValue = saved.shortMinutes
            longValue = saved.longMinutes
            cycleValue = saved.cycles
            volumeValue = saved.volume
            soundValue = saved.soundEnabled
            pinValue = saved.alwaysOnTop
        }
        header: Column {
            Text { text: "按你的节奏"; leftPadding: 24; topPadding: 24; font.pixelSize: 21; font.weight: Font.DemiBold; color: root.ink }
            Text { text: "进行中的计时保持原时长，新时长用于下一阶段"; leftPadding: 24; topPadding: 7; bottomPadding: 14; font.pixelSize: 11; color: root.muted }
        }
        contentItem: ScrollView {
            id: settingsScroll
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ColumnLayout {
                width: settingsScroll.availableWidth
                spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "专注时长"; color: root.ink; Layout.fillWidth: true }
                    RoundedSpinBox { objectName: "focusSpin"; from: 1; to: 180; value: settingsDialog.focusValue; editable: true; onValueModified: settingsDialog.focusValue = value }
                    Text { text: "分钟"; color: root.muted; font.pixelSize: 12 }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "短休息"; color: root.ink; Layout.fillWidth: true }
                    RoundedSpinBox { from: 1; to: 60; value: settingsDialog.shortValue; editable: true; onValueModified: settingsDialog.shortValue = value }
                    Text { text: "分钟"; color: root.muted; font.pixelSize: 12 }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "长休息"; color: root.ink; Layout.fillWidth: true }
                    RoundedSpinBox { from: 1; to: 120; value: settingsDialog.longValue; editable: true; onValueModified: settingsDialog.longValue = value }
                    Text { text: "分钟"; color: root.muted; font.pixelSize: 12 }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "长休息间隔"; color: root.ink; Layout.fillWidth: true }
                    RoundedSpinBox { from: 1; to: 12; value: settingsDialog.cycleValue; editable: true; onValueModified: settingsDialog.cycleValue = value }
                    Text { text: "次"; color: root.muted; font.pixelSize: 12; Layout.preferredWidth: 24 }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#EAE0D5" }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "结束提示音"; color: root.ink; Layout.fillWidth: true }
                    Switch { objectName: "soundSwitch"; checked: settingsDialog.soundValue; onToggled: settingsDialog.soundValue = checked; Accessible.name: "结束提示音" }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "音量"; color: root.muted; font.pixelSize: 12 }
                    Slider {
                        objectName: "volumeSlider"
                        Layout.fillWidth: true
                        from: 0; to: 100; stepSize: 1; value: settingsDialog.volumeValue
                        onMoved: settingsDialog.volumeValue = Math.round(value)
                        Accessible.name: "提示音音量"
                    }
                    Text { text: settingsDialog.volumeValue + "%"; color: root.muted; font.pixelSize: 12; Layout.preferredWidth: 34 }
                    AppButton { objectName: "previewButton"; text: "试听"; compact: true; onClicked: appController.previewSound(settingsDialog.volumeValue) }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "窗口置顶"; color: root.ink; Layout.fillWidth: true }
                    Switch { checked: settingsDialog.pinValue; onToggled: settingsDialog.pinValue = checked; Accessible.name: "窗口置顶" }
                }
                Text {
                    Layout.fillWidth: true
                    text: "阶段手动切换；睡眠自动暂停，唤醒后继续。"
                    font.pixelSize: 11
                    color: root.muted
                    wrapMode: Text.Wrap
                    lineHeight: 1.5
                }
                Text {
                    Layout.fillWidth: true
                    visible: appController.validationError !== "" || appController.storageError !== ""
                    text: appController.validationError !== "" ? appController.validationError
                        : "设置已应用，但尚未保存。\n" + appController.storageError
                    color: "#AC4E3C"; font.pixelSize: 11; wrapMode: Text.Wrap
                }
            }
        }
        footer: Item {
            height: 78
            RowLayout {
                anchors.right: parent.right
                anchors.rightMargin: 24
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                AppButton { text: "取消"; onClicked: settingsDialog.close() }
                AppButton {
                    objectName: "saveSettingsButton"
                    text: "保存设置"; accent: true
                    onClicked: {
                        if (appController.applySettings({
                            focusMinutes: settingsDialog.focusValue,
                            shortMinutes: settingsDialog.shortValue,
                            longMinutes: settingsDialog.longValue,
                            cycles: settingsDialog.cycleValue,
                            volume: settingsDialog.volumeValue,
                            soundEnabled: settingsDialog.soundValue,
                            alwaysOnTop: settingsDialog.pinValue
                        })) settingsDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: statisticsDialog
        objectName: "statisticsDialog"
        anchors.centerIn: Overlay.overlay
        width: Math.min(root.width - 40, 420)
        modal: true
        padding: 24
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: "#FFFCF7"; radius: 18; border.color: "#E6DACE" }
        header: Text { text: "每一段，都算数"; padding: 24; bottomPadding: 14; color: root.ink; font.pixelSize: 21; font.weight: Font.DemiBold }
        contentItem: ColumnLayout {
            spacing: 14
            Text { Layout.fillWidth: true; text: "最近 7 天 · 仅统计完整完成的专注"; font.pixelSize: 11; color: root.muted }
            RowLayout {
                Layout.fillWidth: true
                Text { text: "日期"; Layout.fillWidth: true; color: root.muted; font.pixelSize: 11 }
                Text { text: "完成番茄"; Layout.preferredWidth: 76; horizontalAlignment: Text.AlignRight; color: root.muted; font.pixelSize: 11 }
                Text { text: "专注分钟"; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight; color: root.muted; font.pixelSize: 11 }
            }
            Repeater {
                model: appController.history
                RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Text { text: modelData.label; Layout.fillWidth: true; color: root.ink; font.pixelSize: 14 }
                    Text { text: modelData.count; Layout.preferredWidth: 76; horizontalAlignment: Text.AlignRight; color: root.activeColor; font.pixelSize: 16 }
                    Text { text: modelData.minutes; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight; color: root.ink; font.pixelSize: 16 }
                }
            }
            Text { Layout.fillWidth: true; Layout.topMargin: 4; text: "跨午夜的专注，计入完成当天。"; color: root.muted; font.pixelSize: 11 }
            AppButton { Layout.alignment: Qt.AlignRight; text: "完成"; onClicked: statisticsDialog.close() }
        }
    }
}
