import QtQuick
import QtQuick.Window

MouseArea {
    required property Window window
    required property int edges
    acceptedButtons: Qt.LeftButton
    onPressed: window.startSystemResize(edges)
}
