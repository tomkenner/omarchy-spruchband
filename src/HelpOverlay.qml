import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Every keyboard shortcut, shown with "?".
Popup {
    id: help

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    padding: 24
    background: Rectangle {
        color: "#16161a"
        border.color: "#26262c"
        radius: theme.radius
    }

    readonly property var keys: [
        ["Space", qsTr("Show a banner now")],
        ["P", qsTr("Pause for an hour or resume")],
        ["N", qsTr("New collection")],
        ["?", qsTr("Show or hide this help")],
        ["Esc", qsTr("Close dialogs")],
        ["Q", qsTr("Quit")]
    ]

    contentItem: ColumnLayout {
        spacing: 12
        Label {
            text: qsTr("Keyboard shortcuts")
            font.pixelSize: 18
            font.bold: true
        }
        GridLayout {
            columns: 2
            columnSpacing: 16
            rowSpacing: 8
            Repeater {
                model: help.keys.length * 2
                delegate: Item {
                    required property int index
                    readonly property var row: help.keys[Math.floor(index / 2)]
                    implicitWidth: index % 2 === 0 ? chip.implicitWidth : text.implicitWidth
                    implicitHeight: Math.max(chip.implicitHeight, text.implicitHeight)
                    Rectangle {
                        id: chip
                        visible: index % 2 === 0
                        implicitWidth: Math.max(36, keyLabel.implicitWidth + 16)
                        implicitHeight: keyLabel.implicitHeight + 8
                        color: "#24242a"
                        border.color: "#34343c"
                        radius: Math.min(4, theme.radius)
                        Label {
                            id: keyLabel
                            anchors.centerIn: parent
                            text: row[0]
                            font.family: "monospace"
                        }
                    }
                    Label {
                        id: text
                        visible: index % 2 === 1
                        anchors.verticalCenter: parent.verticalCenter
                        text: row[1]
                    }
                }
            }
        }
    }
}
