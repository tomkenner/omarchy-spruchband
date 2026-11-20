import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// One section of the settings, like the Android app's cards.
Rectangle {
    id: card

    property string title
    property string hint
    default property alias content: body.data

    Layout.fillWidth: true
    implicitHeight: column.implicitHeight + 36
    color: "#16161a"
    border.color: "#26262c"
    radius: theme.radius

    ColumnLayout {
        id: column
        x: 18
        y: 18
        width: card.width - 36
        spacing: 4

        Label {
            text: card.title
            font.pixelSize: 18
            font.bold: true
        }
        Label {
            visible: card.hint !== ""
            text: card.hint
            color: "#9a9aa3"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.bottomMargin: 4
        }
        ColumnLayout {
            id: body
            Layout.fillWidth: true
            spacing: 0
        }
    }
}
