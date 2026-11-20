import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Edit one collection: a name and one saying per line.
Dialog {
    id: dlg

    property string collId: ""
    property string originalName: ""
    property string originalText: ""
    property bool discardArmed: false
    property bool deleteArmed: false
    readonly property bool dirty: nameField.text !== originalName || linesArea.text !== originalText

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 640)
    height: Math.min(parent.height - 48, 620)
    modal: true
    closePolicy: Popup.NoAutoClose
    title: collId === "" ? qsTr("New collection") : qsTr("Edit collection")
    Material.roundedScale: theme.radius

    function begin(id, name, text) {
        collId = id
        originalName = name
        originalText = text
        nameField.text = name
        linesArea.text = text
        discardArmed = false
        deleteArmed = false
        open()
        linesArea.forceActiveFocus()
        linesArea.cursorPosition = linesArea.length
    }
    function openFor(id) {
        const c = backend.loadCollection(id)
        begin(id, c.name || "", c.text || "")
    }
    function openNew() {
        begin("", qsTr("My sayings"), "")
    }
    // Unsaved work is only dropped on a second Esc or Cancel.
    function tryClose() {
        if (dirty && !discardArmed) {
            discardArmed = true
            return
        }
        reject()
    }
    function save() {
        if (backend.saveCollection(collId, nameField.text, linesArea.text) !== "") {
            originalName = nameField.text
            originalText = linesArea.text
            accept()
        }
    }

    Shortcut {
        sequence: "Escape"
        enabled: dlg.opened
        onActivated: dlg.tryClose()
    }
    Shortcut {
        sequences: [StandardKey.Save]
        enabled: dlg.opened
        onActivated: dlg.save()
    }

    contentItem: ColumnLayout {
        spacing: 8

        TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: qsTr("Name")
            Material.roundedScale: theme.radius
        }
        Label {
            text: qsTr("One saying per line. Sayings from all ticked collections are shuffled, and each one comes up once before any repeats.")
            color: "#9a9aa3"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: linesArea
                placeholderText: qsTr("One saying per line")
                wrapMode: TextEdit.Wrap
                Material.roundedScale: theme.radius
            }
        }
        Label {
            visible: dlg.discardArmed
            text: qsTr("Unsaved changes. Press Esc or Cancel again to discard them.")
            color: Material.color(Material.Orange)
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    footer: DialogButtonBox {
        Button {
            visible: dlg.collId !== ""
            text: dlg.deleteArmed ? qsTr("Delete this collection?") : qsTr("Delete")
            flat: true
            Material.foreground: dlg.deleteArmed ? Material.color(Material.Red) : undefined
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                if (!dlg.deleteArmed) {
                    dlg.deleteArmed = true
                    return
                }
                backend.deleteCollection(dlg.collId)
                dlg.originalName = nameField.text
                dlg.originalText = linesArea.text
                dlg.reject()
            }
        }
        Button {
            text: dlg.discardArmed ? qsTr("Discard changes") : qsTr("Cancel")
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dlg.tryClose()
        }
        Button {
            text: qsTr("Save")
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dlg.save()
        }
    }
}
